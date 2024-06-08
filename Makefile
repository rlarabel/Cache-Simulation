all: mysim.c
	gcc -g -std=gnu11 -Werror -Wall mysim.c -o mysim

run: mysim
	./mysim -s 4 -E 1 -b 4 -t long.trace

clean: mysim
	rm -rf ./mysim
