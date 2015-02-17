copyright-fixer: main.c Makefile
	$(CC) -Wall -Werror -O2 -o $@ $<
