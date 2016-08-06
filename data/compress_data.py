import json

file_name = 'games.data'

def compress_data(file_name):
	data = json.loads(open(file_name, 'r').read())
	compressed_data = []
	for game_object in data:
		game = game_object['states']

		move_history = [0] * 81
		prev_board = [0] * 81
		move_index = 1
		for move in game:
			cur_board = [int(el) for el in move['field'].split(',')]
			difference = [el[0] != el[1] for el in zip(cur_board, prev_board)]

			if True in difference:
				index = difference.index(True)
				move_history[index] = move_index
				move_index += 1

			prev_board = cur_board
		game_object['states'] = {'move_history': move_history, 'board': cur_board}
	compressed_data = json.dumps(data)
	out_file = open('%s.compressed' % file_name, 'w')
	out_file.write(compressed_data)
	out_file.close()

def main():
	compress_data(file_name)

if __name__ == '__main__':
	main()