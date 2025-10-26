// include libraries
#include <stdbool.h>
#include <time.h>
#include <graphx.h>
#include <keypadc.h>
#include <debug.h>
#include "gfx/gfx.h"

// map settings
#define MAP_WIDTH 10
#define MAP_HEIGHT 10
// ticking settings
#define TICK_WAIT 0.2
#define MAX_INTENTS 128
// input settings
#define KEY_DELAY_INIT 15
#define KEY_DELAY_REPEAT 2
// drawing settings
#define INVENTORY_DRAW_X_OFFSET 280

// define vars
unsigned int offset_x = 0, offset_y = 0;

unsigned int cursor_x = 0, cursor_y = 0;
unsigned int key_delay = 0;

clock_t tick_start = 0;
unsigned long tick_count = 0;

gfx_sprite_t *tileset_90_tiles[9];
gfx_sprite_t *tileset_180_tiles[9];
gfx_sprite_t *tileset_270_tiles[9];

int selected_cell_type = 1;
int selected_rotation = 0;

typedef struct {
    uint8_t x, y;
    int8_t dx, dy;
} Intent;
Intent intents[MAX_INTENTS];
int intent_count = 0;

typedef enum {
    STOPPED,
    PLAYING,
    PAUSED
} GameStates;
GameStates game_state = STOPPED;

typedef enum {
    BLANK,
    CELL_IMMOBILE,
    CELL_PUSH,
    CELL_SLIDE,
    CELL_MOVER,
    CELL_CW_ROTATOR,
    CELL_CCW_ROTATOR,
    CELL_GENERATOR,
    CELL_ENEMY,
    CELL_TRASH
} CellTypes;

uint8_t map_setup[MAP_WIDTH * MAP_HEIGHT] = {0};
uint8_t map_play[MAP_WIDTH * MAP_HEIGHT] = {0};
uint8_t *map = map_setup;
bool moved_flag[MAP_WIDTH * MAP_HEIGHT] = {false};

// helper funcs
uint8_t get_cell(uint8_t x, uint8_t y)
{
    if (x >= MAP_WIDTH || y >= MAP_HEIGHT) return 0;
    return map[y * MAP_WIDTH + x];
}

void set_cell(uint8_t x, uint8_t y, uint8_t value)
{
    if (x >= MAP_WIDTH || y >= MAP_HEIGHT) return;
    map[y * MAP_WIDTH + x] = value;
}

int8_t calc_cell_type(uint8_t id) // calculates cell type
{
    uint8_t high_nib = (id >> 4) & 0x0F;
    uint8_t low_nib = id & 0x0F;
    return low_nib + high_nib / 4 * 16;
}

int8_t calc_cell_rotation(uint8_t id) // calculates cell rotation
{
    uint8_t high_nib = (id >> 4) & 0x0F;
    return high_nib % 4;
}

uint8_t calc_cell_id(uint8_t type, uint8_t rotation) {
    uint8_t high_high_bits = type / 16;
    uint8_t high_nib = (high_high_bits << 2) | (rotation & 0x03);
    uint8_t low_nib = type % 16;
    return (high_nib << 4) | low_nib;
}

bool can_move(int x, int y, int dx, int dy)
{
    uint8_t cell_id = get_cell(x, y);
    uint8_t cell_type = calc_cell_type(cell_id);

    if (cell_type == CELL_IMMOBILE) return false;
    else if (cell_type == CELL_SLIDE) {
        uint8_t cell_rotation = calc_cell_rotation(cell_id);
        if ((dx != 0 && (cell_rotation == 1 || cell_rotation == 3)) ||
            (dy != 0 && (cell_rotation == 0 || cell_rotation == 2)))
        {
            return false;
        }
    }
    return true;
}

// draw funcs
void draw_cell(int type, int rotation, int x, int y)
{
    switch (rotation)
    {
    case 0:
        gfx_Sprite(tileset_0_tiles[type], x, y);
        break;
    case 1:
        gfx_Sprite(tileset_90_tiles[type], x, y);
        break;
    case 2:
        gfx_Sprite(tileset_180_tiles[type], x, y);
        break;
    case 3:
        gfx_Sprite(tileset_270_tiles[type], x, y);
        break;
    }
}

