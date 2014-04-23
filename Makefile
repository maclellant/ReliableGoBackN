all : client server

client : | dirtree
	g++ client.cpp -o client/client

server : | dirtree
	g++ server.cpp -o server/server

dirtree :
	@mkdir server client