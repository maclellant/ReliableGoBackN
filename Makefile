.PHONY : all client server clean

all : client server

client :
	g++ client.cpp util.cpp -o client/client -lrt

server :
	g++ server.cpp util.cpp -o server/server -lrt

clean :
	rm -rf server/server client/client