// action funcs
int want_move_cell_depth(int x, int y, int dx, int dy, int depth)
{
    if (depth > 64) return -1; // stop ridiculously long pushes

    uint8_t from_cell_id = get_cell(x, y);
    uint8_t from_cell_type = calc_cell_type(from_cell_id);
    if (from_cell_type == CELL_MOVER && depth == 0 && moved_flag[y * MAP_WIDTH + x]) {
        return -1;
    }
    if (from_cell_type == BLANK) return 0;
    if (from_cell_type == CELL_ENEMY)
    {
        set_cell(x, y, 0);
        return 1;
    }
    
    int to_x = x + dx, to_y = y + dy;
    if (to_x < 0 || to_y < 0 || to_x >= MAP_WIDTH || to_y >= MAP_HEIGHT)
    return -1; // hit boundary

    ///////
    uint8_t to_cell_id = get_cell(to_x, to_y);
    uint8_t to_cell_type = calc_cell_type(to_cell_id);
    if (can_move(to_x, to_y, dx, dy))
    {
        if (to_cell_type == CELL_TRASH) {
            set_cell(x, y, 0);
            return 0;
        } else if (to_cell_type == CELL_ENEMY) {
            set_cell(to_x, to_y, 0);
            set_cell(x, y, 0);
            return 0;
        } else {
            int moved = want_move_cell_depth(to_x, to_y, dx, dy, depth + 1);
            if (moved == 0) {
                set_cell(x, y, BLANK);
                set_cell(to_x, to_y, from_cell_id);
                if (from_cell_type == CELL_MOVER && depth == 0) {
                    moved_flag[to_y * MAP_WIDTH + to_x] = true;
                }
                return 0;
            }
        }
    }

    return -1;
}

int want_move_cell(int x, int y, int dx, int dy)
{
    return want_move_cell_depth(x, y, dx, dy, 0);
}

void want_rotate_cell(int x, int y, bool is_ccw)
{
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return; // bounds check
    
    uint8_t cell_id = get_cell(x, y);
    uint8_t cell_type = calc_cell_type(cell_id);
    if (cell_type == BLANK) return;
    uint8_t cell_rotation = calc_cell_rotation(cell_id);

    if (is_ccw)
    {
        cell_rotation --;
        if (cell_rotation < 0) cell_rotation = 3;
    } else
    {
        cell_rotation ++;
        if (cell_rotation > 3) cell_rotation = 0;
    }

    set_cell(x, y, calc_cell_id(cell_type, cell_rotation));
}

void want_rotate_neighbors(int x, int y, bool is_ccw)
{
    want_rotate_cell(x, y - 1, is_ccw);    
    want_rotate_cell(x, y + 1, is_ccw);    
    want_rotate_cell(x - 1, y, is_ccw);    
    want_rotate_cell(x + 1, y, is_ccw);    
}

void game_tick()
{
    // clean stuff
    for (unsigned int i = 0; i < sizeof moved_flag; i++)
    {
        moved_flag[i] = false;
    }

    // process generators
    intent_count = 0;
    for (unsigned int i = 0; i < MAP_WIDTH * MAP_HEIGHT; i++)
    {
        uint8_t cell_id = map[i];
        if (calc_cell_type(cell_id) != CELL_GENERATOR) continue;
        
        Intent cell_intent;
        uint8_t cell_rotation = calc_cell_rotation(cell_id);
        cell_intent.x = i % MAP_WIDTH;
        cell_intent.y = i / MAP_WIDTH;
        switch (cell_rotation)
        {
        case 0:
            cell_intent.dx = 1;
            cell_intent.dy = 0;
            break;
        case 1:
            cell_intent.dx = 0;
            cell_intent.dy = 1;
            break;
        case 2:
            cell_intent.dx = -1;
            cell_intent.dy = 0;
            break;
        case 3:
            cell_intent.dx = 0;
            cell_intent.dy = -1;
            break;
        }
        if (get_cell(cell_intent.x - cell_intent.dx, cell_intent.y - cell_intent.dy) != 0)
        {
            if (can_move(cell_intent.x + cell_intent.dx, cell_intent.y + cell_intent.dy, cell_intent.dx, cell_intent.dy))
                intents[intent_count++] = cell_intent;
        }
    }
    for (int i = 0; i < intent_count; i++)
    {
        Intent c_intent = intents[i];
        if (want_move_cell(c_intent.x + c_intent.dx, c_intent.y + c_intent.dy, c_intent.dx, c_intent.dy) == 0)
            set_cell(c_intent.x + c_intent.dx, c_intent.y + c_intent.dy, get_cell(c_intent.x - c_intent.dx, c_intent.y - c_intent.dy));
    }
    
    // process rotators
    for (unsigned int i = 0; i < MAP_WIDTH * MAP_HEIGHT; i++)
    {
        uint8_t cell_id = map[i];
        if (cell_id == 0) continue;  // skip empty cells
        
        uint8_t cell_type = calc_cell_type(cell_id);
        switch (cell_type)
        {
        case CELL_CW_ROTATOR:
            want_rotate_neighbors(i % MAP_WIDTH, i / MAP_WIDTH, false);
            break;
        case CELL_CCW_ROTATOR:
            want_rotate_neighbors(i % MAP_WIDTH, i / MAP_WIDTH, true);
            break;
        }
    }

    // process movers
    intent_count = 0;
    for (unsigned int i = 0; i < MAP_WIDTH * MAP_HEIGHT; i++)
    {
        uint8_t cell_id = map[i];
        if (calc_cell_type(cell_id) != CELL_MOVER) continue;
        
        Intent cell_intent;
        uint8_t cell_rotation = calc_cell_rotation(cell_id);
        cell_intent.x = i % MAP_WIDTH;
        cell_intent.y = i / MAP_WIDTH;
        switch (cell_rotation)
        {
        case 0:
            cell_intent.dx = 1;
            cell_intent.dy = 0;
            break;
        case 1:
            cell_intent.dx = 0;
            cell_intent.dy = 1;
            break;
        case 2:
            cell_intent.dx = -1;
            cell_intent.dy = 0;
            break;
        case 3:
            cell_intent.dx = 0;
            cell_intent.dy = -1;
            break;
        }
        intents[intent_count++] = cell_intent;
    }
    for (int i = 0; i < intent_count; i++)
    {
        Intent c_intent = intents[i];
        want_move_cell(c_intent.x, c_intent.y, c_intent.dx, c_intent.dy);
    }

    tick_count ++;
}

