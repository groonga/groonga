all: suggest-learner suggest-httpd

suggest-httpd: suggest-httpd.c util.c
	gcc -O3 -ggdb -Wall -o suggest-httpd suggest-httpd.c util.c -lzmq -levent -lmsgpack -lgroonga

suggest-learner: suggest-learner.c util.c
	gcc -O3 -ggdb -Wall -o suggest-learner suggest-learner.c util.c -lzmq -lmsgpack -lgroonga

clean:
	rm suggest-httpd suggest-learner
