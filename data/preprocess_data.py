from __future__ import print_function
import numpy as np
import sys
import json
import copy

def load_game_data():
    f = open("games.data", "r")
    game_dat = json.loads(f.read())
    f.close()
    return game_dat

def print_board(b):
    for i in range(9):
        print("".join([str(x) for x in b[i*9:i*9+9]]))

def extract_user_plane(board, user):
    return [1 if x == user else 0 for x in board]

def did_win(board, subgrid_x, subgrid_y, player):
    for i in range(3):
        if board[(subgrid_x)*9+subgrid_y] == player and \
           board[(subgrid_x+1)*9+subgrid_y] == player and \
           board[(subgrid_x+2)*9+subgrid_y] == player:
            return True
        if board[(subgrid_x)*9+subgrid_y] == player and \
           board[(subgrid_x)*9+subgrid_y+1] == player and \
           board[(subgrid_x)*9+subgrid_y+2] == player:
            return True
    diag1 = board[(subgrid_x)*9+subgrid_y] == player and \
            board[(subgrid_x+1)*9+subgrid_y+1] == player and \
            board[(subgrid_x+2)*9+subgrid_y+2] == player
    diag2 = board[(subgrid_x+2)*9+subgrid_y] == player and \
            board[(subgrid_x+1)*9+subgrid_y+1] == player and \
            board[(subgrid_x)*9+subgrid_y+2] == player
    return diag1 or diag2

def is_tie(board, subgrid_x, subgrid_y):
    for i in range(3):
        for j in range(3):
            if board[(subgrid_x+i) * 9 + (subgrid_y + j)] == 0:
                return False
    return True

def subgrid_is_finished(board, subgrid_x, subgrid_y):
    subgrid_x = subgrid_x * 3
    subgrid_y = subgrid_y * 3
    result = did_win(board, subgrid_x, subgrid_y, 1) or \
             did_win(board, subgrid_x, subgrid_y, 2) or \
             is_tie(board, subgrid_x, subgrid_y)
    return result

def extract_legality_plane(board, lastmove):
    if lastmove < 0:
        return [1] * 81
    valid_move_plane = []
    x, y = lastmove / 9, lastmove % 9
    subgrid_x, subgrid_y = (lastmove / 9) / 3, (lastmove % 9) / 3
    move_subgrid_x, move_subgrid_y = x % 3, y % 3
    if subgrid_is_finished(board, move_subgrid_x, move_subgrid_y):
        for i in range(9):
            for j in range(9):
                cur_subgrid_x, cur_subgrid_y = i/3, j/3
                if not subgrid_is_finished(board, cur_subgrid_x, cur_subgrid_y) and board[i*9+j] == 0:
                    valid_move_plane.append(1)
                else:
                    valid_move_plane.append(0)
    else:
        for i in range(9):
            for j in range(9):
                cur_subgrid_x, cur_subgrid_y = i/3, j/3
                if cur_subgrid_x == move_subgrid_x and cur_subgrid_y == move_subgrid_y and board[i*9+j] == 0:
                    valid_move_plane.append(1)
                else:
                    valid_move_plane.append(0)
    return valid_move_plane

def extract_move_made(cur, prev):
    diffs = [0 if cur[i] == prev[i] else i for i in range(len(cur))]
    return sum(diffs)

# Extract features for predicting moves (Note this is not for predicting which move will lead to a win).
# Note that the format in the input file should follow the rules:
# 0 - empty space
# 1 - firt player
# 2 - second player
def extract_features(single_game_data, index, previous_move_location=-1):
    # List of features
    # player/opponent/empty - 3 planes (note 1 goes first)
    # legality - 1 plane
    current_board_state = []
    if index == 0:
        current_board_state = [0] * 81
    else:
        current_board_state = [int(x) for x in single_game_data["states"][index-1]["field"].split(",")]
    next_board_state = [int(x) for x in single_game_data["states"][index]["field"].split(",")]
    n_ones = len([x for x in current_board_state if x == 1])
    n_twos = len([x for x in current_board_state if x == 2])
    player, opponent = -1, -1
    if (n_ones + n_twos) % 2 == 1:
        # player 2 to go
        player, opponent = 2, 1
    else:
        # player 1 to go
        player, opponent = 1, 2

    # Construct features
    player_plane = extract_user_plane(current_board_state, player)
    opponent_plane = extract_user_plane(current_board_state, opponent)
    free_space_plane = extract_user_plane(current_board_state, 0)
    legality_plane = extract_legality_plane(current_board_state, previous_move_location)

    # Put features in form [y, x, channel]
    player_plane_99 = np.reshape(player_plane, (9,9))
    opponent_plane_99 = np.reshape(opponent_plane, (9,9))
    free_space_plane_99 = np.reshape(free_space_plane, (9,9))
    legality_plane_99 = np.reshape(legality_plane, (9,9))
    all_features = [[[i for i in [player_plane_99[y][x], opponent_plane_99[y][x], free_space_plane_99[y][x], legality_plane_99[y][x]]] for x in range(9)] for y in range(9)]

    # Get label
    label = extract_move_made(current_board_state, next_board_state)
    label_99 = [[0 for i in range(9)] for j in range(9)]
    label_99[label/9][label%9] = 1

    # Is a winning move?
    player_name = "player%d" % player
    is_winning_move = single_game_data["settings"]["winnerplayer"] == player_name or \
                      single_game_data["settings"]["winnerplayer"] == "none"
    return all_features, label, label_99, is_winning_move