// gamestate funcs
void start_play()
{
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            map_play[y * MAP_WIDTH + x] = map_setup[y * MAP_WIDTH + x];
        }
    }
    map = map_play;
    game_state = PLAYING;
}

void stop_play()
{
    map = map_setup;
    tick_count = 0;
    game_state = STOPPED;
}

void pause_play()
{
    map = map_play;
    game_state = PAUSED;   
}

void resume_play()
{
    game_state = PLAYING;
}

void step_play()
{
    if (game_state == STOPPED)
    {
        for (int y = 0; y < MAP_HEIGHT; y++)
        {
            for (int x = 0; x < MAP_WIDTH; x++)
            {
                map_play[y * MAP_WIDTH + x] = map_setup[y * MAP_WIDTH + x];
            }
        }
        map = map_play;
        game_state = PAUSED;
    }
    game_tick();
}

void begin()
{
    for (int i = 0; i < 9; i++)
    {
        gfx_sprite_t *sprite_in = tileset_0_tiles[i];

        gfx_sprite_t *sprite_90_out = gfx_MallocSprite(sprite_in->height, sprite_in->width);
        if (sprite_90_out == NULL) break;

        gfx_RotateSpriteC(sprite_in, sprite_90_out);
        tileset_90_tiles[i] = sprite_90_out;
        gfx_sprite_t *sprite_180_out = gfx_MallocSprite(sprite_in->width, sprite_in->height);
        if (sprite_180_out == NULL) break;

        gfx_RotateSpriteHalf(sprite_in, sprite_180_out);
        tileset_180_tiles[i] = sprite_180_out;
        gfx_sprite_t *sprite_270_out = gfx_MallocSprite(sprite_in->height, sprite_in->width);
        if (sprite_270_out == NULL) break;

        gfx_RotateSpriteCC(sprite_in, sprite_270_out);
        tileset_270_tiles[i] = sprite_270_out;
    }

    int draw_width = 16 * MAP_WIDTH, draw_height = 16 * MAP_HEIGHT;
    offset_x = (GFX_LCD_WIDTH - draw_width) / 4;
    offset_y = (GFX_LCD_HEIGHT - draw_height) / 2;

    gfx_SetColor(255);
    gfx_SetTextFGColor(19);

    // setup ticking
    tick_start = clock();
}

void end()
{
    // clear rotated sprites
    for (int i = 0; i < 9; i++) {
        free(tileset_90_tiles[i]);
        free(tileset_180_tiles[i]);
        free(tileset_270_tiles[i]);
    }
}

