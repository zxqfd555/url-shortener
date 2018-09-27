debug:
	g++ hello.cpp -g -o hello -lboost_system -lpthread --std=c++11

release:
	g++ hello.cpp -o hello -lboost_system -lpthread --std=c++11 -O2

test:
	curl -v "http://localhost:18080/hello"

