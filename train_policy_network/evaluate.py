# Sample: python evaluate.py --model_file="trained_model_depth=192_layers=12/model.ckpt" --pipeline_length=12 --K=192 --game_data_train_file="../data/preprocessed_games.data"

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import gzip
import json
import os
import sys
import time

import numpy
from six.moves import urllib
from six.moves import xrange  # pylint: disable=redefined-builtin
import tensorflow as tf
import sys

numpy.random.seed(int(time.time()))

IMAGE_SIZE = 9
NUM_CHANNELS = 4
NUM_LABELS = 81
SEED = None
EVAL_BATCH_SIZE = 64
NUM_TO_EVALUATE = 1000

tf.app.flags.DEFINE_string("game_data_train_file", "", "game data location")
tf.app.flags.DEFINE_boolean('use_fp16', False,
                            "Use half floats instead of full floats if True.")
tf.app.flags.DEFINE_string("model_file", "", "")
tf.app.flags.DEFINE_integer("pipeline_length", "", "pipeline length")
tf.app.flags.DEFINE_integer("K", "", "depth")
FLAGS = tf.app.flags.FLAGS

def data_type():
  """Return the type of the activations, weights, and placeholder variables."""
  if FLAGS.use_fp16:
    return tf.float16
  else:
    return tf.float32

eval_data = tf.placeholder(
    data_type(),
    shape=(EVAL_BATCH_SIZE, IMAGE_SIZE, IMAGE_SIZE, NUM_CHANNELS))

first_conv = tf.Variable(tf.truncated_normal([3, 3, NUM_CHANNELS, FLAGS.K],
                                             stddev=.1, seed=SEED, dtype=data_type()))
first_bias = tf.Variable(tf.zeros([FLAGS.K], dtype=data_type()))
conv_weights = []
conv_biases = []

pipeline_length = FLAGS.pipeline_length
for i in range(pipeline_length):
  conv_weight = tf.Variable(tf.truncated_normal([3, 3, FLAGS.K, FLAGS.K],
                                                stddev=0.1, seed=SEED, dtype=data_type()))
  conv_bias = tf.Variable(tf.zeros([FLAGS.K], dtype=data_type()))
  conv_weights.append(conv_weight)
  conv_biases.append(conv_bias)
final_conv_weight = tf.Variable(tf.truncated_normal([3, 3, FLAGS.K, 1],
                                                    stddev=0.1, seed=SEED, dtype=data_type()))

# We will replicate the model structure for the training subgraph, as well
# as the evaluation subgraphs, while sharing the trainable parameters.
def model(data, train=False):
    conv, relu = None, None
    conv = tf.nn.conv2d(data, first_conv, strides=[1,1,1,1], padding="SAME")
    relu = tf.nn.relu(tf.nn.bias_add(conv, first_bias))
    #relu = tf.pad(relu, [[0,0],[3,3],[3,3],[0,0]], "CONSTANT")
    for i in range(0, FLAGS.pipeline_length):
        conv = tf.nn.conv2d(relu, conv_weights[i], strides=[1,1,1,1], padding="SAME")
        relu = tf.nn.relu(tf.nn.bias_add(conv, conv_biases[i]))
        #relu = tf.pad(relu, [[0,0],[3,3],[3,3],[0,0]], "CONSTANT")
        final_conv = tf.nn.conv2d(relu, final_conv_weight, strides=[1,1,1,1], padding="SAME")
    shape = final_conv.get_shape().as_list()
    return tf.reshape(final_conv, [shape[0], shape[1]*shape[2]*shape[3]])
  #return reshape

eval_prediction = tf.nn.softmax(model(eval_data))

def get_data():
  print("Loading game training data...")
  f = open(FLAGS.game_data_train_file, "r")
  raw_data = json.loads(f.read())
  f.close()

  # Be sure to shuffle.
  numpy.random.shuffle(raw_data)
  train_data = numpy.array([x[0] for x in raw_data])
  label_data = numpy.array([x[1] for x in raw_data])

  print("%d train_data" % len(train_data))

  # Need data in 4d tensor form: [image index, y, x, channels]
  print("Done.")
  return train_data[:NUM_TO_EVALUATE], label_data[:NUM_TO_EVALUATE]

def error_rate(predictions, labels, take_k):
  """Return the error rate based on dense predictions and sparse labels."""
  n_right, n_total = 0, 0
  for i, prediction in enumerate(predictions):
      prediction = [x[0] for x in sorted([(index, value) for index, value in enumerate(prediction)], key=lambda x:x[1], reverse=True)]
      if labels[i] in prediction[:take_k]:
          n_right += 1
      n_total += 1
  return 100.0 - 100 * float(n_right / n_total)

def eval_in_batches(data, sess):
  """Get all predictions for a dataset by running it in small batches."""
  size = data.shape[0]
  if size < EVAL_BATCH_SIZE:
    raise ValueError("batch size for evals larger than dataset: %d" % size)
  predictions = numpy.ndarray(shape=(size, NUM_LABELS), dtype=numpy.float32)
  for begin in xrange(0, size, EVAL_BATCH_SIZE):
    end = begin + EVAL_BATCH_SIZE
    if end <= size:
      predictions[begin:end, :] = sess.run(
          eval_prediction,
          feed_dict={eval_data: data[begin:end, ...]})
    else:
      batch_predictions = sess.run(
          eval_prediction,
          feed_dict={eval_data: data[-EVAL_BATCH_SIZE:, ...]})
      predictions[begin:, :] = batch_predictions[begin - size:, :]
  return predictions

FLAGS = tf.app.flags.FLAGS
saver = tf.train.Saver()
with tf.Session() as sess:
    tf.initialize_all_variables().run()
    saver.restore(sess, FLAGS.model_file)
    print("Initialized!")
    test_data, label_data = get_data()
    for i in range(1, 5):
        overall_error = error_rate(eval_in_batches(test_data, sess), label_data, i)
        print("Overall error considering top %d predicted moves : %f"  % (i, overall_error))
