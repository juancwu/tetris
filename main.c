#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// default width of 22 squares in width
#define WIDTH 22
// default of 33 squares in height
#define HEIGHT 23
// the extra spaces in the width
#define WIDTH_OFFSET 1
// the extra spaces in the height
#define HEIGHT_OFFSET 1

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
            if (y == 0 || y == HEIGHT - 2) {
                grid[y][x] = '-';
            } else if (x == 0 || x == WIDTH - 1) {
                grid[y][x] = ':';
            } else {
                grid[y][x] = ' ';
            }
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
        case Bottom:
            for (int i = 0; i < 4; i++) {
                grid[current_y + HEIGHT_OFFSET][current_x + i] = '[';
                grid[current_y + HEIGHT_OFFSET][current_x + i + 1] = ']';
            }
            break;
        case Right:
        case Left:
            for (int i = 0; i < 4; i++) {
                grid[current_y + i + HEIGHT_OFFSET][current_x] = '[';
                grid[current_y + i + HEIGHT_OFFSET][current_x + 1] = ']';
            }
            break;
        }
        break;
    case O:
        // initial shape
        // ##
        // ##
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 4; x += 2) {
                grid[current_y + y + HEIGHT_OFFSET]
                    [current_x + x + WIDTH_OFFSET] = '[';
                grid[current_y + y + HEIGHT_OFFSET]
                    [current_x + x + WIDTH_OFFSET + 1] = ']';
            }
        }
        break;
    case T:
    case S:
    case Z:
    case J:
    case L:
        break;
    }
}

void print_grid() {
    // Move the cursor to the top-left corner of the terminal
    printf("\033[H");
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("%c", grid[y][x]);
        }
        printf("\n");
    }
}

// this should be call after update
void view() {
    // reset the grid
    empty_grid();
    // paint the current tetromino
    paint_block();
    // print it to console
    print_grid();
}

void spawn_block() {
    // int index = rand() % 7;
    int index = O;
    current_tetromino = index;
    current_x = WIDTH / 2 - 2;
    current_y = 0;
}

void update() { current_y = (current_y + 1) % (HEIGHT - HEIGHT_OFFSET); }

int main() {
    // 500ms
    int print_rate = 500 * 1000;

    printf("It's Tetris, ON THE TERMINAL :o\n");

    empty_grid();
    spawn_block();
    current_orientation = Left;
    // Clear the screen and hide the cursor
    printf("\033[2J\033[?25l");
    while (1) {
        update();
        view();
        usleep(print_rate);
    }
    // Show the cursor again before exiting
    printf("\033[?25h");

    return 0;
}
