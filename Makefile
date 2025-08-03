all: logger

lib/%.o: src/%.cpp
	g++ -Wall -Wextra -c -o $@ $<

logger: src/logger.cpp lib/maxlib.o
	g++ -Wall -Wextra -Isrc -o $@ $^ -lm

clean:
	rm -f lib/*.o logger
