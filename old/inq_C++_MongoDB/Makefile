test_mongo: test_mongo.o
	g++ test_mongo.cpp -pedantic -Wall -o test_mongo -lbluetooth\
		 -lmongoclient -lboost_thread-mt -lboost_filesystem-mt\
		 -lboost_program_options-mt

test_mongo.o: test_mongo.cpp
	g++ test_mongo.cpp -c -pedantic -Wall -o test_mongo.o

