all : client server

client :
	g++ client.cpp -o client/client

server :
	g++ server.cpp -o server/server

clean :
	rm -rf server/server client/client