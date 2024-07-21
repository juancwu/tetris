CC := clang

ifneq ($(CC),)
	CC := $(CC)
endif

build:
	@$(CC) -o bin/tetris main.c
run:
	@./bin/tetris
clean:
	rm -f ./bin/tetris
