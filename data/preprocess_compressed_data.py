# does the same thing as preprocess_data.py but faster and saves to np array
# from the info in compressed_games.data:
# no augmented data, takes ~110 seconds (~45 games/s)-> 85MB output file
# with augmented data, takes ~150 seconds (~33 games/s) -> 680MB output file
import json
import numpy as np
import random
from scipy.io import savemat
import time

input_file_name = 'compressed_games.data'
output_file_name = 'processed_games.mat'
# apply transposition and reflection to augment data by factor of 8
augment_data = True
# whether or not to shuffle the data
shuffle_data = True
BOARD_DIM = 9
NUM_FEATURES = 8

# finds out which subboards is it possible to move in.
# returns 3x3 np array of bools
def get_open_boards(board):
	result = np.ones((3, 3), dtype=np.bool)
	for board_y in range(3):
		for board_x in range(3):
			sub_board = board[board_y*3: (board_y+1)*3, board_x*3: (board_x+1)*3]
			# if all spaces are filled, can't move there
			filled = sub_board != 0
			if np.sum(filled) == 9:
				result[board_y, board_x] = False
			# check for three in a rows, can't move there
			for line in [sub_board[0], sub_board[1], sub_board[2],
					sub_board[:, 0], sub_board[:, 1], sub_board[:, 2],
					np.array([sub_board[0,0], sub_board[1,1], sub_board[2,2]]),
					np.array([sub_board[0,2], sub_board[1,1], sub_board[2,0]])]:
				if line[0] != 0 and line[0] == line[1] and line[1] == line[2]:
					result[board_y, board_x] = False
					break
	return result

# gets the legal feature plane for a board
def get_legal_plane(board, last_move):
	# get all possible places to move
	open_boards = get_open_boards(board)

	# if first move, last_move = None, and can move anywhere in open_boards
	if last_move is not None:
		board_sent_to = (last_move[0]%3, last_move[1]%3)
		# can only move in the board sent to, overwrite open_boards.
		# else can move into any open board
		if open_boards[board_sent_to]:
			open_boards = np.zeros((3, 3), dtype=np.bool)
			open_boards[board_sent_to] = True

	# can move in the union of open spaces and open boards
	legal_moves = board == 0
	for board_y in range(3):
		for board_x in range(3):
			if not open_boards[board_y, board_x]:
				legal_moves[board_y*3: (board_y+1)*3, board_x*3: (board_x+1)*3] = False

	return legal_moves

# gets the layers for open three-in-a-rows and one move away from a three-in-a-row
def get_rows(board):
	open_boards = get_open_boards(board)

	ones_open = np.ones((9, 9), dtype=np.bool)
	twos_open = np.ones((9, 9), dtype=np.bool)
	ones_almost = np.zeros((9, 9), dtype=np.bool)
	twos_almost = np.zeros((9, 9), dtype=np.bool)

	for board_y in range(3):
		for board_x in range(3):
			sub_board = (board_y * 3, board_x * 3)
			# for each line
			for line in [[(0, 0), (0, 1), (0, 2)], [(1, 0), (1, 1), (1, 2)], [(2, 0), (2, 1), (2, 2)],
						 [(0, 0), (1, 0), (2, 0)], [(0, 1), (1, 1), (2, 1)], [(0, 2), (1, 2), (2, 2)],
						 [(0, 0), (1, 1), (2, 2)], [(0, 2), (1, 1), (2, 0)]]:
				# count number of ones, twos seen
				ones_count = 0
				twos_count = 0
				for spot in line:
					el = board[sub_board[0] + spot[0], sub_board[1] + spot[1]]
					if el == 1:
						ones_count += 1
					elif el == 2:
						twos_count += 1

				# Do the line stuff
				for spot in line:
					if ones_count > 0:
						twos_open[sub_board[0] + spot[0], sub_board[1] + spot[1]] = False
					if twos_count > 0:
						ones_open[sub_board[0] + spot[0], sub_board[1] + spot[1]] = False
					if ones_count == 2 and twos_count == 0:
						el = board[sub_board[0] + spot[0], sub_board[1] + spot[1]]
						if el == 0:
							ones_almost[sub_board[0] + spot[0], sub_board[1] + spot[1]] = True
					if twos_count == 2 and ones_count == 0:
						el = board[sub_board[0] + spot[0], sub_board[1] + spot[1]]
						if el == 0:
							twos_almost[sub_board[0] + spot[0], sub_board[1] + spot[1]] = True

	# should have all of them be false if the board is not open
	for board_y in range(3):
		for board_x in range(3):
			if not open_boards[board_y, board_x]:
				for feature_board in [ones_open, twos_open, ones_almost, twos_almost]:
					feature_board[board_y*3: (board_y+1)*3, board_x*3: (board_x+1)*3] = False
	return ones_open, twos_open, ones_almost, twos_almost

# gets the spots where it is possible to send opponent to specific board
def get_open_board(board):
	open_boards = get_open_boards(board)
	send_to_board = np.zeros((9, 9), dtype=np.bool)
	for board_y in range(3):
		for board_x in range(3):
			send_to_board[board_y*3: (board_y+1)*3, board_x*3: (board_x+1)*3] = open_boards
	return send_to_board

# (y, x) -> (x, y)
def transpose_move(move):
	return (move[1], move[0])

# (y, x) -> (y, 8-x)
def reverse_rows_move(move):
	return (move[0], 8-move[1])

