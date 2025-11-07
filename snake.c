#include <gb/gb.h>
#include <stdint.h>

// ========== SIMPLE RNG (no srand/rand needed) ==========
static uint16_t rng_state = 0xACE1u;
uint8_t rand8(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return (uint8_t)(rng_state >> 8);
}

// ========== GAME CONSTANTS ==========
#define GRID_W 20 // 160 / 8
#define GRID_H 18 // 144 / 8

#define MAX_SPRITES 40
#define FOOD_SPRITE_INDEX 39
#define MAX_SNAKE (FOOD_SPRITE_INDEX) // 39 segments max

#define FRAME_DELAY 6 // frames between movement steps

// ========== TILE DATA ==========
const uint8_t tile_filled[16] = {
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00};

// ========== GAME STATE ==========
uint8_t snake_x[MAX_SNAKE];
uint8_t snake_y[MAX_SNAKE];
uint8_t snake_len;

enum
{
    DIR_UP = 0,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT
};
uint8_t dir = DIR_RIGHT;

uint8_t food_x, food_y;

// ========== UTILITY FUNCTIONS ==========

void place_sprite_on_grid(uint8_t index, uint8_t gx, uint8_t gy)
{
    uint8_t px = gx * 8 + 8;
    uint8_t py = gy * 8 + 16;
    move_sprite(index, px, py);
}

// place food somewhere random
void place_food_random()
{
    food_x = rand8() % GRID_W;
    food_y = rand8() % GRID_H;
    place_sprite_on_grid(FOOD_SPRITE_INDEX, food_x, food_y);
}

// setup snake in center
void init_snake()
{
    snake_len = 3;
    uint8_t cx = GRID_W / 2;
    uint8_t cy = GRID_H / 2;
    for (uint8_t i = 0; i < snake_len; ++i)
    {
        snake_x[i] = cx - (snake_len - 1) + i;
        snake_y[i] = cy;
    }
    dir = DIR_RIGHT;
}

// draw all segments
void draw_snake()
{
    for (uint8_t i = 0; i < snake_len; ++i)
    {
        set_sprite_tile(i, 0);
        place_sprite_on_grid(i, snake_x[i], snake_y[i]);
    }
}

// update snake position + wrapping
void update_snake_position()
{
    for (uint8_t i = 0; i < snake_len - 1; ++i)
    {
        snake_x[i] = snake_x[i + 1];
        snake_y[i] = snake_y[i + 1];
    }

    int8_t hx = snake_x[snake_len - 1];
    int8_t hy = snake_y[snake_len - 1];

    if (dir == DIR_UP)
        hy--;
    else if (dir == DIR_DOWN)
        hy++;
    else if (dir == DIR_LEFT)
        hx--;
    else if (dir == DIR_RIGHT)
        hx++;

    if (hx < 0)
        hx = GRID_W - 1;
    else if (hx >= GRID_W)
        hx = 0;
    if (hy < 0)
        hy = GRID_H - 1;
    else if (hy >= GRID_H)
        hy = 0;

    snake_x[snake_len - 1] = (uint8_t)hx;
    snake_y[snake_len - 1] = (uint8_t)hy;
}

// grow snake by 1 segment
void grow_snake()
{
    if (snake_len >= MAX_SNAKE)
        return;
    for (int i = snake_len; i > 0; --i)
    {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }
    snake_len++;
}

// read input for direction
void handle_input()
{
    uint8_t j = joypad();
    if (j & J_UP)
        dir = DIR_UP;
    else if (j & J_DOWN)
        dir = DIR_DOWN;
    else if (j & J_LEFT)
        dir = DIR_LEFT;
    else if (j & J_RIGHT)
        dir = DIR_RIGHT;
}

// ========== MAIN LOOP ==========
void main(void)
{
    rng_state = DIV_REG; // seed RNG with hardware divider

    set_sprite_data(0, 1, tile_filled);

    for (uint8_t i = 0; i < MAX_SPRITES; ++i)
    {
        set_sprite_tile(i, 0);
        move_sprite(i, 0, 0);
    }

    SHOW_SPRITES;
    DISPLAY_ON;

    init_snake();
    place_food_random();
    draw_snake();
    place_sprite_on_grid(FOOD_SPRITE_INDEX, food_x, food_y);

    uint16_t frame_counter = 0;

    while (1)
    {
        wait_vbl_done();
        frame_counter++;

        handle_input();

        if ((frame_counter % FRAME_DELAY) == 0)
        {
            update_snake_position();

            // check if head hits food
            if (snake_x[snake_len - 1] == food_x &&
                snake_y[snake_len - 1] == food_y)
            {
                grow_snake();
                place_food_random();
            }

            draw_snake();
        }
    }
}
