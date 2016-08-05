debug:
	rm -f debug_run
	g++ -Ofast --std=c++11 alphabeta.cpp -o debug_run
profile:
	rm -f profiling
	rm -f callgrind.out.*
	g++ -Ofast --std=c++11 alphabeta.cpp -o profiling
	valgrind --tool=callgrind ./profiling
	callgrind_annotate callgrind.out.*

memcheck:
	rm -f memcheck_bin
	g++ -Ofast --std=c++11 alphabeta.cpp -o memcheck_bin
	valgrind --leak-check=full --track-origins=yes ./memcheck_bin
