#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>

/* const numbers define */
#define ROW 17
#define COLUMN 49
#define HORI_LINE '-'
#define VERT_LINE '|'
#define CORNER '+'
#define PLAYER '0'
#define WALL_CHAR '='
#define WALL_LEN 15
#define GOLD_CHAR '$'

/* global variables */
int player_x;
int player_y;
char map_data[ROW][COLUMN + 1];

pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int running = 1;    // 1: running, 0: stop
volatile int game_over = 0;  // 0: playing, 1: lose, 2: win, 3: quit

/* walls and gold structures */
typedef struct {
    int row;
    int col;   // starting column (1..COLUMN-2)
    int dir;   // +1 right, -1 left
} Wall;

typedef struct {
    int row;
    int col;
    int dir;   // +1 right, -1 left
    int alive; // 1 alive, 0 collected
} Gold;

#define WALL_COUNT 6
#define GOLD_COUNT 6

Wall walls[WALL_COUNT];
Gold golds[GOLD_COUNT];

/* functions */
int kbhit(void);
void map_print(void);
void rebuild_map(void);
void *input_thread_fn(void *arg);
void *move_thread_fn(void *arg);
void end_screen(int reason); // 1 lose, 2 win, 3 quit

/* Determine a keyboard is hit or not.
 * If yes, return 1. If not, return 0. */
int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

/* print the map */
void map_print(void)
{
    printf("\033[H\033[2J");
    int i;
    for (i = 0; i <= ROW - 1; i++)
        puts(map_data[i]);
}

/* rebuild the map_data from current objects */
void rebuild_map(void)
{
    int i, j;
    // clear interior and borders
    for (i = 0; i < ROW; i++) {
        for (j = 0; j < COLUMN; j++) map_data[i][j] = ' ';
        map_data[i][COLUMN] = '\0';
    }
    for (j = 1; j <= COLUMN - 2; j++) {
        map_data[0][j] = HORI_LINE;
        map_data[ROW - 1][j] = HORI_LINE;
    }
    for (i = 1; i <= ROW - 2; i++) {
        map_data[i][0] = VERT_LINE;
        map_data[i][COLUMN - 1] = VERT_LINE;
    }
    map_data[0][0] = CORNER;
    map_data[0][COLUMN - 1] = CORNER;
    map_data[ROW - 1][0] = CORNER;
    map_data[ROW - 1][COLUMN - 1] = CORNER;

    // draw walls
    for (i = 0; i < WALL_COUNT; i++) {
        int r = walls[i].row;
        int start = walls[i].col; // 1..COLUMN-2
        for (j = 0; j < WALL_LEN; j++) {
            int pos = (start - 1 + j) % (COLUMN - 2); // 0..COLUMN-3
            if (pos < 0) pos += (COLUMN - 2);
            int c = 1 + pos;
            // do not overwrite borders (shouldn't happen)
            if (r >= 1 && r <= ROW - 2 && c >= 1 && c <= COLUMN - 2)
                map_data[r][c] = WALL_CHAR;
        }
    }

    // draw golds
    for (i = 0; i < GOLD_COUNT; i++) {
        if (!golds[i].alive) continue;
        int r = golds[i].row;
        int c = golds[i].col;
        if (r >= 1 && r <= ROW - 2 && c >= 1 && c <= COLUMN - 2)
            map_data[r][c] = GOLD_CHAR;
    }

    // draw player (player cannot be on border by rule)
    if (player_x >= 1 && player_x <= ROW - 2 && player_y >= 1 && player_y <= COLUMN - 2)
        map_data[player_x][player_y] = PLAYER;
}

/* input thread: read keys and move player */
void *input_thread_fn(void *arg)
{
    (void)arg;
    while (running && !game_over) {
        if (kbhit()) {
            int ch = getchar();
            if (ch == EOF) continue;
            ch = tolower(ch);
            pthread_mutex_lock(&map_mutex);
            if (ch == 'q') {
                game_over = 3; // quit
                running = 0;
                pthread_mutex_unlock(&map_mutex);
                break;
            } else if (ch == 'w' || ch == 's' || ch == 'a' || ch == 'd') {
                int nx = player_x;
                int ny = player_y;
                if (ch == 'w') nx = player_x - 1;
                if (ch == 's') nx = player_x + 1;
                if (ch == 'a') ny = player_y - 1;
                if (ch == 'd') ny = player_y + 1;

                // can't go to border or beyond
                if (nx <= 0 || nx >= ROW-1 || ny <= 0 || ny >= COLUMN-1) {
                    // ignore move
                } else {
                    // check if stepping onto wall -> lose
                    if (map_data[nx][ny] == WALL_CHAR) {
                        player_x = nx; player_y = ny;
                        rebuild_map();
                        map_print();
                        game_over = 1; // lose
                        running = 0;
                        pthread_mutex_unlock(&map_mutex);
                        break;
                    }
                    // check if stepping on gold -> collect
                    if (map_data[nx][ny] == GOLD_CHAR) {
                        // find and mark gold dead
                        for (int i = 0; i < GOLD_COUNT; i++) {
                            if (golds[i].alive && golds[i].row == nx && golds[i].col == ny) {
                                golds[i].alive = 0;
                                break;
                            }
                        }
                    }
                    player_x = nx; player_y = ny;
                    // check win
                    int remaining = 0;
                    for (int i = 0; i < GOLD_COUNT; i++) if (golds[i].alive) remaining++;
                    if (remaining == 0) {
                        rebuild_map();
                        map_print();
                        game_over = 2; // win
                        running = 0;
                        pthread_mutex_unlock(&map_mutex);
                        break;
                    }
                }
            }
            // swallow any extra chars on line (ensure no stray prints)
            // (we used non-echo mode locally in kbhit so typed chars won't show)
            rebuild_map();
            map_print();
            pthread_mutex_unlock(&map_mutex);
        }
        usleep(50 * 1000);
    }
    return NULL;
}

