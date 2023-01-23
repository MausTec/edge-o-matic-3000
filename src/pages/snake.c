/**
 * @file snake.c
 * @author Idia Shroud (ishroud72@nightraven.edu)
 * @brief An example UI page which renders the game Snake.
 * @version $Version$
 * @date 2023-01-19
 * 
 * @copyright Copyright (c) 2023 Maus-Tec Electronics
 * 
 * What is a snake?! A miserable little pile of slithers. But enough talk, have at you!
 * 
 * This file should actually be split into a platform-agnostic Snake library, and Page
 * files should only be used for rendering and interacting with the UI, but not process
 * device functions.
 * 
 * The Render hook will only be executed in the main task, or the UI Render task, and should
 * not process any additional code aside from what is required to render the UI.
 * 
 */

#include "ui/ui.h"
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "page:snake";

#define SNAKE_CUBE_SIZE 3
#define SNAKE_PADDING_SIZE 1

#define GAME_TABLE_WIDTH  ((EOM_DISPLAY_WIDTH) / (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE))
#define GAME_TABLE_HEIGHT ((EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT - 1) / (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE))

enum snake_game_state { GAME_STARTING, GAME_PAUSED, GAME_RUNNING, GAME_OVER };
enum snake_move_direction { UP, RIGHT, DOWN, LEFT };
enum snake_turn_direction { CW = 1, CCW = -1 };

struct snake_point { 
    uint8_t x;
    uint8_t y;
};

struct snake_node {
    struct snake_point point;
    struct snake_node *next;
    struct snake_node *prev;
};

struct snake_game_status {
    enum snake_game_state state;
    enum snake_move_direction direction;
    struct snake_node *head;
    struct snake_node *tail;
    uint8_t length;
    struct snake_point food;
    int64_t last_tick;
};

static struct snake_game_status *Game = NULL;

static void snake_deinit_game(void) {
    if (Game == NULL) return;

    struct snake_node *ptr = Game->head;
    
    while(ptr != NULL) {
        struct snake_node *tmp = ptr;
        ptr = ptr->next;
        free(tmp);
    }

    free(Game);
    Game = NULL;
}

static void snake_add_node(uint8_t x, uint8_t y) {
    if (Game == NULL) return;
    
    struct snake_node *node = (struct snake_node*) malloc(sizeof(struct snake_node));
    node->point.x = x % GAME_TABLE_WIDTH;
    node->point.y = y % GAME_TABLE_HEIGHT;
    node->prev = NULL;
    node->next = Game->head;

    if (Game->head == NULL) Game->tail = node;
    else Game->head->prev = node;

    Game->head = node;
    Game->length++;
}

static int snake_check_intersect(struct snake_point *p) {
    if (Game == NULL) return 0;

    struct snake_node *ptr = Game->head;
    int idx = 0;
    int max = -1;
    
    while (ptr != NULL) {
        if (p->x == ptr->point.x && p->y == ptr->point.y) {
            max = idx;
        }

        ptr = ptr->next;
        idx++;
    }

    return max;
}

static void snake_randomize_food(void) {
    if (Game == NULL) return;

    struct snake_point p = {0, 0};

    do {
        p.x = (rand() % GAME_TABLE_WIDTH);
        p.y = (rand() % GAME_TABLE_HEIGHT);
    } while (snake_check_intersect(&p) >= 0);

    Game->food = p;
}

static void snake_remove_tail(void) {
    if (Game == NULL) return;

    struct snake_node *tail = Game->tail;
    if (tail != NULL) {
        struct snake_node *prev = tail->prev;

        if (prev == NULL) {
            Game->head = NULL;
            Game->tail = NULL;
            Game->length = 0;
        } else {
            prev->next = NULL;
            Game->length--;
            Game->tail = prev;
        }

        free(tail);
    } else {
        Game->head = NULL;
        Game->length = 0;
    }
}

static void snake_init_game(void) {
    if (Game != NULL) snake_deinit_game();

    Game = (struct snake_game_status*) malloc(sizeof(struct snake_game_status));
    memset(Game, 0, sizeof(struct snake_game_status));

    Game->state = GAME_STARTING;
    Game->direction = RIGHT;

    // Render our first snake!
    snake_add_node((GAME_TABLE_WIDTH / 2) - 3, GAME_TABLE_HEIGHT / 2);
    snake_add_node((GAME_TABLE_WIDTH / 2) - 2, GAME_TABLE_HEIGHT / 2);
    snake_add_node((GAME_TABLE_WIDTH / 2) - 1, GAME_TABLE_HEIGHT / 2);
    snake_randomize_food();
}

static uint32_t snake_calculate_delay_ms(void) {
    if (Game == NULL) return 0;
    return (1000UL / (Game->length - 1));
}

