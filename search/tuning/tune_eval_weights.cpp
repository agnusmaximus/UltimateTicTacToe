#include <iostream>
#include <chrono>
#include <random>
#include <math.h>
#include <time.h>
#include "../alphabeta/headers/utils.h"
#include "../alphabeta/headers/alphabeta.h"

using namespace std;

#define PP_TIE 2
#define PP_WIN 1
#define PP_LOSE 0

#define POPULATION_SIZE 1000
#define EVALUATIONS_PER_EPOCH 100
#define N_EPOCHS 1000000
#define N_WEIGHTS 13
#define K_FACTOR 100
#define BASE_ELO_RATING 1400
#define KILL_RATIO .20

#define TUNE_DEPTH 30

struct Individual {
    float weights[N_WEIGHTS];
    double elo_rating;
    int n_games_played;
    int won_as_first, won_as_second, ties;
    double generation;
};

typedef struct Individual Individual;

struct IndividualSort {
    bool operator() (const Individual &a, const Individual &b) {
	return a.elo_rating > b.elo_rating;
    };
};

double PlayValue(const Individual &a) {
    return 420 * (a.elo_rating/BASE_ELO_RATING) + 97/(double)(a.n_games_played+1);
}

struct WillingnessToPlaySort {
    bool operator() (const Individual &a, const Individual &b) {
	return PlayValue(a) > PlayValue(b);
    }
};

std::default_random_engine generator;
std::normal_distribution<double> distribution(1.0,1.5);
std::normal_distribution<double> mutation(0,.1);

Individual CreateRandomIndividual() {
    Individual individual;
    for (int i = 0; i < N_WEIGHTS; i++) {
	individual.weights[i] = distribution(generator);
    }
    individual.elo_rating = BASE_ELO_RATING;
    individual.n_games_played = 0;
    individual.generation = 0;
    individual.won_as_first = 0;
    individual.won_as_second = 0;
    individual.ties = 0;
    return individual;
}

vector<Individual> CreateRandomPopulation() {
    vector<Individual> result;
    for (int i = 0; i < POPULATION_SIZE; i++) {
	result.push_back(CreateRandomIndividual());
    }
    return result;
}

char CheckEnd(State &s, bool verbose=true) {
    if (DidWinGame(s, PLAYER_1)) {
	return PLAYER_1;
    }
    if (DidWinGame(s, PLAYER_2)) {
	return PLAYER_2;
    }
    if (IsFilled(s.results_board.data(), 0, 0, BOARD_DIM/3)) {
	return TIE;
    }
    return EMPTY;
}