# extracts features, labels, and the legal moves plane into numpy arrays.
# feature planes are (total of 8):
# 	Plane of 0s, 1s, 2s (3 planes)
# 	Plane of possible response moves (1 plane)
#	Plane where it is possible to complete a row of 3 for 1s, 2s (2 planes)
#	Plane where if moved here can complete a row of 3 for 1s, 2s (2 planes)
#	Plane where this will send the opponent to a specific board vs. open board (1 plane)

def extract_data(data, num_moves):
	# whether or not to augment training data
	if(augment_data):
		num_moves *= 8
	# pre-alloc memory, makes it run much faster
	features = np.zeros((num_moves, NUM_FEATURES, BOARD_DIM, BOARD_DIM), dtype=np.bool)
	labels = np.zeros((num_moves, 2), dtype=np.uint8)
	legal_moves = np.zeros((num_moves, BOARD_DIM, BOARD_DIM), dtype=np.bool)

	# which index to write features and labels to
	move_index = 0
	for game in data:
		p1_lost = game['settings']['winnerplayer'] == game['settings']['players']['names'][1]
		p2_lost = game['settings']['winnerplayer'] == game['settings']['players']['names'][0]

		board = np.array(game['states']['board']).reshape(BOARD_DIM, BOARD_DIM)
		history = np.array(game['states']['move_history']).reshape(BOARD_DIM, BOARD_DIM)
		moves_in_game = np.max(history)

		# for each move, find the state of the board one move before it and extract features
		for i in range(moves_in_game):
			# if player lost, do not include them in data set
			if i % 2 == 0 and p1_lost:
				continue
			elif i % 2 == 1 and p2_lost:
				continue

			history_filter = history <= i
			cur_board = np.zeros((BOARD_DIM, BOARD_DIM), dtype=np.uint8)
			last_move = None
			cur_move = None
			for y in range(BOARD_DIM):
				for x in range(BOARD_DIM):
					if history_filter[y, x]:
						cur_board[y, x] = board[y, x]
					if history[y, x] == i and i != 0:
						last_move = (y, x)
					elif history[y, x] == i+1:
						cur_move = (y, x)

			zeros_layer = cur_board == 0
			ones_layer = cur_board == 1
			twos_layer = cur_board == 2
			legal_moves_layer = get_legal_plane(cur_board, last_move)
			ones_open, twos_open, ones_almost, twos_almost = get_rows(cur_board)
			send_to_board = get_open_board(cur_board)

			if i % 2 == 0:
				features_data = np.array([zeros_layer, ones_layer, twos_layer,
					ones_open, twos_open, ones_almost, twos_almost, send_to_board])
			else:
				features_data = np.array([zeros_layer, twos_layer, ones_layer,
					twos_open, ones_open, twos_almost, ones_almost, send_to_board])

			if augment_data:
				for i in range(4):
					# transpose
					for j in range(features_data.shape[0]):
						features_data[j] = features_data[j].T
					features[move_index] = features_data
					cur_move = transpose_move(cur_move)
					labels[move_index] = cur_move
					legal_moves_layer = legal_moves_layer.T
					legal_moves[move_index] = legal_moves_layer
					move_index += 1

					# reverse rows
					features_data = features_data[:, :, ::-1]
					features[move_index] = features_data
					cur_move = reverse_rows_move(cur_move)
					labels[move_index] = cur_move
					legal_moves_layer = legal_moves_layer[::-1]
					legal_moves[move_index] = legal_moves_layer
					move_index += 1
			else:
				features[move_index] = features_data
				labels[move_index] = cur_move
				legal_moves[move_index] = legal_moves_layer
				move_index += 1
			if move_index % 10000 == 0:
				print move_index
	return features, labels, legal_moves

def shuffle(features, labels, legal_moves):
	shuffled_features = np.zeros(features.shape, features.dtype)
	shuffled_labels = np.zeros(labels.shape, labels.dtype)
	shuffled_legal_moves = np.zeros(legal_moves.shape, legal_moves.dtype)
	indices = [i for i in range(features.shape[0])]
	random.shuffle(indices)

	for i, index in enumerate(indices):
		shuffled_features[i] = features[index]
		shuffled_labels[i] = labels[index]
		shuffled_legal_moves[i] = legal_moves[index]

	return shuffled_features, shuffled_labels, shuffled_legal_moves

def save_data(features, labels, legal_moves):
	file_dict = {'features': features, 'labels': labels, 'legal_moves': legal_moves}
	savemat(output_file_name, file_dict)

def main():
	t0 = time.time()
	data = json.loads(open(input_file_name, 'r').read())
	print 'loaded data: %fs' % (time.time() - t0)
	num_moves = [max(game['states']['move_history']) for game in data]
	# filter out people who didn't lose
	for i, (moves, game) in enumerate(zip(num_moves, data)):
		# no winner
		if game['settings']['winnerplayer'] == 'none':
			continue
		# the winning player went first and odd number of moves
		elif (game['settings']['winnerplayer'] == game['settings']['players']['names'][0] and
			moves % 2 == 1):
			num_moves[i] = (moves / 2) + 1
		else:
			num_moves[i] = moves / 2
	num_moves = sum(num_moves)
	print 'games found: %s\nnum_moves: %s' % (len(data), num_moves)

	features, labels, legal_moves = extract_data(data, num_moves)
	print 'extracted data: %fs' % (time.time() - t0)
	if shuffle_data:
		features, labels, legal_moves = shuffle(features, labels, legal_moves)
		print 'shuffled data: %fs' % (time.time() - t0)
	save_data(features, labels, legal_moves)
	print 'saved data: %fs' % (time.time() - t0)

if __name__ == '__main__':
	main()