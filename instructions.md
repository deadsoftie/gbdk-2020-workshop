# Installation

## SDK

The actual installation of the SDK itself is fairly straightforward. The repository is open-source: <https://github.com/gbdk-2020/gbdk-2020>. The latest release build can be downloaded from this page. The zip file can then be extracted to the desired location.

## Make

GBDK runs through LCC (Local C Compiler) which has to be built platform specific based on which OS it runs on. Therefore running it through WSL won’t work since executables don’t usually work the way we want them to on the subsystem. It is best to install make on your native environment to get the best setup. For this workshop we are going to directly use the new windows package manager so just open up the terminal and paste the following command.

```text
winget install ezwinports.make
```

After restarting the terminal make sure that the installation was correct by checking the version.

### 1) Includes and RNG

```c
#include <gb/gb.h>
#include <stdint.h>
```

- `gb/gb.h` gives Game Boy specific functions (sprites, joypad, etc.).
- `stdint.h` gives fixed-size number types like `uint8_t`.

```c
static uint16_t rng_state = 0xACE1u;
uint8_t rand8(void) {
    rng_state = rng_state * 1103515245u + 12345u;
    return (uint8_t)(rng_state >> 8);
}
```

- `rand8()` is a small, self-contained pseudo-random generator.
- We shift (`>> 8`) and return 8 bits to get `0..255`. It’s deterministic, compact, and doesn’t need library `srand`.

### Rand8 — what's going on (step-by-step)

We use a small **linear congruential generator (LCG)** because the Game Boy environment doesn’t provide `srand()`/`rand()` reliably.

Code:

```c
static uint16_t rng_state = 0xACE1u;

uint8_t rand8(void) {
    rng_state = rng_state * 1103515245u + 12345u;
    return (uint8_t)(rng_state >> 8);
}
```

Step explanation:

1. `rng_state` is a 16-bit number that holds the RNG’s internal state.
2. Each call updates `rng_state` with the formula `state = state*mult + add`. This is the LCG recurrence. The constants `1103515245` and `12345` are common LCG parameters.
3. We return the top 8 bits `((rng_state >> 8) & 0xFF)` as an `uint8_t`. That gives numbers 0–255.
4. To vary sequences between runs, we seed `rng_state` at program start, e.g. `rng_state = DIV_REG;` (DIV_REG changes rapidly after boot).
5. LCGs are deterministic — same seed → same sequence — but good enough for food placement.

### 2) Constants and tile data

```c
#define GRID_W 20
#define GRID_H 18
#define FOOD_SPRITE_INDEX 39
#define FRAME_DELAY 6
const uint8_t tile_filled[16] = { ... };
```

- `GRID_W` and `GRID_H` are the number of cells horizontally and vertically.
- `FOOD_SPRITE_INDEX` reserves one hardware sprite for the food (Game Boy hardware supports 40 sprites).
- `FRAME_DELAY` controls speed — smaller = faster.
- `tile_filled` is the pixel data for a filled 8×8 sprite (both bitplanes set -> solid).

<aside>
💡

Visually show the math of how we got to 20 and 18 (160/8 and 144/8).

</aside>

### Why do we upload tile data (`set_sprite_data`)?

Game Boy sprites are stored as binary tile data (the pixels). You must load the pattern (tile) into video RAM before using it.

Key points:

1. Each 8×8 tile on the Game Boy uses **2 bitplanes**, so 16 bytes per tile. `tile_filled[16]` holds those bytes.
2. `set_sprite_data(0, 1, tile_filled);` tells the system: put 1 tile starting at tile index 0 using the provided bytes.
3. `set_sprite_tile(sprite_index, tile_number);` binds a sprite to that tile pattern.
4. Without loading a tile, the sprite has no pixels (or garbage).

Simple mental model: tile data = the *stencil* (shape and pixels), sprite = an instance of that stencil placed on screen.

