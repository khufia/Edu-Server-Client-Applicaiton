EXECS = server client

all: $(EXECS)

server : server.c
	gcc -o server  server.c -lrt -lpthread

client : client.c
	gcc -o client client.c -lrt -lpthread 

clean :  
	rm -f $(EXECS)