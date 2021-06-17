all: server client

server: bin/aurrasd

client: bin/aurras

bin/aurrasd: obj/aurrasd.o obj/request.o
	gcc -g obj/aurrasd.o obj/request.o -o bin/aurrasd

obj/aurrasd.o: src/aurrasd.c
	gcc -Wall -g -c src/aurrasd.c -o obj/aurrasd.o

obj/request.o: src/request.c src/request.h
	gcc -Wall -g -c src/request.c -o obj/request.o

bin/aurras: obj/aurras.o
	gcc -g obj/aurras.o -o bin/aurras

obj/aurras.o: src/aurras.c
	gcc -g -c src/aurras.c -o obj/aurras.o

clean:
	rm obj/* tmp/* bin/{aurras,aurrasd}

test:
	bin/aurras samples/sample-1.mp3 tmp/sample-1.mp3
	bin/aurras samples/sample-2.mp3 tmp/sample-2.mp3
