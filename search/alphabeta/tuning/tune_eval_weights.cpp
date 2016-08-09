#include <iostream>
#include <chrono>
#include <random>
#include "../headers/utils.h"
#include "../headers/alphabeta.h"

using namespace std;

#define PP_TIE 2
#define PP_WIN 1
#define PP_LOSE 0

#define POPULATION_SIZE 1000
#define EVALUATIONS_PER_EPOCH 100
#define N_EPOCHS 100
#define N_WEIGHTS 10
#define K_FACTOR 100
#define BASE_ELO_RATING 1400
#define KILL_RATIO .20

struct Individual {
    float weights[N_WEIGHTS];
    double elo_rating;
    int n_games_played;
};

typedef struct Individual Individual;

struct IndividualSort {
    bool operator() (const Individual &a, const Individual &b) {
	return a.elo_rating > b.elo_rating;
    };
};

double PlayValue(const Individual &a) {
    return 97 / (a.n_games_played+1) + 42 * (a.elo_rating/BASE_ELO_RATING);
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

int Play(Individual &a, Individual &b) {
    State s1, s2;
    InitializeWithWeights(s1, a.weights);
    InitializeWithWeights(s2, b.weights);
    int turn = 0;
    while (true) {
	Move first_player_move;
	iterative_deepening(s1, 100, &first_player_move, false);
	PerformMove(s1, first_player_move);
	PerformMove(s2, first_player_move);

	if (CheckEnd(s1) == PLAYER_1) return PP_WIN;
	if (CheckEnd(s1) == PLAYER_2) return PP_LOSE;
	if (CheckEnd(s1) == TIE) return PP_TIE;

	Move second_player_move;
	iterative_deepening(s2, 100, &second_player_move, false);
	PerformMove(s1, second_player_move);
	PerformMove(s2, second_player_move);

	if (CheckEnd(s1) == PLAYER_1) return PP_WIN;
	if (CheckEnd(s1) == PLAYER_2) return PP_LOSE;
	if (CheckEnd(s1) == TIE) return PP_TIE;
    }
}

void UpdateEloScore(Individual &first, double self_rating, double other_rating, int did_win) {
    double score = 0;
    if (did_win == PP_WIN) score = 1;
    if (did_win == PP_LOSE) score = 0;
    if (did_win == PP_TIE) score = .5;
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
    result += "}\n";
    return result;
}

void PlayIndividuals(Individual &first, Individual &second) {
    printf("----------------------------------------\n");
    printf("Playing:\n%s%s", IndividualString(first).c_str(), IndividualString(second).c_str());
    int result = Play(first, second);
    double rating1 = first.elo_rating;
    double rating2 = second.elo_rating;
    if (result == PP_WIN) {
	UpdateEloScore(first, rating1, rating2, PP_WIN);
	UpdateEloScore(second, rating2, rating1, PP_LOSE);
    }
    else {
	UpdateEloScore(second, rating2, rating1, PP_WIN);
	UpdateEloScore(first, rating1, rating2, PP_LOSE);
    }
    first.n_games_played++;
    second.n_games_played++;
    if (result == PP_WIN || result == PP_LOSE) {
	cout << "Winner:" << endl;
	if (result == PP_WIN) {
	    cout << IndividualString(first) << endl;
	}
	else {
	    cout << IndividualString(second) << endl;
	}
    }
    else {
	cout << "Tie" << endl;;
    }
    printf("----------------------------------------\n");
}

string PrintTopIndividuals(vector<Individual> &v) {
    string result = "Top individuals: \n";
    for (int i = 0; i < min((int)v.size(), 10); i++) {
	result += IndividualString(v[i]);
    }
    return result;
}

double fRand(double fMin, double fMax) {
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}

int SelectIndividualToPlay(vector<Individual> &individuals) {
    double total_play_score = 0;
    for (const auto &individual : individuals) {
	total_play_score += PlayValue(individual);
    }
    double random_value = fRand(0, total_play_score);
    double cur_value = 0;
    for (int i = 0; i < individuals.size(); i++) {
	if (cur_value >= random_value) return i;
	cur_value += PlayValue(individuals[i]);
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
    for (int i = 0; i < v.size(); i++) {
	if (cur >= total_elo_score) return i;
	cur += v[i].elo_rating;
    }
    return v.size()-1;
}

Individual Reproduce(Individual &a, Individual &b) {
    Individual new_individual;
    for (int i = 0; i < N_WEIGHTS; i++) {
	int choice = rand() % 4;
	float w1 = a.weights[i],  w2 = b.weights[i];
	int new_weight = 0;
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
    new_individual.elo_rating = (a.elo_rating + b.elo_rating) / 2;
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
	string individuals_strings = PrintTopIndividuals(individuals);
	cout << individuals_strings;
	for (int iter = 0; iter < EVALUATIONS_PER_EPOCH; iter++) {
	    int i, j = -1;
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