Bitplanes are **individual layers of binary data that together form an image, where each layer corresponds to a specific bit position for every pixel**. For example, a 16-bit image can be thought of as 16 separate bitplanes, each one a 1-bit-per-pixel image that stores a single bit for every pixel in the original image. This layered approach was used in older graphics systems to allow for special effects like transparency by manipulating individual bitplanes.

![image.png](attachment:cbf0c6ef-792a-4bc9-9a1e-d9e099e4ed75:image.png)

### 3) Game state variables

```c
uint8_t snake_x[MAX_SNAKE];
uint8_t snake_y[MAX_SNAKE];
uint8_t snake_len;
enum { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT };
uint8_t dir;
uint8_t food_x, food_y;
```

- `snake_x[i]` and `snake_y[i]` store each segment’s grid position.
- `snake_len` is the current length.
- `dir` stores current direction.
- `food_x`/`food_y` are the food grid coordinates.

### 4) place_sprite_on_grid()

```c
void place_sprite_on_grid(uint8_t index, uint8_t gx, uint8_t gy) {
    uint8_t px = gx * 8 + 8;
    uint8_t py = gy * 8 + 16;
    move_sprite(index, px, py);
}
```

- Converts grid coordinates `(gx,gy)` to pixel coordinates for `move_sprite`.
- The `+8` and `+16` offsets are because Game Boy’s sprite drawing origin is offset (standard practice in GBDK).

<aside>
💡

Visually show a grid and draw how (gx, gy) converts to (px, py).

</aside>

### Grid vs pixel coordinates — why convert and how

We think in **grid cells** (20×18) because it matches 8×8 sprite tiles and is easier to reason about. The hardware uses pixels.

Math:

- Screen is 160 × 144 pixels.
- Cell size = 8 pixels → GRID_W = 160/8 = 20, GRID_H = 144/8 = 18.

Conversion in `place_sprite_on_grid()`:

```c
uint8_t px = gx * 8 + 8;
uint8_t py = gy * 8 + 16;
move_sprite(index, px, py);
```

Why the `+8` and `+16` offsets?

- GBDK sprite positions are relative to the Game Boy display origin plus an internal sprite offset (common pattern). These offsets are empirical and standard to position 8×8 sprites correctly on the visible screen. If you remove them sprites may be drawn shifted or off-screen.

Visual example:

- Grid cell (0,0) -> pixel at (8,16)
- Grid cell (1,0) -> pixel at (16,16) etc.

### 5) place_food_random()

```c
void place_food_random() {
    food_x = rand8() % GRID_W;
    food_y = rand8() % GRID_H;
    place_sprite_on_grid(FOOD_SPRITE_INDEX, food_x, food_y);
}
```

- Chooses a random cell using `rand8()` and places the food sprite there.
- Note: naive version — it may place food on the snake. (Good future exercise.)

<aside>
💡

Talk about why we are using the naive version, and how this can be improved on.

</aside>

### 6) init_snake()

```c
void init_snake() {
    snake_len = 3;
    uint8_t cx = GRID_W / 2;
    uint8_t cy = GRID_H / 2;
    for (uint8_t i = 0; i < snake_len; ++i) {
        snake_x[i] = cx - (snake_len - 1) + i;
        snake_y[i] = cy;
    }
    dir = DIR_RIGHT;
}
```

- Places a 3-segment snake in the center, horizontally.
- Sets initial direction to the right.

### 7) draw_snake()

```c
void draw_snake() {
    for (uint8_t i = 0; i < snake_len; ++i) {
        set_sprite_tile(i, 0);
        place_sprite_on_grid(i, snake_x[i], snake_y[i]);
    }
}
```

- Assigns tile 0 to each sprite and moves each sprite to its grid cell.
- Sprite index `i` corresponds to segment `i`.

### 8) update_snake_position()

