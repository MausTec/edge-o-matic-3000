#include "ui/ui.h"
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "page:snake";

#define SNAKE_CUBE_SIZE 3
#define SNAKE_PADDING_SIZE 1

#define GAME_TABLE_WIDTH  (eom_hal_get_display_width() / (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE))
#define GAME_TABLE_HEIGHT (eom_hal_get_display_height() / (SNAKE_CUBE_SIZE + SNAKE_PADDING_SIZE))

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
    node->point.x = x;
    node->point.y = y;
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
    
    while (ptr != NULL) {
        if (p->x == ptr->point.x && p->y == ptr->point.y) {
            return idx;
        }

        ptr = ptr->next;
        idx++;
    }

    return -1;
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
        prev->next = NULL;
        free(tail);
        Game->length--;
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
    return 500 + (Game->length * 250);
}

static int snake_move(void) {
    if (Game == NULL || Game->head == NULL) return 0;

    struct snake_point *p = &Game->head->point;

    switch (Game->direction) {
        case UP:
        snake_add_node(p->x, p->y + 1);
        break;

        case DOWN:
        snake_add_node(p->x, p->y - 1);
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
    }

    if (Game->state == GAME_RUNNING) {
        Game->state = GAME_PAUSED;
    }
}

static void on_open(struct ui_page *p) {
    snake_init_game();
}

static void on_render(u8g2_t *display, struct ui_page *p) {
    if (Game == NULL) return;

    ESP_LOGI(TAG, "Rendering Snake:");
    ESP_LOGI(TAG, "State: %d", Game->state);
    ESP_LOGI(TAG, "Length: %d, Direction: %d", Game->length, Game->direction);
    ESP_LOGI(TAG, "Head: (%d,%d), Food: (%d,%d)", Game->head->point.x, Game->head->point.y, Game->food.x, Game->food.y);
}

static void on_loop(struct ui_page *p) {
    if (Game == NULL) return;
    int64_t millis = esp_timer_get_time();

    if (millis - Game->last_tick >= snake_calculate_delay_ms()) {
        Game->last_tick = millis;

        if (Game->state != GAME_RUNNING) {
            return;
        }

        int consume = snake_move();

        if (consume >= 0) {
            Game->state = GAME_OVER;
        }
    }
}

static void on_close(struct ui_page *p) {
    snake_deinit_game();
}

static void on_button(eom_hal_button_t button, eom_hal_button_event_t evt, struct ui_page *p) {
    if (evt != EOM_HAL_BUTTON_PRESS) return;
    
    switch (button) {
        case EOM_HAL_BUTTON_BACK:
            snake_turn(CCW);
            break;
        case EOM_HAL_BUTTON_OK:
            snake_turn(CW);
            break;
        case EOM_HAL_BUTTON_MID:
            snake_game_pause();
            break;
        default:
            break;
    }
}

static void on_encoder(int difference, struct ui_page *p) {
    if (Game == NULL) return;

    if (difference < 0) {
        snake_turn(CCW);
    } else if (difference > 0) {
        snake_turn(CW);
    }
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