/* movement thread: move walls and golds periodically */
void *move_thread_fn(void *arg)
{
    (void)arg;
    // wall speed: want wall fully cross in ~10s; approximate interval
    const useconds_t wall_interval = 200 * 1000; // 200 ms
    const useconds_t gold_interval = 220 * 1000; // 220 ms (slightly different)
    useconds_t counters = 0;

    while (running && !game_over) {
        pthread_mutex_lock(&map_mutex);
        // move walls every wall_interval
        // move each wall by its dir
        for (int i = 0; i < WALL_COUNT; i++) {
            // update start col
            int span = COLUMN - 2; // available columns (1..COLUMN-2)
            int newstart = walls[i].col + walls[i].dir;
            // wrap around in [1, span]
            if (newstart < 1) newstart += span;
            if (newstart > span) newstart -= span;
            walls[i].col = newstart;
        }

        // move golds
        for (int i = 0; i < GOLD_COUNT; i++) {
            if (!golds[i].alive) continue;
            int span = COLUMN - 2;
            int newc = golds[i].col + golds[i].dir;
            if (newc < 1) newc += span;
            if (newc > span) newc -= span;
            golds[i].col = newc;
        }

        // check if any wall occupies player -> lose
        for (int i = 0; i < WALL_COUNT; i++) {
            int r = walls[i].row;
            for (int j = 0; j < WALL_LEN; j++) {
                int pos = (walls[i].col - 1 + j) % (COLUMN - 2);
                if (pos < 0) pos += (COLUMN - 2);
                int c = 1 + pos;
                if (r == player_x && c == player_y) {
                    rebuild_map();
                    map_print();
                    game_over = 1; // lose
                    running = 0;
                    break;
                }
            }
            if (game_over) break;
        }
        if (game_over) {
            pthread_mutex_unlock(&map_mutex);
            break;
        }

        // check if any gold moves onto player -> collect
        for (int i = 0; i < GOLD_COUNT; i++) {
            if (!golds[i].alive) continue;
            if (golds[i].row == player_x && golds[i].col == player_y) {
                golds[i].alive = 0;
            }
        }
        // check remaining golds for win
        int remaining = 0;
        for (int i = 0; i < GOLD_COUNT; i++) if (golds[i].alive) remaining++;
        if (remaining == 0) {
            rebuild_map();
            map_print();
            game_over = 2; // win
            running = 0;
            pthread_mutex_unlock(&map_mutex);
            break;
        }

        // redraw
        rebuild_map();
        map_print();
        pthread_mutex_unlock(&map_mutex);

        // sleep
        usleep(wall_interval);
    }
    return NULL;
}

void end_screen(int reason)
{
    printf("\033[H\033[2J");
    if (reason == 1) {
        printf("You loss the game!!\n");
    } else if (reason == 2) {
        printf("You win the game!!\n");
    } else if (reason == 3) {
        printf("You exit the game.\n");
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    int i, j;

    /* initialize the map array */
    for (i = 0; i < ROW; i++) {
        for (j = 0; j < COLUMN; j++) map_data[i][j] = ' ';
        map_data[i][COLUMN] = '\0';
    }

    // borders & interior
    for (j = 1; j <= COLUMN - 2; j++) {
        map_data[0][j] = HORI_LINE;
        map_data[ROW - 1][j] = HORI_LINE;
    }
    for (i = 1; i <= ROW - 2; i++) {
        map_data[i][0] = VERT_LINE;
        map_data[i][COLUMN - 1] = VERT_LINE;
    }
    map_data[0][0] = CORNER;
    map_data[0][COLUMN - 1] = CORNER;
    map_data[ROW - 1][0] = CORNER;
    map_data[ROW - 1][COLUMN - 1] = CORNER;

    /* initialize player */
    player_x = ROW / 2;
    player_y = COLUMN / 2;
    map_data[player_x][player_y] = PLAYER;

    /* initialize walls */
    int wall_rows[WALL_COUNT] = {2,4,6,10,12,14};
    for (i = 0; i < WALL_COUNT; i++) {
        walls[i].row = wall_rows[i];
        // random starting col in [1, COLUMN-2]
        walls[i].col = (rand() % (COLUMN - 2)) + 1;
        // directions: right, left, right, left...
        walls[i].dir = (i % 2 == 0) ? 1 : -1;
    }

    /* initialize golds */
    int gold_rows[GOLD_COUNT] = {1,3,5,11,13,15};
    for (i = 0; i < GOLD_COUNT; i++) {
        golds[i].row = gold_rows[i];
        golds[i].col = (rand() % (COLUMN - 2)) + 1;
        golds[i].dir = (rand() % 2 == 0) ? 1 : -1;
        golds[i].alive = 1;
    }

    rebuild_map();
    map_print();

    // create threads
    pthread_t input_thread, mover_thread;
    pthread_create(&input_thread, NULL, input_thread_fn, NULL);
    pthread_create(&mover_thread, NULL, move_thread_fn, NULL);

    // wait for threads
    pthread_join(input_thread, NULL);
    pthread_join(mover_thread, NULL);

    // show end screen based on game_over
    if (game_over == 1) end_screen(1);
    else if (game_over == 2) end_screen(2);
    else if (game_over == 3) end_screen(3);
    else end_screen(3);

    return 0;
}