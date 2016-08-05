# Sample: python convolutional.py --game_data_train_file="../data/preprocessed_games.data" --pipeline_length=20 --K=10

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

WORK_DIRECTORY = 'data'
IMAGE_SIZE = 9
NUM_CHANNELS = 4
NUM_LABELS = 81
VALIDATION_SIZE = 1000  # Size of the validation set.
SEED = None  # Set to None for random seed.
BATCH_SIZE = 64
NUM_EPOCHS = 10
EVAL_BATCH_SIZE = 64
EVAL_FREQUENCY = 100  # Number of steps between evaluations.
SAVE_FREQUENCY = 1000

tf.app.flags.DEFINE_boolean("self_test", False, "True if running a self test.")
tf.app.flags.DEFINE_boolean('use_fp16', False,
                            "Use half floats instead of full floats if True.")
tf.app.flags.DEFINE_string("game_data_train_file", "", "game data location")
tf.app.flags.DEFINE_integer("pipeline_length", "", "pipeline length")
tf.app.flags.DEFINE_integer("K", "", "depth")
FLAGS = tf.app.flags.FLAGS


def data_type():
  """Return the type of the activations, weights, and placeholder variables."""
  if FLAGS.use_fp16:
    return tf.float16
  else:
    return tf.float32


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
  return train_data, label_data

def error_rate(predictions, labels):
  """Return the error rate based on dense predictions and sparse labels."""
  return 100.0 - (
      100.0 *
      numpy.sum(numpy.argmax(predictions, 1) == labels) /
      predictions.shape[0])


def main(argv=None):  # pylint: disable=unused-argument

  # Load validation_data, validataion_labels,
  # train_data, train_labels
  data, labels = get_data()
  train_data, train_labels = data[VALIDATION_SIZE*2:], labels[VALIDATION_SIZE*2:]
  test_data, test_labels = data[VALIDATION_SIZE:VALIDATION_SIZE*2], labels[VALIDATION_SIZE:VALIDATION_SIZE*2]
  validation_data, validation_labels = data[:VALIDATION_SIZE], labels[:VALIDATION_SIZE]

  num_epochs = NUM_EPOCHS
  train_size = train_labels.shape[0]

  # This is where training samples and labels are fed to the graph.
  # These placeholder nodes will be fed a batch of training data at each
  # training step using the {feed_dict} argument to the Run() call below.
  train_data_node = tf.placeholder(
      data_type(),
      shape=(BATCH_SIZE, IMAGE_SIZE, IMAGE_SIZE, NUM_CHANNELS))
  train_labels_node = tf.placeholder(tf.int64, shape=(BATCH_SIZE,))
  eval_data = tf.placeholder(
      data_type(),
      shape=(EVAL_BATCH_SIZE, IMAGE_SIZE, IMAGE_SIZE, NUM_CHANNELS))

  pipeline_length = FLAGS.pipeline_length

  first_conv = tf.Variable(tf.truncated_normal([3, 3, NUM_CHANNELS, FLAGS.K],
                                               stddev=.1, seed=SEED, dtype=data_type()))
  first_bias = tf.Variable(tf.zeros([FLAGS.K], dtype=data_type()))

  conv_weights = []
  conv_biases = []
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

  # Training computation: logits + cross-entropy loss.
  logits = model(train_data_node, True)
  loss = tf.reduce_mean(tf.nn.sparse_softmax_cross_entropy_with_logits(
      logits, train_labels_node))

  loss += 5e-4

  # Optimizer: set up a variable that's incremented once per batch and
  # controls the learning rate decay.
  batch = tf.Variable(0, dtype=data_type())
  # Decay once per epoch, using an exponential schedule starting at 0.01.
  learning_rate = tf.train.exponential_decay(
      0.001,                # Base learning rate.
      batch * BATCH_SIZE,  # Current index into the dataset.
      train_size/4,          # Decay step.
      0.95,                # Decay rate.
      staircase=True)
  # Use simple momentum for the optimization.
  #optimizer = tf.train.MomentumOptimizer(learning_rate,
  #                                       0.9).minimize(loss,
  #                                                     global_step=batch)
  optimizer = tf.train.AdamOptimizer(learning_rate, .09).minimize(loss, global_step=batch)

  # Predictions for the current training minibatch.
  train_prediction = tf.nn.softmax(logits)

  # Predictions for the test and validation, which we'll compute less often.
  eval_prediction = tf.nn.softmax(model(eval_data))

  # Small utility function to evaluate a dataset by feeding batches of data to
  # {eval_data} and pulling the results from {eval_predictions}.
  # Saves memory and enables this to run on smaller GPUs.
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

  saver = tf.train.Saver()

  # Create a local session to run the training.
  start_time = time.time()
  with tf.Session() as sess:
    # Run all the initializers to prepare the trainable parameters.
    tf.initialize_all_variables().run()
    saver.restore(sess, "./trained_model/model.ckpt")
    print('Initialized!')
    # Loop through training steps.
    for step in xrange(int(num_epochs * train_size) // BATCH_SIZE):
      # Compute the offset of the current minibatch in the data.
      # Note that we could use better randomization across epochs.
      offset = (step * BATCH_SIZE) % (train_size - BATCH_SIZE)
      batch_data = train_data[offset:(offset + BATCH_SIZE), ...]
      batch_labels = train_labels[offset:(offset + BATCH_SIZE)]
      # This dictionary maps the batch data (as a numpy array) to the
      # node in the graph it should be fed to.
      feed_dict = {train_data_node: batch_data,
                   train_labels_node: batch_labels}
      # Run the graph and fetch some of the nodes.
      _, l, lr, predictions = sess.run(
          [optimizer, loss, learning_rate, train_prediction],
          feed_dict=feed_dict)
      if step % EVAL_FREQUENCY == 0:
        elapsed_time = time.time() - start_time
        start_time = time.time()
        print('Step %d (epoch %.2f), %.1f ms' %
              (step, float(step) * BATCH_SIZE / train_size,
               1000 * elapsed_time / EVAL_FREQUENCY))
        print('Minibatch loss: %.3f, learning rate: %.6f' % (l, lr))
        print('Minibatch error: %.1f%%' % error_rate(predictions, batch_labels))
        print('Validation error: %.1f%%' % error_rate(
            eval_in_batches(validation_data, sess), validation_labels))
        sys.stdout.flush()

      if step % SAVE_FREQUENCY == 0:
        save_path = saver.save(sess, "./trained_model/model.ckpt")
        print("Model saved in file: %s" % save_path)
    # Finally print the result!
    test_error = error_rate(eval_in_batches(test_data, sess), test_labels)
    print('Test error: %.1f%%' % test_error)
    if FLAGS.self_test:
      print('test_error', test_error)
      assert test_error == 0.0, 'expected 0.0 test_error, got %.2f' % (
          test_error,)


if __name__ == '__main__':
  tf.app.run()
