crow:
	wget -O lib/crow.h https://github.com/ipkn/crow/releases/download/v0.1/crow_all.h

debug:
	c++ lib/util.cpp lib/slug.cpp lib/manager.cpp lib/access_log.cpp lib/daemon.cpp main.cpp --std=c++11 -g -o main -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/bsoncxx/v_noabi -L/usr/local/lib -lmongocxx -lbsoncxx -Wl,-rpath,/usr/local/lib -lboost_system -lpthread

release:
	c++ -O2 lib/util.cpp lib/slug.cpp lib/manager.cpp lib/access_log.cpp lib/daemon.cpp main.cpp --std=c++11 -o main -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/bsoncxx/v_noabi -L/usr/local/lib -lmongocxx -lbsoncxx -Wl,-rpath,/usr/local/lib -lboost_system -lpthread

