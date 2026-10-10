all: server
server: avl.c avl.h server.c
	gcc -O2 -Wall avl.c server.c -o server -lpthread
test: avl.c avl.h test_avl.c
	gcc -O2 -Wall avl.c test_avl.c -o test_avl && ./test_avl
clean:
	rm -f server test_avl