double Play(Individual &a, Individual &b, string &r_string) {
    double p1_score = 0;

    State s1, s2;
    InitializeWithWeights(s1, a.weights);
    InitializeWithWeights(s2, b.weights);
    int r1_result;
    while (true) {
	Move first_player_move;
	iterative_deepening(s1, TUNE_DEPTH, &first_player_move, false);
	PerformMove(s1, first_player_move);
	PerformMove(s2, first_player_move);

	r1_result = CheckEnd(s1);
	if (r1_result == PLAYER_1 ||
	    r1_result == PLAYER_2 ||
	    r1_result == TIE) break;

	Move second_player_move;
	iterative_deepening(s2, TUNE_DEPTH, &second_player_move, false);
	PerformMove(s1, second_player_move);
	PerformMove(s2, second_player_move);

	r1_result = CheckEnd(s1);
	if (r1_result == PLAYER_1 ||
	    r1_result == PLAYER_2 ||
	    r1_result == TIE) break;
    }

    if (r1_result == PLAYER_1) {
	r_string += "P1,";
	a.won_as_first++;
    }
    else if (r1_result == TIE) {
	r_string += "TIE,";
	a.ties++;
	b.ties++;
    }
    else if (r1_result == PLAYER_2) {
	r_string += "P2,";
	b.won_as_second++;
    }
    else {
	cout << "ERROR: invalid game result" << endl;
	exit(0);
    }

    int r2_result;
    InitializeWithWeights(s1, a.weights);
    InitializeWithWeights(s2, b.weights);
    while (true) {
	Move first_player_move;
	iterative_deepening(s2, TUNE_DEPTH, &first_player_move, false);
	PerformMove(s1, first_player_move);
	PerformMove(s2, first_player_move);

	r2_result = CheckEnd(s1);
	if (r2_result == PLAYER_1 ||
	    r2_result == PLAYER_2 ||
	    r2_result == TIE) break;

	Move second_player_move;
	iterative_deepening(s1, TUNE_DEPTH, &second_player_move, false);
	PerformMove(s1, second_player_move);
	PerformMove(s2, second_player_move);

	r2_result = CheckEnd(s1);
	if (r2_result == PLAYER_1 ||
	    r2_result == PLAYER_2 ||
	    r2_result == TIE) break;
    }

    if (r2_result == PLAYER_1) {
	r_string += "P2";
	b.won_as_first++;
    }
    else if (r2_result == TIE) {
	r_string += "TIE";
	b.ties++;
	a.ties++;
    }
    else if (r2_result == PLAYER_2) {
	r_string += "P1";
	a.won_as_second++;
    }
    else {
	cout << "ERROR: invalid game result for game 2" << endl;
	exit(0);
    }

    // WW -> 1
    // WL or LW -> .5
    // WT -> .90
    if (r1_result==PLAYER_1 && r2_result==PLAYER_2)
	p1_score = 1;
    if (r1_result==PLAYER_2 && r2_result==PLAYER_1)
	p1_score = 0;
    if ((r1_result==PLAYER_2 && r2_result==PLAYER_2) ||
	(r1_result==PLAYER_1 && r2_result==PLAYER_1))
	p1_score = .5;
    if ((r1_result==PLAYER_1 && r2_result==TIE) ||
	(r1_result==TIE && r2_result==PLAYER_2))
	p1_score = .9;
    if ((r1_result==PLAYER_2 && r2_result==TIE) ||
	(r1_result==TIE && r2_result==PLAYER_1))
	p1_score = .1;
    if (r1_result==TIE && r2_result==TIE)
	p1_score = .5;
    cout << "Score: " << p1_score << endl;
    return p1_score;
}

void UpdateEloScore(Individual &first, double self_rating, double other_rating, double score) {
    double expected = 1 / (pow(10, -1 * (self_rating-other_rating)/400)+1);
    double rn = self_rating + K_FACTOR * (score - expected);
    first.elo_rating = rn;
}

string IndividualString(const Individual &i) {
    string result = to_string(i.elo_rating) + " {";
    for (int k = 0; k < N_WEIGHTS; k++) {
	if (k != 0)
	    result += ", ";
	result += to_string(i.weights[k]);
    }
    result += "} n_games: " + to_string(i.n_games_played) +
	", won_as_first: " + to_string(i.won_as_first) +
	", won as second: " + to_string(i.won_as_second) +
	", n_ties: " + to_string(i.ties) +
	", generation: " + to_string(i.generation) + "\n";
    return result;
}

void PlayIndividuals(Individual &first, Individual &second) {
    printf("----------------------------------------\n");
    printf("Playing:\n%s%s", IndividualString(first).c_str(), IndividualString(second).c_str());
    string result_string;
    double result = Play(first, second, result_string);
    double rating1 = first.elo_rating;
    double rating2 = second.elo_rating;
    UpdateEloScore(first, rating1, rating2, result);
    UpdateEloScore(second, rating2, rating1, 1-result);
    cout << "Result: " << result_string << endl;
    first.n_games_played++;
    second.n_games_played++;
    printf("New scores:\n%s%s", IndividualString(first).c_str(), IndividualString(second).c_str());
}

string PrintTopIndividuals(vector<Individual> &v) {
    string result = "Top individuals: \n";
    for (int i = 0; i < min((int)v.size(), 10); i++) {
	result += IndividualString(v[i]);
    }
    return result;
}

double fRand(double fMin, double fMax) {
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist(fMin, fMax);
    rng.seed(std::random_device{}());
    return dist(rng);
}

