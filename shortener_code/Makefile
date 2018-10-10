crow:
	wget -O crow.h https://github.com/ipkn/crow/releases/download/v0.1/crow_all.h

debug:
	c++ slug.cpp main.cpp --std=c++11 -g -o main $(pkg-config --cflags --libs libmongocxx) -Wl,-rpath,/usr/local/lib -lboost_system -lpthread

test:
	curl -v "http://localhost:18080/hello"

