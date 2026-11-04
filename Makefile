CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -Isrc
LIB_SRC  = src/TicketQueue.cpp src/ActionStack.cpp src/HelpDesk.cpp src/CommandLine.cpp \
           src/TicketFile.cpp src/Simulator.cpp

helpdesk: src/main.cpp $(LIB_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

test_m1: tests/test_m1.cpp $(LIB_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

test_m2: tests/test_m2.cpp $(LIB_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

test_m3: tests/test_m3.cpp $(LIB_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

test_m4: tests/test_m4.cpp $(LIB_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

test: test_m1 test_m2 test_m3 test_m4
	./test_m1
	./test_m2
	./test_m3
	./test_m4

clean:
	rm -f helpdesk test_m1 test_m2 test_m3 test_m4

.PHONY: test clean
