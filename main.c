#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define HEIGHT 16
#define WIDTH 10
#define CLEAR_SCREEN_AND_HIDE_CURSOR "\033[2J\033[?25l"
#define SHOW_CURSOR "\033[?25h"

// A virtual grid to represent the state of the playfield.
// This makes it easier to do collision detection, rotation and movement.
// Then when everything has been checked, the virtual grid is rendered.
int virtual_grid[HEIGHT][WIDTH];

// A rendered grid contains the visual representation of the virtual grid.
// It has 2 more height and width than the virtual grid because it renders
// borders as well. However, the borders are not included into the virtual grid
// because we do not need it for evaluation.
//
// width * 2 is to compensate the size of a block drawn with [].
char rendered_grid[HEIGHT + 2][WIDTH * 2 + 2];

struct winsize window;
int center_y, center_x;

int can_update;

// This represents the different tetromino available.
enum Tetromino {
    I,
    O,
    T,
    S,
    Z,
    J,
    L,
};

// This represents the an orientation for a shape. Makes it easier to calculate
// movements and rotations to render.
enum Orientation {
    Top,
    Right,
    Bottom,
    Left,
};

// Current shape properties
enum Tetromino current_shape;
enum Orientation current_shape_orientation;
int current_y, current_x;

volatile sig_atomic_t stop_reading = 0;
struct termios original_tio;

// Function to restore the terminal to its original settings
void restore_terminal_settings() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

// Function to set the terminal to non-canonical mode
void set_non_canonical_mode() {
    struct termios tio;
    tcgetattr(STDIN_FILENO, &tio);
    original_tio = tio;
    atexit(restore_terminal_settings);

    tio.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    tio.c_cc[VMIN] = 1;              // Minimum number of characters to read
    tio.c_cc[VTIME] = 0;             // Timeout for read
    tcsetattr(STDIN_FILENO, TCSANOW, &tio);
}

void *read_from_stdin(void *arg) {
    char ch;
    while (!stop_reading) {
        if (read(STDIN_FILENO, &ch, 1) > 0 && can_update) {
            // read movements
            switch (ch) {
            case 'h':
                current_x -= 1;
                if (current_x < 0)
                    current_x = 0;
                break;
            case 'l':
                current_x += 1;
                if (current_x >= WIDTH) {
                    current_x = WIDTH - 1;
                }
                break;
            case 'q':
                stop_reading = 1;
                break;
            default:
                break;
            }
        }
        usleep(100000);
    }
    return NULL;
}

// Initialize the game, includes playfield and picks the starting tetromino.
int init() {
    set_non_canonical_mode();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            virtual_grid[y][x] = 0;
        }
    }
    for (int y = 0; y < HEIGHT + 2; y++) {
        for (int x = 0; x < WIDTH * 2 + 2; x++) {
            if (y == 0 || y == HEIGHT + 1) {
                rendered_grid[y][x] = '-';
            } else if (x == 0 || x == WIDTH * 2 + 1) {
                rendered_grid[y][x] = ':';
            } else {
                rendered_grid[y][x] = ' ';
            }
        }
    }
    current_y = -1;
    return 0;
}

// cleans up after the game
void clean_up() { printf(SHOW_CURSOR); }

// Update the virtual grid according to various states.
int update() {
    current_y++;
    if (current_y >= HEIGHT) {
        current_y = 0;
    }
    return 0;
}

// Renders the virtual grid.
int view() {
    // render the grid
    // we don't have to touch the borders
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (virtual_grid[y][x] == 1) {
                rendered_grid[y + 1][x * 2 + 1] = '[';
                rendered_grid[y + 1][x * 2 + 2] = ']';
            } else {
                rendered_grid[y + 1][x * 2 + 1] = ' ';
                rendered_grid[y + 1][x * 2 + 2] = ' ';
            }
        }
    }

    // just drawing a single block for simplicity
    rendered_grid[current_y + 1][current_x * 2 + 1] = '[';
    rendered_grid[current_y + 1][current_x * 2 + 2] = ']';

    // clear the screen
    printf(CLEAR_SCREEN_AND_HIDE_CURSOR);
    // print game title
    for (int s = 0; s < center_x; s++) {
        printf(" ");
    }
    printf("Tetris!\n");
    // print the grid
    for (int y = 0; y < HEIGHT + 2; y++) {
        for (int s = 0; s < center_x; s++) {
            printf(" ");
        }
        for (int x = 0; x < WIDTH * 2 + 2; x++) {
            printf("%c", rendered_grid[y][x]);
        }
        printf("\n");
    }
    for (int s = 0; s < center_y; s++) {
        printf("\n");
    }

    return 0;
}

void handle_sigint(int sig) {
    printf("Caught signal %d (SIGINT).", sig);
    clean_up();
    exit(1);
}

int main() {
    pthread_t thread_id;
    signal(SIGINT, handle_sigint);

    // struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == -1) {
        perror("ioctl");
        return 1;
    }

    // get the center points
    center_y = window.ws_row / 2 - HEIGHT / 2;
    center_x = window.ws_col / 2 - WIDTH - 1;

    init();

    if (pthread_create(&thread_id, NULL, read_from_stdin, NULL) != 0) {
        perror("pthread_create");
        clean_up();
        return 1;
    }

    while (!stop_reading) {
        can_update = 1;
        update();
        view();
        can_update = 0;
        usleep(500 * 1000);
    }

    pthread_join(thread_id, NULL);

    clean_up();

    return 0;
}
