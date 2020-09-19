all: file_server file_client

run_server:
	mkdir -p server
	./bin/server 10001

run_client:
	mkdir -p client
	./bin/client 127.0.0.1 10001

file_server:
	mkdir -p bin
	gcc -o bin/server file_server.c

file_client:
	mkdir -p bin
	gcc -o bin/client file_client.c

clean:
	rm -rf bin