static int snake_move(void) {
    if (Game == NULL || Game->head == NULL) return 0;

    struct snake_point *p = &Game->head->point;

    switch (Game->direction) {
        case UP:
        snake_add_node(p->x, p->y - 1);
        break;

        case DOWN:
        snake_add_node(p->x, p->y + 1);
        break;

        case LEFT:
        snake_add_node(p->x - 1, p->y);
        break;

        case RIGHT:
        snake_add_node(p->x + 1, p->y);
        break;
    }

    if (snake_check_intersect(&Game->head->point) > 0) {
        return 1;
    }

    if (snake_check_intersect(&Game->food) < 0) {
        snake_remove_tail();
    } else {
        snake_randomize_food();
    }

    return -1;
}

static void snake_turn(enum snake_turn_direction dir) {
    if (Game == NULL) return;
    Game->direction = (Game->direction + dir) % 4;
}

static void snake_game_pause() {
    if (Game == NULL) return;

    if (Game->state == GAME_PAUSED) {
        Game->state = GAME_RUNNING;
    } else if (Game->state == GAME_RUNNING) {
        Game->state = GAME_PAUSED;
    }
}

static void on_open(void *arg) {
    snake_init_game();
}

static void on_render(u8g2_t *display, void *arg) {
    if (Game == NULL) return;

    struct snake_node *s = Game->head;

    while (s != NULL) {
        if (s == Game->head) {
            u8g2_DrawFrame(
                display, 
                s->point.x * (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE), 
                s->point.y * (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE), 
                SNAKE_CUBE_SIZE, SNAKE_CUBE_SIZE
            );
        } else {
            u8g2_DrawBox(
                display, 
                s->point.x * (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE), 
                s->point.y * (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE), 
                SNAKE_CUBE_SIZE, SNAKE_CUBE_SIZE
            ); 
        }

        s = s->next;
    }

    u8g2_DrawVLine(
        display, 
        Game->food.x * (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE) + 1, 
        Game->food.y * (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE),
        3
    );

    u8g2_DrawHLine(
        display, 
        Game->food.x * (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE), 
        (Game->food.y * (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE)) + 1,
        3
    );

    ui_draw_button_labels(display, "<", 
        Game->state == GAME_PAUSED ? "PLAY" : 
        Game->state == GAME_STARTING ? "START" : 
        Game->state == GAME_OVER ? "RESTART" : "PAUSE", 
        ">"
    );
}

static ui_render_flag_t on_loop(void *arg) {
    if (Game == NULL) return NORENDER;
    int64_t millis = esp_timer_get_time() / 1000UL;

    if ((millis - Game->last_tick) >= snake_calculate_delay_ms()) {
        Game->last_tick = millis;

        if (Game->state != GAME_RUNNING) {
            return NORENDER;
        }

        int consume = snake_move();

        if (consume >= 0) {
            Game->state = GAME_OVER;
            ui_toast("GAME OVER!\nScore: %d", Game->length);
        }

        return RENDER;
    }

    return NORENDER;
}

static void on_close(void *arg) {
    snake_deinit_game();
}

static ui_render_flag_t on_button(eom_hal_button_t button, eom_hal_button_event_t evt, void *arg) {
    if (evt != EOM_HAL_BUTTON_PRESS) return PASS;
    if (Game == NULL) return NORENDER;
    
    switch (button) {
        case EOM_HAL_BUTTON_BACK:
            if (Game->state == GAME_RUNNING) {
                snake_turn(CCW);
                return RENDER;
            }
            break;
        case EOM_HAL_BUTTON_OK:
            if (Game->state == GAME_RUNNING) {
                snake_turn(CW);
                return RENDER;
            }
            break;
        case EOM_HAL_BUTTON_MID:
            if (Game->state == GAME_RUNNING || Game->state == GAME_PAUSED) {
                snake_game_pause();
                return RENDER;
            }

            if (Game->state == GAME_STARTING) {
                Game->state = GAME_RUNNING;
                return RENDER;
            }

            if (Game->state == GAME_OVER) {
                snake_init_game();
                return RENDER;
            }

            break;
        default:
            return PASS;
            break;
    }

    return NORENDER;
}

static ui_render_flag_t on_encoder(int difference, void *arg) {
    if (Game == NULL) return NORENDER;

    if (difference < 0) {
        snake_turn(CCW);
    } else if (difference > 0) {
        snake_turn(CW);
    } else {
        return PASS;
    }

    return RENDER;
}

const struct ui_page pSNAKE = {
    .title = "Snake",
    .on_open = on_open,
    .on_render = on_render,
    .on_loop = on_loop,
    .on_close = on_close,
    .on_button = on_button,
    .on_encoder = on_encoder,
};