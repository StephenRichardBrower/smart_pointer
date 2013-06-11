all: test.out

test.out: main.o Sptr.hpp
	g++ -Wall -ggdb -pedantic -std=c++11  main.o -o test.out -lpthread

main.o: main.cpp Sptr.hpp
	g++ -ggdb -pedantic -Wall -std=c++11 -c main.cpp -o main.o

clean:
	rm -f *.o parser.exe *.~
