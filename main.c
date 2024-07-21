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
// Delay in microseconds (100 ms)
#define MS_100 100000
// Delay in microseconds (150 ms)
#define MS_150 150000
// Delay in microseconds (600 ms)
#define MS_600 600000
// Each tetromino can be represented by 4 points. This is the size of the array
// containing those points.
#define TETROMINO_BLOCK_SIZE 4

// Represents an individual point/block that makes up a tetromino.
typedef struct {
    int x, y;
} Point;

// A game state. Everything the game needs will be here.
typedef struct {
    // A virtual grid to represent the state of the playfield.
    // This makes it easier to do collision detection, rotation and movement.
    // Then when everything has been checked, the grid can be printed.
    int **virtual_grid;
    // A snapshot of the virtual grid. This is used for comparison before
    // updating the virtual grid.
    int snap_virtual_grid[HEIGHT][WIDTH];

    // Current tetromino being manipulated
    Point points[TETROMINO_BLOCK_SIZE];

    // Window stat, the center point on the Y-axis.
    int window_center_y;
    // Window stat, the center point on the X-axis.
    int window_center_x;

    // Keep track of the time the main loop refreshes.
    long current_time;

    // The score in the game
    int score;

    // Keep track when was the last gravity update.
    // By gravity, it means the time a tetromino falls by one block.
    long last_gravity_update_time;
    // Keep track of when was the last virtual grid update.
    // This help not overloading the virtual grid too fast which makes the
    // terminal shift a lot when rendering the grid.
    long last_virtual_grid_update_time;
    // Keep track when was the last view rendered. Reduce overloading with
    // re-renders.
    long last_view_update_time;
} GameState;

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

// Gets the current time in miliseconds. The function uses the system time to
// calculate the current time.
long get_current_time() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    return (current_time.tv_sec * ONE_SECOND_IN_MS + current_time.tv_usec);
}

// Checks if grid A is equal to grid B.
int equal_virtual_grid(int A[HEIGHT][WIDTH], int B[HEIGHT][WIDTH]) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (A[y][x] != B[y][x]) {
                return 0;
            }
        }
    }
    return 1;
}

// Copies the grid from src to dst.
void copy_grid(int src[HEIGHT][WIDTH], int dst[HEIGHT][WIDTH]) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dst[y][x] = src[y][x];
        }
    }
}

// Clears the tetromino described by the points on the grid.
void clear_tetromino_in_grid(int **grid, Point *points) {
    int x, y;
    for (int i = 0; i < TETROMINO_BLOCK_SIZE; i++) {
        x = points[i].x, y = points[i].y;
        grid[y][x] = 0;
    }
}

// Places the tetromino described by the points on the grid.
void place_tetromino_in_grid(int **grid, Point *points) {
    int x, y;
    for (int i = 0; i < TETROMINO_BLOCK_SIZE; i++) {
        x = points[i].x, y = points[i].y;
        grid[y][x] = 1;
    }
}

// Takes a snapshot of the virtual grid
void take_virtual_grid_snapshot(GameState *game_state) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            game_state->snap_virtual_grid[y][x] =
                game_state->virtual_grid[y][x];
        }
    }
}

// This is a thread function that is responsible of handling reading inputs from
// stdin.
void *read_from_stdin(void *arg) {
    GameState *game_state = (GameState *)arg;
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
                    // TODO: should clear grid here
                    // read movements
                    switch (ch) {
                    case 'h':
                        break;
                    case 'l':
                        break;
                    default:
                        break;
                    }
                    // TODO: should place block here
                }
            }
        }
    }
    return NULL;
}

