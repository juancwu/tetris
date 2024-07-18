#include <stdio.h>
#include <stdlib.h>

// default width of 10 squares in width
#define WIDTH 10
// default of 20 squares in height
#define HEIGHT 20

enum TetrominoShape { I, O, T, S, Z, J, L };
enum Orientation { Top, Right, Bottom, Left };

// small to big represents the points gotten from 1, 2, 3, or 4+ lines.
const unsigned short int POINTS[4] = {100, 300, 500, 800};

// map/grid of tetris
char grid[HEIGHT][WIDTH];
// the current tetromino being manipulated
enum TetrominoShape current_tetromino;
// the current orientation of the block being manipulated
enum Orientation current_orientation;
// the current position of the current tetromino, many currents
int current_x, current_y;

void empty_grid() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            grid[y][x] = ' ';
        }
    }
}

void paint_block() {
    switch (current_tetromino) {
    case I:
        // initial shape
        // ####
        switch (current_orientation) {
        case Top:
            grid[current_y][current_x] = '#';
        case Right:
        case Bottom:
        case Left:
        }
    case O:
    case T:
    case S:
    case Z:
    case J:
    case L:
    }
}

// this should be call after update
void view() {
    // reset the grid
    empty_grid();
    // paint the current tetromino
    paint_block();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("%c", grid[y][x]);
        }
        printf("\n");
    }
}

void spawn_block() {
    int index = rand() % 7;
    current_tetromino = index;
    current_x = WIDTH / 2 - 2;
    current_y = 0;
}

void update() {}

int main() {
    printf("It's Tetris, ON THE TERMINAL :o\n");

    empty_grid();

    return 0;
}
