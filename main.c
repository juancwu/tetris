#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define HEIGHT 16
#define WIDTH 10
#define CLEAR_SCREEN_AND_HIDE_CURSOR "\033[2J\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define ONE_SECOND_IN_MS 1000000
#define MS_100 100000 // Delay in microseconds (100 ms)
#define MS_150 150000 // Delay in microseconds (150 ms)
#define MS_600 600000 // Delay in microseconds (600 ms)

// A virtual grid to represent the state of the playfield.
// This makes it easier to do collision detection, rotation and movement.
// Then when everything has been checked, the virtual grid is rendered.
int virtual_grid[HEIGHT][WIDTH], snap_virtual_grid[HEIGHT][WIDTH];

// A rendered grid contains the visual representation of the virtual grid.
// It has 2 more height and width than the virtual grid because it renders
// borders as well. However, the borders are not included into the virtual grid
// because we do not need it for evaluation.
//
// width * 2 is to compensate the size of a block drawn with [].
char rendered_grid[HEIGHT + 2][WIDTH * 2 + 2];

// Window stats
struct winsize window;
int window_center_y, window_center_x;

// score in the game
int score;

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

long last_gravity_update_time, last_view_update_time,
    last_virtual_grid_update_time;

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

// Gets the current time in miliseconds. The function uses the system time to
// calculate the current time.
long get_current_time() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    return (current_time.tv_sec * ONE_SECOND_IN_MS + current_time.tv_usec);
}

int equal_virtual_grid() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (snap_virtual_grid[y][x] != virtual_grid[y][x]) {
                return 0;
            }
        }
    }
    return 1;
}

void copy_grid() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            virtual_grid[y][x] = snap_virtual_grid[y][x];
        }
    }
}

void set_tetromino_in_grid_as(int i) {
    switch (current_shape) {
    case I:
        snap_virtual_grid[current_y][current_x] = i;
        snap_virtual_grid[current_y][current_x + 1] = i;
        snap_virtual_grid[current_y][current_x + 2] = i;
        snap_virtual_grid[current_y][current_x + 3] = i;
        break;
    case T:
        snap_virtual_grid[current_y][current_x] = i;
        snap_virtual_grid[current_y][current_x + 1] = i;
        snap_virtual_grid[current_y][current_x + 2] = i;
        snap_virtual_grid[current_y + 1][current_x + 1] = i;
        break;
    case O:
        snap_virtual_grid[current_y][current_x] = i;
        snap_virtual_grid[current_y][current_x + 1] = i;
        snap_virtual_grid[current_y + 1][current_x] = i;
        snap_virtual_grid[current_y + 1][current_x + 1] = i;
        break;
    case S:
        snap_virtual_grid[current_y][current_x + 1] = i;
        snap_virtual_grid[current_y][current_x + 2] = i;
        snap_virtual_grid[current_y + 1][current_x + 1] = i;
        snap_virtual_grid[current_y + 1][current_x] = i;
        break;
    case Z:
        snap_virtual_grid[current_y][current_x] = i;
        snap_virtual_grid[current_y][current_x + 1] = i;
        snap_virtual_grid[current_y + 1][current_x + 1] = i;
        snap_virtual_grid[current_y + 1][current_x + 2] = i;
        break;
    case L:
        snap_virtual_grid[current_y][current_x + 2] = i;
        snap_virtual_grid[current_y + 1][current_x] = i;
        snap_virtual_grid[current_y + 1][current_x + 1] = i;
        snap_virtual_grid[current_y + 1][current_x + 2] = i;
        break;
    case J:
        snap_virtual_grid[current_y][current_x] = i;
        snap_virtual_grid[current_y + 1][current_x] = i;
        snap_virtual_grid[current_y + 1][current_x + 1] = i;
        snap_virtual_grid[current_y + 1][current_x + 2] = i;
        break;
    default:
        break;
    }
    if (!equal_virtual_grid()) {
        copy_grid();
    }
}

// Takes a snapshot of the virtual grid
void take_virtual_grid_snapshot() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            snap_virtual_grid[y][x] = virtual_grid[y][x];
        }
    }
}

// This is a thread function that is responsible of handling reading inputs from
// stdin.
void *read_from_stdin(void *arg) {
    char ch;
    long last_input_time = 0, current_input_time;
    while (!stop_reading) {
        if (read(STDIN_FILENO, &ch, 1) > 0) {
            if (ch == 'q') {
                stop_reading = 1;
            } else {
                current_input_time = get_current_time();
                if (last_input_time == 0 ||
                    current_input_time - last_input_time >= MS_100) {
                    last_input_time = current_input_time;
                    set_tetromino_in_grid_as(0);
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
                    default:
                        break;
                    }
                    set_tetromino_in_grid_as(1);
                }
            }
        }
    }
    return NULL;
}

// Randomly picks a tetromino
enum Tetromino pick_tetromino() {
    enum Tetromino t = rand() % 8;
    return t;
}

// Initialize the game, includes playfield and picks the starting tetromino.
int init() {
    srand(time(0));
    set_non_canonical_mode();
    current_shape = pick_tetromino();
    current_x = WIDTH / 2;
    current_y = 0;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            virtual_grid[y][x] = 0;
        }
    }
    set_tetromino_in_grid_as(1);
    take_virtual_grid_snapshot();
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

int can_update_virtual_grid(long current_time) {
    return last_virtual_grid_update_time == 0 ||
           current_time - last_virtual_grid_update_time >= MS_100;
}

int can_update_gravity(long current_time) {
    return last_gravity_update_time == 0 ||
           current_time - last_gravity_update_time >= MS_600;
}

// Update the virtual grid according to various states.
int update(long current_time) {
    if (can_update_virtual_grid(current_time)) {
        set_tetromino_in_grid_as(0);
    }
    if (can_update_gravity(current_time)) {
        last_gravity_update_time = current_time;
        current_y++;
        // TODO: remove this once we have collision
        if (current_y >= HEIGHT) {
            current_y = 0;
        }
    }
    if (can_update_virtual_grid(current_time)) {
        last_virtual_grid_update_time = current_time;
        set_tetromino_in_grid_as(1);
    }
    return 0;
}

// Renders the virtual grid.
int view(long current_time) {
    if (last_view_update_time == 0 ||
        current_time - last_view_update_time >= MS_100) {
        last_view_update_time = current_time;
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

        // clear the screen
        printf(CLEAR_SCREEN_AND_HIDE_CURSOR);
        // print game title and score
        for (int s = 0; s < window_center_x; s++) {
            printf(" ");
        }
        printf("Tetris! Score: %7d\n", score);
        // print the grid
        for (int y = 0; y < HEIGHT + 2; y++) {
            for (int s = 0; s < window_center_x; s++) {
                printf(" ");
            }
            for (int x = 0; x < WIDTH * 2 + 2; x++) {
                printf("%c", rendered_grid[y][x]);
            }
            printf("\n");
        }
        for (int s = 0; s < window_center_y - 1; s++) {
            printf("\n");
        }
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
    window_center_y = window.ws_row / 2 - HEIGHT / 2;
    window_center_x = window.ws_col / 2 - WIDTH - 1;

    init();

    if (pthread_create(&thread_id, NULL, read_from_stdin, NULL) != 0) {
        perror("pthread_create");
        clean_up();
        return 1;
    }

    long current_time;
    while (!stop_reading) {
        current_time = get_current_time();
        update(current_time);
        view(current_time);
    }

    pthread_join(thread_id, NULL);

    clean_up();

    return 0;
}
