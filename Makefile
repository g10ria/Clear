clear: clear.c
	$(CC) clear.c -o clear -Wall -Wextra -pedantic -std=c99

run:
	./clear