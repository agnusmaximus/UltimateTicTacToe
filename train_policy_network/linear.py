# linear model that trains prediction of moves
import tensorflow as tf
from scipy.io import loadmat
import time
import numpy as np
import sys

# TODO: use flags instead
input_file_name = '../data/processed_games.mat'
initial_learning_rate = .1
num_steps = 10001

BATCH_SIZE = 8192
IMAGE_SIZE = 9
NUM_CHANNELS = 8
EVAL_FREQUENCY = 10
NUM_LABELS = 81
SAVE_FREQUENCY = 1000

def error_rate(predictions, labels):
 	"""Return the error rate based on dense predictions and sparse labels."""
 	return 100.0 - (
		100.0 *
		np.sum(np.argmax(predictions, 1) == labels) /
		predictions.shape[0])

def get_data(input_file_name):
	# load data
	input_data = loadmat(input_file_name)
	data = np.array(input_data['features'], dtype=np.bool)
	labels = input_data['labels']

	# flatten data
	data = np.reshape(data, (data.shape[0], (data.shape[1] * data.shape[2] * data.shape[3])))

	# map labels from (y, x) -> y*9 + x
	labels[:, 1] *= 9
	labels = np.sum(labels, axis=1)
	labels = np.array(labels, dtype=np.int64)

	# typecast data
	data = np.array(data, dtype=np.float32)
	labels = np.array(labels, dtype=np.int64)

	# 98/1/1 split for train/validation/test data
	data_len = data.shape[0]
	split = 98 * data_len / 100
	split2 = 99 * data_len / 100

	train_data = data[:split]
	train_labels = labels[:split]
	validation_data = data[split:split2]
	validation_labels = labels[split:split2]
	test_data = data[split2:]
	test_labels = labels[split2:]

	return (train_data, train_labels, validation_data, validation_labels, test_data, test_labels)

def main():
	data = get_data(input_file_name)
	train_data, train_labels, validation_data, validation_labels, test_data, test_labels = data
	train_size = train_data.shape[0]

	# the model
	train_data_node = tf.placeholder(
		tf.float32,
		shape=(BATCH_SIZE, IMAGE_SIZE * IMAGE_SIZE * NUM_CHANNELS))
	train_labels_node = tf.placeholder(tf.int64, shape=(BATCH_SIZE,))
	eval_data = tf.placeholder(
		tf.float32,
		shape=(BATCH_SIZE, IMAGE_SIZE * IMAGE_SIZE * NUM_CHANNELS))
	# define the weights so I can print them later.
	# TODO: figure out how to print weights from tf.contrib.layers
	weights = tf.Variable(tf.truncated_normal([IMAGE_SIZE * IMAGE_SIZE * NUM_CHANNELS, NUM_LABELS],
		stddev=0.1, dtype=tf.float32))
	bias = tf.Variable(tf.truncated_normal([NUM_LABELS,],
		stddev=0.1, dtype=tf.float32))

	# get logits layer
	logits = tf.nn.bias_add(tf.matmul(train_data_node, weights), bias)
	# cross entropy error
	loss = tf.reduce_mean(tf.nn.sparse_softmax_cross_entropy_with_logits(
			logits, train_labels_node))

	# for the training, decrease by 10^(-.01) every time
	step = tf.Variable(0, dtype=tf.float32)
	learning_rate = tf.train.exponential_decay(
		initial_learning_rate,
		step,
		num_steps / 500,
		pow(10, -.01),
		staircase=True)
	optimizer = tf.train.AdamOptimizer(learning_rate).minimize(loss, global_step=step)

	# get predictions
	train_prediction = tf.nn.softmax(logits)
	eval_prediction = tf.nn.softmax(tf.nn.bias_add(tf.matmul(eval_data, weights), bias))

 	saver = tf.train.Saver()

	# Small utility function to evaluate a dataset by feeding batches of data to
	# {eval_data} and pulling the results from {eval_predictions}.
	# Saves memory and enables this to run on smaller GPUs.
	def eval_in_batches(data, sess):
		"""Get all predictions for a dataset by running it in small batches."""
		size = data.shape[0]
		if size < BATCH_SIZE:
			raise ValueError("batch size for evals larger than dataset: %d" % size)
		predictions = np.ndarray(shape=(size, NUM_LABELS), dtype=np.float32)
		for begin in xrange(0, size, BATCH_SIZE):
			end = begin + BATCH_SIZE
			if end <= size:
				predictions[begin:end, :] = sess.run(
					eval_prediction,
					feed_dict={eval_data: data[begin:end, ...]})
			else:
				batch_predictions = sess.run(
					eval_prediction,
					feed_dict={eval_data: data[-BATCH_SIZE:, ...]})
				predictions[begin:, :] = batch_predictions[begin - size:, :]
		return predictions

	saver = tf.train.Saver()

	# Create a local session to run the training.
	start_time = time.time()
	with tf.Session() as sess:
		# Run all the initializers to prepare the trainable parameters.
		tf.initialize_all_variables().run()
		#saver.restore(sess, "./trained_model/model.ckpt")
		#print('Initialized!')		
    	# Loop through training steps.
		for step in xrange(num_steps):
			# Compute the offset of the current minibatch in the data.
			# Note that we could use better randomization across epochs.
			offset = (step * BATCH_SIZE) % (train_size - BATCH_SIZE)
			batch_data = train_data[offset:(offset + BATCH_SIZE), ...]
			batch_labels = train_labels[offset:(offset + BATCH_SIZE)]
			# This dictionary maps the batch data (as a np array) to the
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
				save_path = saver.save(sess, "./linear_model2/model_{}.ckpt".format(step))
				print("Model saved in file: %s" % save_path)
		# Finally print the result!
		test_error = error_rate(eval_in_batches(test_data, sess), test_labels)
		print('Test error: %.1f%%' % test_error)


if __name__ == '__main__':
	main()