bool step()
{
    if (game_state == PLAYING)
    {
        // ticking
        clock_t now = clock();
        float elapsed = (float)(now - tick_start) / CLOCKS_PER_SEC;
        if (elapsed >= TICK_WAIT) {
            game_tick();
            tick_start = now;
        }
    }

    kb_Scan();

    if (kb_AnyKey()) {
        if (key_delay == 0 || key_delay >= KEY_DELAY_INIT) {
            if (game_state != PLAYING)
            {
                // move cursor
                if (kb_Data[7] & kb_Left && cursor_x > 0) cursor_x--;
                if (kb_Data[7] & kb_Right && cursor_x < MAP_WIDTH - 1) cursor_x++;
                if (kb_Data[7] & kb_Up && cursor_y > 0) cursor_y--;
                if (kb_Data[7] & kb_Down && cursor_y < MAP_HEIGHT - 1) cursor_y++;
            }

            // rotate cell
            if (kb_Data[1] & kb_Mode)
            {
                selected_rotation --;
                if (selected_rotation < 0) selected_rotation = 3;
            }
            if (kb_Data[1] & kb_Del)
            {
                selected_rotation ++;
                if (selected_rotation > 3) selected_rotation = 0;
            }

            // change gamestate
            if (kb_Data[1] & kb_Yequ)
            {
                if (game_state == STOPPED) start_play();
                else if (game_state == PAUSED) resume_play();
                else if (game_state == PLAYING) pause_play();
            }
            if (kb_Data[1] & kb_Window)
            {
                if (game_state != PLAYING) step_play();
            }
            if (kb_Data[1] & kb_Zoom)
            {
                if (game_state == PLAYING || game_state == PAUSED) stop_play();
            }

            // reset delay for repeat
            if (key_delay >= KEY_DELAY_INIT) {
                key_delay = KEY_DELAY_INIT - KEY_DELAY_REPEAT;
            }
        }
        key_delay++;

        // change selected cell
        if (kb_Data[3] & kb_1) selected_cell_type = 1;
        if (kb_Data[3] & kb_4) selected_cell_type = 4;
        if (kb_Data[3] & kb_7) selected_cell_type = 7;
        if (kb_Data[4] & kb_2) selected_cell_type = 2;
        if (kb_Data[4] & kb_5) selected_cell_type = 5;
        if (kb_Data[4] & kb_8) selected_cell_type = 8;
        if (kb_Data[5] & kb_3) selected_cell_type = 3;
        if (kb_Data[5] & kb_6) selected_cell_type = 6;
        if (kb_Data[5] & kb_9) selected_cell_type = 9;

        if (game_state != PLAYING)
        {
            if (kb_Data[1] & kb_2nd) set_cell(cursor_x, cursor_y, calc_cell_id(selected_cell_type, selected_rotation)); // paint on map
            if (kb_Data[2] & kb_Alpha) set_cell(cursor_x, cursor_y, 0x00); // erase on map
        }
    } else {
        key_delay = 0;
    }

    // exit program
    return (kb_Data[6] & kb_Clear) ? false : true;
}

void draw()
{
    gfx_FillScreen(0);

    for (int y = 0; y < MAP_HEIGHT; ++y)
    {
        for (int x = 0; x < MAP_WIDTH; ++x)
        {
            // draw background
            gfx_Sprite(background, x * 16 + offset_x, y * 16 + offset_y);

            // draw cells
            uint8_t id = get_cell(x, y);
            if (id != 0)
            {
                uint8_t cell_type = calc_cell_type(id);
                uint8_t rotation = calc_cell_rotation(id);

                // draw cell and apply rotation
                draw_cell(cell_type - 1, rotation, x * 16 + offset_x, y * 16 + offset_y);
            }
        }
    }

    // draw cursor
    if (game_state != PLAYING) gfx_Rectangle(cursor_x * 16 + offset_x, cursor_y * 16 + offset_y, 16, 16);

    // draw inventory
    for (unsigned int i = 0; i < 9; i++)
    {
        gfx_SetTextXY(INVENTORY_DRAW_X_OFFSET - 12, 20 + i * 24);
        gfx_PrintUInt(i + 1, 1);
        draw_cell(i, selected_rotation, INVENTORY_DRAW_X_OFFSET, 16 + i * 24);
    }
    gfx_Rectangle(INVENTORY_DRAW_X_OFFSET - 2, 14 + (selected_cell_type - 1) * 24, 20, 20);

    gfx_Sprite_NoClip(logo, 0, 0);

    // draw gamestate
    switch (game_state)
    {
    case STOPPED:
        gfx_Sprite(gamestate_icons_tiles[1], 16, 220);
        gfx_Sprite(gamestate_icons_tiles[3], 80, 220);
        break;
    case PLAYING:
        gfx_Sprite(gamestate_icons_tiles[2], 16, 220);
        gfx_Sprite(gamestate_icons_tiles[0], 144, 220);
        break;
    case PAUSED:
        gfx_Sprite(gamestate_icons_tiles[1], 16, 220);
        gfx_Sprite(gamestate_icons_tiles[3], 80, 220);
        gfx_Sprite(gamestate_icons_tiles[0], 144, 220);
        break;
    }

    // draw tick count
    gfx_PrintStringXY("tick:", 16, 210);
    gfx_SetTextXY(50, 210);
    gfx_PrintUInt(tick_count, 8);
}

int main()
{
    begin();

    // setup graphics
    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_SetTransparentColor(255);
    gfx_SetDrawBuffer();

    while (step())
    {
        draw();
        gfx_SwapDraw();
    }
    
    gfx_End();
    end();

    return 0;
}