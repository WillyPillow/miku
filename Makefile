CC = g++
CFLAGS = -O2 -Wall -Wno-unused-result -std=c++11 -g
SRC = ./src
HEADERS = ./src/config.h ./src/utils.h

all: | ./bin ./build ./bin/miku ./bin/isolate ./bin/batchjudge ./bin/tddump

./bin/miku: ./build/main.o ./build/sandbox.o ./build/testsuite.o ./build/server_io.o ./build/utils.o
	$(CC) -o $@ $^ $(CFLAGS)

./bin/tddump: ./build/tddump.o ./build/server_io.o ./build/utils.o
	$(CC) -o $@ $^ $(CFLAGS)

./build/tddump.o: ./src/tddump.cpp $(HEADERS)
	$(CC) -o $@ -c $< $(CFLAGS)

./build/utils.o: ./src/utils.cpp ./src/utils.h
	$(CC) -o $@ -c $< $(CFLAGS)

./build/main.o: ./src/main.cpp ./src/testsuite.h ./src/server_io.h $(HEADERS)
	$(CC) -o $@ -c $< $(CFLAGS)

./build/sandbox.o: ./src/sandbox.cpp ./src/sandbox.h $(HEADERS)
	$(CC) -o $@ -c $< $(CFLAGS)

./build/testsuite.o: ./src/testsuite.cpp ./src/testsuite.h ./src/sandbox.h $(HEADERS)
	$(CC) -o $@ -c $< $(CFLAGS)

./build/server_io.o: ./src/server_io.cpp ./src/server_io.h $(HEADERS)
	$(CC) -o $@ -c $< $(CFLAGS)

./bin/batchjudge: ./build/batchjudge.o ./build/sandbox.o ./build/utils.o
	$(CC) -o $@ $^ $(CFLAGS)

./build/batchjudge.o: ./src/batchjudge.cpp ./src/sandbox.h $(HEADERS)
	$(CC) -o $@ -c $< $(CFLAGS)

./bin/isolate: ./isolate/*
	make -C ./isolate/
	mv ./isolate/isolate ./bin/isolate

./bin:
	mkdir -p ./bin

./build:
	mkdir -p ./build

clean:
	rm -f -r ./build/*