// Randomly picks a tetromino and sets the default starting points of a
// tetromino in the game state.
void pick_tetromino(GameState *game_state) {
    enum Tetromino t = rand() % 8;
    switch (t) {
    case I:
        // Shape
        // [][][][]
        for (int i = 0; i < TETROMINO_BLOCK_SIZE; i++) {
            game_state->points[i].y = 0;
            game_state->points[i].x = WIDTH / 2 + i;
        }
        break;
    case T:
        // Shape
        //   []
        // [][][]
        // top middle point
        game_state->points[0].y = 0;
        game_state->points[0].x = WIDTH / 2 + 1;

        // lower left point
        game_state->points[1].y = 1;
        game_state->points[1].x = WIDTH / 2;

        // lower middle point
        game_state->points[2].y = 1;
        game_state->points[2].x = WIDTH / 2 + 1;

        // lower right point
        game_state->points[3].y = 1;
        game_state->points[3].x = WIDTH / 2 + 2;
        break;
    case O:
        // Shape
        // [][]
        // [][]
        game_state->points[0].y = 0;
        game_state->points[0].x = 0;

        game_state->points[1].y = 0;
        game_state->points[1].x = 1;

        game_state->points[2].y = 1;
        game_state->points[2].x = 0;

        game_state->points[3].y = 1;
        game_state->points[3].x = 1;
        break;
    case S:
        // Shape
        //   [][]
        // [][]
        // top middle point
        game_state->points[0].y = 0;
        game_state->points[0].x = WIDTH / 2 + 1;

        // top right point
        game_state->points[1].y = 0;
        game_state->points[1].x = WIDTH / 2 + 2;

        // lower left point
        game_state->points[2].y = 1;
        game_state->points[2].x = WIDTH / 2;

        // lower middle point
        game_state->points[3].y = 1;
        game_state->points[3].x = WIDTH / 2 + 1;
        break;
    case Z:
        // Shape
        // [][]
        //   [][]
        // top left point
        game_state->points[0].y = 0;
        game_state->points[0].x = WIDTH / 2;

        // top middle point
        game_state->points[1].y = 0;
        game_state->points[1].x = WIDTH / 2 + 1;

        // lower middle point
        game_state->points[2].y = 1;
        game_state->points[2].x = WIDTH / 2 + 1;

        // lower right point
        game_state->points[3].y = 1;
        game_state->points[3].x = WIDTH / 2 + 2;
        break;
    case L:
        // Shape
        //     []
        // [][][]
        // top right point
        game_state->points[0].y = 0;
        game_state->points[0].x = WIDTH / 2 + 2;

        // lower left point
        game_state->points[1].y = 1;
        game_state->points[1].x = WIDTH / 2;

        // lower middle point
        game_state->points[2].y = 1;
        game_state->points[2].x = WIDTH / 2 + 1;

        // lower right point
        game_state->points[3].y = 1;
        game_state->points[3].x = WIDTH / 2 + 2;
        break;
    case J:
        // Shape
        // []
        // [][][]
        // top left point
        game_state->points[0].y = 0;
        game_state->points[0].x = WIDTH / 2;

        // lower left point
        game_state->points[1].y = 1;
        game_state->points[1].x = WIDTH / 2;

        // lower middle point
        game_state->points[2].y = 1;
        game_state->points[2].x = WIDTH / 2 + 1;

        // lower right point
        game_state->points[3].y = 1;
        game_state->points[3].x = WIDTH / 2 + 2;
        break;
    }
}

// Initialize the game, includes playfield and picks the starting tetromino.
void init(GameState *game_state) {
    // Window stats
    struct winsize *window = malloc(sizeof(struct winsize));
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, window) == -1) {
        perror("ioctl");
        return;
    }

    // get the center points
    game_state->window_center_y = window->ws_row / 2 - HEIGHT / 2;
    game_state->window_center_x = window->ws_col / 2 - WIDTH - 1;

    // no need of it anymore.
    free(window);
    window = NULL;

    // set the random seed
    srand(time(0));
    set_non_canonical_mode();
    pick_tetromino(game_state);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            game_state->virtual_grid[y][x] = 0;
        }
    }
    place_tetromino_in_grid(game_state->virtual_grid, game_state->points);
    take_virtual_grid_snapshot(game_state);
}

// cleans up after the game
void clean_up() { printf(SHOW_CURSOR); }

// Checks if 100ms has passed since the last update.
int can_update_virtual_grid(long now, long last) {
    return last == 0 || now - last >= MS_100;
}

// Checks if 600ms has passed since the last update.
int can_update_gravity(long now, long last) {
    return last == 0 || now - last >= MS_600;
}

// Collision detection on the bottom of the current points.
int detect_collision_bottom(GameState *game_state) {
    for (int i = 0; i < TETROMINO_BLOCK_SIZE; i++) {
        int peek_y = game_state->points[i].y + 1;
        if (game_state->virtual_grid[peek_y][game_state->points[i].x] == 1) {
            return 1;
        }
    }
    return 0;
}

