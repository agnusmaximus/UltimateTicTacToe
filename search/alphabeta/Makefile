BOT=0
PLAY_DEBUG=2
DEBUG_RUN=1
PLAY_RANDOM=3
PLAY_RANDOM_MANY=4
TEST=5
BENCHMARK=6
PIT_WEIGHTS=7

benchmark:
	rm -f benchmark_run
	g++ -Ofast --std=c++11 Bot.cpp -DMETHOD=BENCHMARK -DVERBOSE=1 -o benchmark_run
	./benchmark_run
test:
	rm -f test_bin
	g++ -Ofast --std=c++11 Bot.cpp -DMETHOD=RUN_TESTS -DVERBOSE=1 -o test_bin
	./test_bin
playself:
	rm -f playself_run
	g++ -O3 --std=c++11 Bot.cpp -DMETHOD=PLAY_DEBUG -o playself_run
	./playself_run

playrandommany:
	rm -f playrandommany_run
	g++ -Ofast --std=c++11 Bot.cpp -DMETHOD=PLAY_RANDOM_MANY -o playrandommany_run
	./playrandommany_run
playrandom:
	rm -f playrandom_run
	g++ -Ofast --std=c++11 Bot.cpp -DMETHOD=PLAY_RANDOM -o playrandom_run
	./playrandom_run
debug:
	rm -f debug_run
	g++ -Ofast --std=c++11 Bot.cpp -DMETHOD=DEBUG_RUN -o debug_run
	./debug_run
profile:
	rm -f profiling
	rm -f callgrind.out.*
	g++ -g -Ofast --std=c++11 Bot.cpp -DMETHOD=DEBUG_RUN -o profiling
	valgrind --tool=callgrind ./profiling
	callgrind_annotate callgrind.out.*
memcheck:
	rm -f memcheck_bin
	g++ -Ofast --std=c++11 Bot.cpp -DMETHOD=PLAY_DEBUG -DVERBOSE=1 -o memcheck_bin
	valgrind --leak-check=full --track-origins=yes ./memcheck_bin
bot:
	rm -f bot_bin
	g++ -Ofast --std=c++11 Bot.cpp -DMETHOD=BOT -o bot_bin

tune_features:
	rm -f tune_features_bin
	g++ -Ofast --std=c++11 ../tuning/tune_eval_weights.cpp -o tune_features_bin
	./tune_features_bin
pit_weights:
	rm -f pit_weights_bin
	g++ -Ofast --std=c++11 Bot.cpp -DMETHOD=PIT_WEIGHTS -DVERBOSE=1 -o pit_weights_bin
	./pit_weights_bin
clean:
	rm -f pit_weights_bin*
	rm -rf memcheck_bin*
	rm -rf *_bin*
	rm -rf playself_run
	rm -rf profiling*
	rm -rf *_run*
	rm -rf callgrind*
	rm -rf test_benchmark
	rm -rf *output
	rm -rf tune_features_bin
