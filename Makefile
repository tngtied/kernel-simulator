CC = gcc

project2: source.c
	$(CC) source.c -o project2

clean:
	rm project2