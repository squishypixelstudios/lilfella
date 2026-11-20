G=../src/game
SRC=$(G)/lcd.cpp $(G)/sprites_data.cpp $(G)/breeds.cpp $(G)/pet.cpp $(G)/save.cpp $(G)/platform.cpp $(G)/motion.cpp $(G)/game.cpp $(G)/view.cpp $(G)/pages.cpp $(G)/eatfx.cpp $(G)/yeet.cpp $(G)/synth.cpp
sim: sim_main.cpp report.inc $(SRC) $(G)/*.h
	g++ -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter -fsanitize=address,undefined -o $@ sim_main.cpp $(SRC)

# optimised, no sanitizers: for the long balance runs
simfast: sim_main.cpp report.inc $(SRC) $(G)/*.h
	g++ -std=c++17 -O2 -o $@ sim_main.cpp $(SRC)