def rotate_position(position):
    dummy = [[position[i*9+j] for j in range(9)] for i in range(9)]
    rotated = zip(*dummy[::-1])
    return [x for sublist in rotated for x in sublist]

def rotate_position_n_times(game, n_times):
    result = game
    for i in range(n_times):
        result = rotate_position(result)
    return result

def rotate_game(game, n_times):
    game_copy = copy.deepcopy(game)
    for i, position in enumerate(game["states"]):
        new_position = rotate_position_n_times([int(x) for x in position["field"].split(",")], n_times)
        game_copy["states"][i]["field"] = ",".join([str(x) for x in new_position])
    return game_copy

def add_rotated_games(data):
    rotated_games = []
    for i, game in enumerate(data):
        if i % 500 == 0:
            print("%d games rotated of %d" % (i, len(data)))
        rotated_games.append(rotate_game(game, 1))
        rotated_games.append(rotate_game(game, 2))
        rotated_games.append(rotate_game(game, 3))
    return data + rotated_games

def swap(arr, x, y):
  t = arr[x]
  arr[x] = arr[y]
  arr[y] = t

def flip_game(game, hor_or_vert):
  game_copy = copy.deepcopy(game)
  for ii, position in enumerate(game["states"]):
    new_position = game["states"][ii]["field"]
    new_position = [int(x) for x in new_position.split(",")]
    l_i, l_j = 9, 9
    if hor_or_vert:
      l_i = 4
    else:
      l_j = 4
    for i in range(l_i):
      for j in range(l_j):
        if hor_or_vert:
          swap(new_position, i*9+j,(9-i-1)*9+j)
        else:
          swap(new_position, i*9+j, i*9+9-j-1)
    game_copy["states"][ii]["field"] = ",".join([str(x) for x in new_position])
  return game_copy

def add_flipped_games(data):
  flipped_games = []
  for i, game in enumerate(data):
    if i % 500 == 0:
      print("%d games flipped of %d" % (i, len(data)))
    flipped_games.append(flip_game(game, 0))
    flipped_games.append(flip_game(game, 1))
  return data + flipped_games

def count_distinct_positions(data):
    positions = set()
    for game in data:
        for game_position in game["states"]:
            positions.add(game_position["field"])
    return len(positions)

def save_preprocessed_data(data):
    f = open("preprocessed_games.data", "w")
    print(json.dumps(data), file=f)
    f.close()
    return

def preprocess():
    data = load_game_data()
    print("Number of raw games: %d" % len(data))
    print("Number of raw distinct positions: %d" % count_distinct_positions(data))
    print("Rotating games to increase data size...")
    data = add_flipped_games(data)
    data = add_rotated_games(data)
    print("Number of total games: %d" % len(data))
    print("Number of total distinct positions: %d" % count_distinct_positions(data))
    preprocessed_data = []
    for i, game in enumerate(data):
        if i % 5000 == 0:
            print("%d games processed..." % i)
            save_preprocessed_data(preprocessed_data)
        previous_move = -1
        for game_step in range(len(game["states"])):
            features, previous_move, label, does_win_or_draw = extract_features(game, game_step, previous_move)
            if does_win_or_draw:
                preprocessed_data.append((features, previous_move))
    save_preprocessed_data(preprocessed_data)

if __name__ == "__main__":
    preprocess()
