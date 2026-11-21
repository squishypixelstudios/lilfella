G=../src/game
SRC=$(addprefix $(G)/,$(shell cat sources.txt))
sim: sim_main.cpp report.inc v010.inc $(SRC) $(G)/*.h
	g++ -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter -fsanitize=address,undefined -o $@ sim_main.cpp $(SRC)

# optimised, no sanitizers: for the long balance runs
simfast: sim_main.cpp report.inc v010.inc $(SRC) $(G)/*.h
	g++ -std=c++17 -O2 -o $@ sim_main.cpp $(SRC)
