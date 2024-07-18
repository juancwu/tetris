#include <stdio.h>

// default width of 10 squares in width
#define WIDTH 10
// default of 20 squares in height
#define HEIGHT 20

// small to big represents the points gotten from 1, 2, 3, or 4+ lines.
const unsigned short int POINTS[4] = {100, 300, 500, 800};

// map/grid of tetris
char grid[HEIGHT][WIDTH];

void initGrid() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            grid[y][x] = ' ';
        }
    }
}

void paintGrid() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("%c", grid[y][x]);
        }
        printf("\n");
    }
}

int main() {
    printf("It's Tetris, ON THE TERMINAL :o\n");

    initGrid();
    paintGrid();

    return 0;
}