```c
void update_snake_position() {
    for (uint8_t i = 0; i < snake_len - 1; ++i) {
        snake_x[i] = snake_x[i + 1];
        snake_y[i] = snake_y[i + 1];
    }
    int8_t hx = snake_x[snake_len - 1];
    int8_t hy = snake_y[snake_len - 1];

    if (dir == DIR_UP) hy--;
    else if (dir == DIR_DOWN) hy++;
    else if (dir == DIR_LEFT) hx--;
    else if (dir == DIR_RIGHT) hx++;

    if (hx < 0) hx = GRID_W - 1;
    else if (hx >= GRID_W) hx = 0;
    if (hy < 0) hy = GRID_H - 1;
    else if (hy >= GRID_H) hy = 0;

    snake_x[snake_len - 1] = (uint8_t)hx;
    snake_y[snake_len - 1] = (uint8_t)hy;
}
```

- Shifts body positions forward (each segment takes the position of the next one), then moves the head.
- Uses wrapping — if head leaves screen on one side, it appears on the other.

<aside>
💡

Explain this one on the whiteboard.

</aside>

### Snake position method — how shifting works (detailed)

We store snake segments in arrays `snake_x[]` and `snake_y[]`. The convention in this code: **head is the last element** (`snake_len - 1`), tail is index 0.

Why this way? It makes shifting logic simple:

When the snake moves:

1. Shift every element toward index 0: `snake_x[i] = snake_x[i+1]` — this makes each segment take the position of the next segment (closer to the head).
2. Compute new head position based on direction and put it in `snake_x[snake_len-1]`.

Concrete example:

- Before move: indexes
  - [0] tail = (3,5)
  - [1] = (4,5)
  - [2] head = (5,5)
- After shifting step:
  - [0] = (4,5) (was [1])
  - [1] = (5,5) (was [2])
- Now compute new head (if dir = right → head x+1): head becomes (6,5) and is stored in [2].

This sliding window is like moving conveyor belts: everything moves forward, then the head advances.

### 9) grow_snake()

```c
void grow_snake() {
    if (snake_len >= MAX_SNAKE) return;
    for (int i = snake_len; i > 0; --i) {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }
    snake_len++;
}
```

- Inserts one extra segment at the tail by shifting everything one place and increasing `snake_len`. The new tail initially has the same co-ords as the old tail (so it visually grows).

<aside>
💡

Ask why are we using the for-loop this way?

</aside>

# Why `grow_snake()` uses the for-loop that copies from right to left

Code:

```c
void grow_snake() {
    if (snake_len >= MAX_SNAKE) return;
    for (int i = snake_len; i > 0; --i) {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }
    snake_len++;
}
```

Step-by-step why it works:

1. We need to add one new element at index 0..snake_len (we choose to insert a copy of the tail).
2. To avoid overwriting existing values, we copy **from the back to the front**:
    - Move the current tail value into the next index, and so on, until we’ve shifted the whole array one position to the right.
3. After the loop: index 0 is a duplicate of old index 0 (old tail), and index `snake_len` now contains the old head.
4. We then `snake_len++` — a new logical segment exists (the tail appears to grow, because the old tail’s position was duplicated and then on the next update the body will move and reveal the new cell).

Why copy backwards? If we copied forwards (`i=0..snake_len-1`) we would overwrite data we still need to copy. Back-to-front avoids that.

Analogy: inserting an item into an array — always move later elements first.

### 10) handle_input()

```c
void handle_input() {
    uint8_t j = joypad();
    if (j & J_UP) dir = DIR_UP;
    else if (j & J_DOWN) dir = DIR_DOWN;
    else if (j & J_LEFT) dir = DIR_LEFT;
    else if (j & J_RIGHT) dir = DIR_RIGHT;
}
```

- Reads the controller and updates direction. This version allows quick reversing (you can add a rule to prevent 180° turns later).

<aside>
💡

This code also has issues, what happens when the input immediately reverses the direction of the snake head?

</aside>

### How to prevent immediate reversal (fix head-turn bug)

Problem: If snake goes RIGHT, pressing LEFT should not immediately make it go left — that would make the head collide with the next segment.

