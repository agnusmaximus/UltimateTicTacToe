# Crawl Ultimate Tic Tac Toe site for game data.
# Leaderboard: http://theaigames.com/competitions/ultimate-tic-tac-toe/leaderboard/global/a/

from __future__ import print_function
import json
import re
import sys
import copy
import urllib2

PRINT_STATS_EVERY_K = 10
N_PLAYERS_TO_CRAWL = 10
N_PAGES_TO_CRAWL_PER_PLAYER = 10

def gather_top_players(n_players):
    leaderboard_url = "http://theaigames.com/competitions/ultimate-tic-tac-toe/leaderboard/global/a/"
    response = urllib2.urlopen(leaderboard_url)
    html = response.read()
    name_indicator = 'class="bot-name">'
    is_bot_name = False
    top_players = []
    for line in html.split():
        if len(top_players) >= n_players:
            break
        if is_bot_name:
            top_players.append(line.strip())
            is_bot_name = False
        if line.find(name_indicator) != -1:
            is_bot_name = True
    return top_players

def top_player_page(player_name, page):
    return "http://theaigames.com/competitions/ultimate-tic-tac-toe/game-log/" + player_name + "/" + str(page)

def gather_game_links_from_player_name(player_name):
    all_game_links = []
    for page in range(1, N_PAGES_TO_CRAWL_PER_PLAYER+1):
        player_page = top_player_page(player_name, page)
        response = urllib2.urlopen(player_page)
        html = response.read()
        game_link_matcher = '"(http://theaigames.com/competitions/ultimate-tic-tac-toe/games/[a-zA-Z0-9]*)"'
        regex = re.compile(game_link_matcher)
        game_links = regex.findall(html)
        if len(game_links) == 0:
            break
        all_game_links += game_links
    return all_game_links

def get_data_from_game_link(game_link):
    game_link += "/data"
    data = json.loads(urllib2.urlopen(game_link).read())
    return data

def clean_game_data(game_data):
    game_data_copy = copy.deepcopy(game_data)
    game_data_copy["states"] = []
    for i, state in enumerate(game_data["states"]):
        field = [int(x) for x in state["field"].split(",")]
        zeros = [x for x in field if x == 0]
        ones = [x for x in field if x == 1]
        twos = [x for x in field if x == 2]
        if len(zeros) + len(ones) + len(twos) == len(field):
            game_data_copy["states"].append(state)
    return game_data_copy

def save_data(all_game_data):
    f = open("games.data", "w")
    all_game_data_string = json.dumps(all_game_data)
    print("%s" % all_game_data_string, file=f)
    f.close()

def add_game_data_positions_to_set(game_data, position_set):
    for state in game_data["states"]:
        position = state["field"]
        position_set.add(position)

def crawl_data():
    distinct_positions_set = set()
    all_game_data = []
    players = gather_top_players(N_PLAYERS_TO_CRAWL)
    iteration = 0
    for player_name in players:
        game_links = gather_game_links_from_player_name(player_name)
        for game_link in game_links:
            if iteration % PRINT_STATS_EVERY_K == 0:
                print("Number of games recorded: %d" % len(all_game_data))
                print("Number of distinct positions recorded: %d" % len(distinct_positions_set))
                save_data(all_game_data)
            cleaned_data = clean_game_data(get_data_from_game_link(game_link))
            add_game_data_positions_to_set(cleaned_data, distinct_positions_set)
            all_game_data.append(cleaned_data)
            iteration += 1
    save_data(all_game_data)

if __name__ == "__main__":
    crawl_data()
