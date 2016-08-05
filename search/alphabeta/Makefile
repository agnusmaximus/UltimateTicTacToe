debug:
	rm -f debug_run
	g++ -Ofast --std=c++11 debug_run.cpp -o debug_run
	./debug_run
profile:
	rm -f profiling
	rm -f callgrind.out.*
	g++ -Ofast --std=c++11 debug_run.cpp -o profiling
	valgrind --tool=callgrind ./profiling
	callgrind_annotate callgrind.out.*
memcheck:
	rm -f memcheck_bin
	g++ -Ofast --std=c++11 debug_run.cpp -o memcheck_bin
	valgrind --leak-check=full --track-origins=yes ./memcheck_bin
bot:
	rm -f bot_bin
	g++ -Ofast --std=c++11 Bot.cpp -o bot_bin
