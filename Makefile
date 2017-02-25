
CXX = g++
HEADER = math.h world.h graph.h shaders.h
SOURCE = math.cpp world.cpp graph.cpp main.cpp
SHADER = food.vert food.frag
FLAGS = -std=c++11 -Wall -Wno-parentheses -DBUILTIN
D_FLAGS = -g -O0 -DDEBUG
R_FLAGS = -g -Ofast -flto -mtune=native -DNDEBUG
LIBS = -lSDL2 -lGL -lepoxy
PROGRAM = evolution


debug: $(SOURCE) $(HEADER)
	$(CXX) $(FLAGS) $(D_FLAGS) $(SOURCE) $(LIBS) -o $(PROGRAM)

release: $(SOURCE) $(HEADER)
	$(CXX) $(FLAGS) $(R_FLAGS) $(SOURCE) $(LIBS) -o $(PROGRAM)

shaders.h: $(foreach FILE, $(SHADER), shaders/$(FILE))
	{ $(foreach FILE, $(SHADER), xxd -i shaders/$(FILE);) } > shaders.h

clean:
	rm $(PROGRAM)
