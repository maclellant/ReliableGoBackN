all : client server

client :
	g++ client.cpp util.cpp -o client/client

server :
	g++ server.cpp util.cpp -o server/server

clean :
	rm -rf server/server client/client