all: clear createProducer createConsumer
clear:
	clear;
createProducer:
	g++ -g -o producer producer.cpp
createConsumer:
	g++ -g -o consumer consumer.cpp

	

