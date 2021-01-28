CXX = g++
CXXFLAGS = -O2 -Wall -Wno-unused-result -std=c++17 -g
SRC = ./src
HEADERS = ./src/config.h ./src/utils.h

all: | ./bin ./build ./bin/miku /var/local/bin/isolate ./bin/tddump

./bin/miku: ./build/main.o ./build/sandbox.o ./build/testsuite.o ./build/server_io.o ./build/utils.o ./build/batchjudge.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

./bin/tddump: ./build/tddump.o ./build/server_io.o ./build/utils.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

./build/tddump.o: ./src/tddump.cpp $(HEADERS)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

./build/utils.o: ./src/utils.cpp ./src/utils.h
	$(CXX) -o $@ -c $< $(CXXFLAGS)

./build/main.o: ./src/main.cpp ./src/testsuite.h ./src/server_io.h $(HEADERS)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

./build/sandbox.o: ./src/sandbox.cpp ./src/sandbox.h $(HEADERS)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

./build/testsuite.o: ./src/testsuite.cpp ./src/testsuite.h ./src/sandbox.h ./src/batchjudge.h $(HEADERS)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

./build/server_io.o: ./src/server_io.cpp ./src/server_io.h $(HEADERS)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

./build/batchjudge.o: ./src/batchjudge.cpp ./src/batchjudge.h ./src/sandbox.h $(HEADERS)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

/var/local/bin/isolate: ./isolate/*
	make -C ./isolate/ install

./bin:
	mkdir -p ./bin

./build:
	mkdir -p ./build

clean:
	rm -f -r ./build/* ./bin/*
	make -C ./isolate/ clean
