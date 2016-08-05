all:
	g++ -Ofast --std=c++11 alphabeta.cpp
bot:
	g++ -Ofast --std=c++11 TicTacToeUltimate_cpp_starter.cpp alphabeta.cpp headers/utils.cpp && \
	zip -r Bot.zip TicTacToeUltimate_cpp_starter.cpp alphabeta.cpp alphabeta.h \
		headers/utils.h headers/utils.cpp headers/transposition_table.h headers/transposition_table