int SelectIndividualToPlay(vector<Individual> &individuals) {
    double total_play_score = 0;
    for (const auto &individual : individuals) {
	total_play_score += pow(PlayValue(individual), 3);
    }
    double random_value = fRand(0, total_play_score);
    double cur_value = 0;
    for (int i = 0; i < individuals.size(); i++) {
	if (cur_value + pow(PlayValue(individuals[i]), 3) >= random_value) return i;
	cur_value += pow(PlayValue(individuals[i]), 3);
    }
    return individuals.size()-1;
}

vector<Individual> UnfitIndividualsDie(vector<Individual> &v) {
    vector<Individual> new_pop;
    int n_to_keep = (int)((double)v.size() * (double)(1-KILL_RATIO));
    sort(v.begin(), v.end(), IndividualSort());
    for (int i = 0; i < n_to_keep; i++) {
	new_pop.push_back(v[i]);
    }
    return new_pop;
}

int ProbSelectOnElo(vector<Individual> &v, double total_elo_score) {
    double cur = 0;
    double random_elo_score = fRand(0, total_elo_score);
    for (int i = 0; i < v.size(); i++) {
	if (cur + v[i].elo_rating >= random_elo_score) return i;
	cur += v[i].elo_rating;
    }
    return v.size()-1;
}

Individual Reproduce(Individual &a, Individual &b) {
    Individual new_individual;
    for (int i = 0; i < N_WEIGHTS; i++) {
	int choice = rand() % 4;
	float w1 = a.weights[i],  w2 = b.weights[i];
	float new_weight = 0;
	if (choice == 0) {
	    new_weight = max(w1, w2);
	}
	else if (choice == 1) {
	    new_weight = min(w1, w2);
	}
	else if (choice == 2 || choice == 3) {
	    new_weight = (w1 + w2)/2;
	}
	new_weight += mutation(generator);
	new_individual.weights[i] = new_weight;
    }
    //new_individual.elo_rating = (a.elo_rating + b.elo_rating) / 2;
    new_individual.elo_rating = min(a.elo_rating, b.elo_rating) * (double)3 / 4 + max(a.elo_rating, b.elo_rating)/(double)4;
    new_individual.n_games_played = 0;
    new_individual.generation = max(a.generation, b.generation)+1;
    new_individual.won_as_first = 0;
    new_individual.won_as_second = 0;
    new_individual.ties = 0;
    return new_individual;
}

void FitIndividualsReproduce(vector<Individual> &v) {
    int n_individuals_to_create = (POPULATION_SIZE - v.size()) * 3 / 4;
    double total_elo_score = 0;
    for (const auto &i : v) {
	total_elo_score += i.elo_rating;
    }
    vector<Individual> news;
    for (int i = 0; i < n_individuals_to_create; i++) {
	int first = ProbSelectOnElo(v, total_elo_score);
	int second = ProbSelectOnElo(v, total_elo_score);
	Individual new_individual = Reproduce(v[first], v[second]);
	news.push_back(new_individual);
    }
    for (int i = 0; i < news.size(); i++) {
	v.push_back(news[i]);
    }
}

void IntroduceNewIndividuals(vector<Individual> &v) {
    int n_individuals_to_create = POPULATION_SIZE - v.size();
    for (int i = 0; i < n_individuals_to_create; i++) {
	v.push_back(CreateRandomIndividual());
    }
}

int main(void) {
    vector<Individual> individuals = CreateRandomPopulation();
    for (int epoch = 0; epoch < N_EPOCHS; epoch++) {
	cout << individuals.size() << endl;
	sort(individuals.begin(), individuals.end(), IndividualSort());
	if (individuals.size() < 10) break;
	string individuals_strings = PrintTopIndividuals(individuals);
	cout << individuals_strings;
	for (int iter = 0; iter < EVALUATIONS_PER_EPOCH; iter++) {
	    int i = -1, j = -1;
	    i = SelectIndividualToPlay(individuals);
	    while (j==-1 || i == j) {
		j = SelectIndividualToPlay(individuals);
	    }
	    PlayIndividuals(individuals[i], individuals[j]);
	}
	individuals = UnfitIndividualsDie(individuals);
	FitIndividualsReproduce(individuals);
	IntroduceNewIndividuals(individuals);
    }
}