Simple fix: when processing input, ignore inputs that are the opposite of current direction.

```c
uint8_t opposite_dir(uint8_t d) {
    if (d == DIR_UP) return DIR_DOWN;
    if (d == DIR_DOWN) return DIR_UP;
    if (d == DIR_LEFT) return DIR_RIGHT;
    return DIR_LEFT; // if DIR_RIGHT
}

void handle_input_safe() {
    uint8_t j = joypad();
    uint8_t new_dir = dir; // default keep current
    if (j & J_UP) new_dir = DIR_UP;
    else if (j & J_DOWN) new_dir = DIR_DOWN;
    else if (j & J_LEFT) new_dir = DIR_LEFT;
    else if (j & J_RIGHT) new_dir = DIR_RIGHT;

    // Ignore if trying to reverse immediately
    if (new_dir != opposite_dir(dir)) {
        dir = new_dir;
    }
}
```

### 11) main()

```c
void main(void) {
    rng_state = DIV_REG;

    set_sprite_data(0, 1, tile_filled);

    for (uint8_t i = 0; i < MAX_SPRITES; ++i) {
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

    while (1) {
        wait_vbl_done();
        frame_counter++;

        handle_input();

        if ((frame_counter % FRAME_DELAY) == 0) {
            update_snake_position();

            if (snake_x[snake_len - 1] == food_x &&
                snake_y[snake_len - 1] == food_y) {
                grow_snake();
                place_food_random();
            }

            draw_snake();
        }
    }
}
```

- `rng_state = DIV_REG` seeds RNG with hardware divider (varies over time).
- `set_sprite_data` uploads the single 8×8 tile into VRAM.
- `SHOW_SPRITES` and `DISPLAY_ON` turn the sprites and display on.
- The `while(1)` loop is the game loop:
  - `wait_vbl_done()` waits for the vertical blank (safe time to update graphics).
  - Every `FRAME_DELAY` frames we move the snake and check for eating food.
  - If the head and food are on the same cell, we `grow_snake()` and place new food.

  ### Explanation

`while (1) { ... }` — runs forever (until ROM reset).

Each loop iteration:

- `wait_vbl_done();` — wait for vertical blank (safe time to update sprite positions).
- `frame_counter++;` — count frames.
- `handle_input();` — read joypad and maybe update `dir`.
- Every `FRAME_DELAY` frames: perform movement step:
  - `update_snake_position();` — shift body, compute new head, wrap edges.
  - Check if head equals food coordinates:
    - If yes: `grow_snake(); place_food_random();`
  - `draw_snake();` — reposition sprites based on the arrays.

## Safe food placement - Optional

```c
uint8_t is_on_snake(uint8_t x, uint8_t y) {
    for (uint8_t i = 0; i < snake_len; ++i)
        if (snake_x[i] == x && snake_y[i] == y) return 1;
    return 0;
}

void place_food_random_safe() {
    uint8_t x, y;
    do {
        x = rand8() % GRID_W;
        y = rand8() % GRID_H;
    } while (is_on_snake(x, y));
    food_x = x; food_y = y;
    place_sprite_on_grid(FOOD_SPRITE_INDEX, food_x, food_y);
}
```

## Prevention immediate reversal

```c
uint8_t opposite_dir(uint8_t d) {
    return (d ^ 2); // DIR_UP(0)^2 -> 2 (DOWN), DIR_RIGHT(1)^2 -> 3 (LEFT) etc
}

void handle_input_safe() {
    uint8_t j = joypad();
    uint8_t new_dir = dir;
    if (j & J_UP) new_dir = DIR_UP;
    else if (j & J_DOWN) new_dir = DIR_DOWN;
    else if (j & J_LEFT) new_dir = DIR_LEFT;
    else if (j & J_RIGHT) new_dir = DIR_RIGHT;

    if (new_dir != opposite_dir(dir)) dir = new_dir;
}
```