// Shifts the current points 1 unit down if possible, otherwise it will stay the
// same.
void shift_points_down(GameState *game_state) {
    if (detect_collision_bottom(game_state)) {
        return;
    }
    for (int i = 0; i < TETROMINO_BLOCK_SIZE; i++) {
        game_state->points[i].y++;
    }
}

// Update the virtual grid according to various states.
int update(GameState *game_state) {
    if (can_update_virtual_grid(game_state->current_time,
                                game_state->last_virtual_grid_update_time)) {
        clear_tetromino_in_grid(game_state->virtual_grid, game_state->points);
    }
    if (can_update_gravity(game_state->current_time,
                           game_state->last_gravity_update_time)) {
        game_state->last_gravity_update_time = game_state->current_time;
        shift_points_down(game_state);
    }
    if (can_update_virtual_grid(game_state->current_time,
                                game_state->last_gravity_update_time)) {
        game_state->last_virtual_grid_update_time = game_state->current_time;
        place_tetromino_in_grid(game_state->virtual_grid, game_state->points);
    }
    return 0;
}

// This will allocate a new array of empty spaces of size n.
char *get_spaces(int n) {
    char *spaces = (char *)malloc(n * sizeof(char));
    if (spaces == NULL) {
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        spaces[i] = ' ';
    }
    return spaces;
}

// Renders the virtual grid.
int view(GameState *game_state) {
    if (game_state->last_view_update_time == 0 ||
        game_state->current_time - game_state->last_view_update_time >=
            MS_100) {
        // update the view update time
        game_state->last_view_update_time = game_state->current_time;

        // get spaces to center the view
        char *spaces = get_spaces(game_state->window_center_x);

        // clear the screen
        // printf(CLEAR_SCREEN_AND_HIDE_CURSOR);

        // print game title and score
        printf("%sTetris! Score: %7d\n", spaces, score);

        // calculate the real rendered width of the grid
        // Multiply by 2 because each 1 or block of the tetromino
        // is two characters long and plus 2 to make up for the left/right
        // borders.
        int real_rendered_width = WIDTH * 2 + 2;

        // print the top border
        printf("%s", spaces);
        for (int s = 0; s < real_rendered_width; s++) {
            printf("-");
        }
        printf("\n");

        // print the grid
        for (int y = 0; y < HEIGHT; y++) {
            printf("%s", spaces);
            printf(":");
            for (int x = 0; x < WIDTH; x++) {
                if (game_state->virtual_grid[y][x] == 1) {
                    printf("[]");
                } else {
                    printf("  ");
                }
            }
            printf(":\n");
        }

        // print the bottom border
        printf("%s", spaces);
        for (int s = 0; s < real_rendered_width; s++) {
            printf("-");
        }
        printf("\n");

        for (int s = 0; s < game_state->window_center_y - 1; s++) {
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

void debug_points(Point points[TETROMINO_BLOCK_SIZE]) {
    for (int i = 0; i < TETROMINO_BLOCK_SIZE; i++) {
        printf("(%d, %d), ", points[i].y, points[i].x);
    }
    printf("\n");
}

void debug_grid(int **grid) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("%d ", grid[y][x]);
        }
        printf("\n");
    }
}

int main() {
    // thread to read from stdin without blocking the main loop.
    pthread_t thread_id;

    // properly handle ctrl+c
    signal(SIGINT, handle_sigint);

    // create a new game state
    GameState game_state;
    init(&game_state);

    if (pthread_create(&thread_id, NULL, read_from_stdin, &game_state) != 0) {
        perror("pthread_create");
        clean_up();
        return 1;
    }

    while (!stop_reading) {
        game_state.current_time = get_current_time();
        update(&game_state);
        debug_points(game_state.points);
        printf("last gravity update: %ld\n",
               game_state.last_gravity_update_time);
        printf("last view update: %ld\n", game_state.last_view_update_time);
        debug_grid(game_state.virtual_grid);
        // view(&game_state);
        usleep(MS_600);
    }

    pthread_join(thread_id, NULL);

    clean_up();

    return 0;
}
