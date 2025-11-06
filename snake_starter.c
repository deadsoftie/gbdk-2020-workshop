// snake_starter.c — Workshop Starter Template
// Step-by-step GBDK-2020 Snake Game Project

#include <gb/gb.h>
#include <stdint.h>

// ========== STEP 1: SIMPLE RNG ==========
// This will give us random numbers for food placement.
static uint16_t rng_state = 0xACE1u;

uint8_t rand8(void)
{
    // TODO: Fill in RNG logic
    // hint: rng_state = rng_state * 1103515245u + 12345u;
    // return (uint8_t)(rng_state >> 8);
    return 0;
}

// ========== STEP 2: GAME CONSTANTS ==========
#define GRID_W
#define GRID_H

#define MAX_SPRITES
#define FOOD_SPRITE_INDEX
#define MAX_SNAKE (FOOD_SPRITE_INDEX) // 39 segments max
#define FRAME_DELAY                   // controls speed

// ========== STEP 3: TILE DATA ==========
const uint8_t tile_filled[16] = {
    // TODO: Fill this tile so that it becomes a solid white square
    // hint: each row has two bytes, 0xFF and 0x00 make a filled row
};

// ========== STEP 4: GAME STATE VARIABLES ==========
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

// ========== STEP 5: BASIC UTILITY FUNCTIONS ==========

// TODO: Implement this helper
// Converts grid coordinates (gx,gy) to pixel coordinates for move_sprite()
void place_sprite_on_grid(uint8_t index, uint8_t gx, uint8_t gy)
{
    // hint: px = gx * 8 + 8; py = gy * 8 + 16;
}

// TODO: Random food placement
void place_food_random()
{
    // food_x = rand8() % GRID_W;
    // food_y = rand8() % GRID_H;
    // place_sprite_on_grid(FOOD_SPRITE_INDEX, food_x, food_y);
}

// ========== STEP 6: SNAKE SETUP ==========

// Initialize snake in the center
void init_snake()
{
    snake_len = 3; // start small
    uint8_t cx = GRID_W / 2;
    uint8_t cy = GRID_H / 2;

    // TODO: Fill snake_x and snake_y arrays so it starts horizontally
    // for (...) snake_x[i] = ..., snake_y[i] = ...
    dir = DIR_RIGHT;
}

// ========== STEP 7: DRAWING ==========
void draw_snake()
{
    // TODO: loop through each segment and move sprite to its position
    // hint: use set_sprite_tile(i, 0); and place_sprite_on_grid(i, snake_x[i], snake_y[i]);
}

// ========== STEP 8: MOVEMENT ==========
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

// ========== STEP 9: GROWTH ==========
void grow_snake()
{
    // TODO: extend the snake length by one
}

// ========== STEP 10: INPUT ==========
void handle_input()
{
    // TODO: Read joypad() and set direction (dir)
    // hint: if (j & J_UP) dir = DIR_UP; ...
}

// ========== STEP 11: MAIN LOOP ==========
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

            // TODO: detect food eaten (head == food)
            // if so, grow_snake(); place_food_random();

            draw_snake();
        }
    }
}
