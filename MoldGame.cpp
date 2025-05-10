#include "splashkit.h"
#include <queue>
using namespace std;

using std::to_string;

// Constants for map dimensions and tile sizes
const int MAX_MAP_ROWS = 50;
const int MAX_MAP_COLS = 50;
const int TILE_WIDTH = 10;
const int TILE_HEIGHT = 10;
const string GAME_TIMER = "Game Timer";

// Directions for 8-connected neighbors (up, down, left, right, and diagonals)
const int DX[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
const int DY[8] = {0, 0, -1, 1, -1, 1, -1, 1};

// Enum for different types of tiles
enum tile_kind
{
    NORMAL_TILE,
    MOLDY_TILE,
    FIX_TILE,
    BROKEN_TILE,
    BORDER_TILE
};

// Enum for different states of mold
enum mold_state
{
    PREPARE,
    SPREADING,
    DONE_SPREADING,
    FIXING
};

// Structure to represent a single tile
struct tile_data
{
    tile_kind kind;
};

// Structure to represent the map, which is a grid of tiles
struct map_data
{
    tile_data tiles[MAX_MAP_COLS][MAX_MAP_ROWS];
};

// Structure to represent the explorer, including the map and camera position
struct explorer_data
{
    map_data map;
    tile_kind editor_tile_kind;
    point_2d camera;
};

// Structure to represent a location on the map
struct location_data
{
    int c; // Column
    int r; // Row
};

// Structure to represent the mold and its behavior
struct mold_data
{
    // Location to start spreading from
    int start_r;
    int start_c;

    mold_state state; // Current state of the mold

    long appear_at_time;    // Time for the mold to appear
    long last_spread_time;  // Last time mold spread
    long time_to_start_fix; // Time to start fixing mold

    location_data spread[MAX_MAP_COLS * MAX_MAP_ROWS]; // Array to track spreads locations
    int spreads_count;                                 // Counter for the number of spreads

    queue<pair<int, int>> q; // Queue for BFS during spreading

    // Check if it's time for the mold to appear
    bool is_time_to_appear(long current_time) const
    {
        return current_time > appear_at_time;
    }

    // Check if it's time for the mold to spread
    bool is_time_to_spread(long current_time) const
    {
        return current_time >= last_spread_time + 1000;
    }
};

// Initialize a location
location_data init_loc(int c, int r)
{
    location_data loc;
    loc.c = c;
    loc.r = r;
    return loc;
}

// Initialize mold with starting position and default values
mold_data init_mold(int start_c, int start_r)
{
    mold_data mold;

    // Initialize location to start spreading from
    mold.start_c = start_c;
    mold.start_r = start_r;

    mold.state = PREPARE; // Initialize state

    mold.appear_at_time = timer_ticks(GAME_TIMER) + 1000 + rnd(0, 2000); // Random time between 1 and 3 seconds
    mold.last_spread_time = mold.appear_at_time;                         // Initialize last spread time
    mold.time_to_start_fix = 0;                                          // Initialize time to start fix

    // Initialize the spread locations
    for (int i = 0; i < MAX_MAP_COLS * MAX_MAP_ROWS; i++)
    {
        mold.spread[i].c = -1;
        mold.spread[i].r = -1;
    }
    mold.spreads_count = 0;

    return mold;
}

// Function to spread mold to neighboring tiles
void spread_mold(map_data &map, mold_data &mold)
{
    int c = mold.q.front().first;
    int r = mold.q.front().second;
    mold.q.pop();

    // Check current spot's 8-connected neighbors
    for (int i = 0; i < 8; i++)
    {
        int nc = c + DY[i];
        int nr = r + DX[i];
        if (nr >= 0 && nr < MAX_MAP_ROWS && nc >= 0 && nc < MAX_MAP_COLS)
        {
            // Check if the neighbor is a normal tile
            if (map.tiles[nc][nr].kind == NORMAL_TILE)
            {
                mold.spread[mold.spreads_count++] = init_loc(nc, nr);
                map.tiles[nc][nr].kind = MOLDY_TILE;
                write_line("Mold spread to: " + to_string(nc) + ", " + to_string(nr));
                mold.q.push({nc, nr});
                mold.last_spread_time = timer_ticks(GAME_TIMER); // Update last spread time
            }
        }
    }

    // Check if the queue is empty, indicating that mold has finished spreading
    if (mold.q.empty())
    {
        write_line("Mold spread finished.");
        mold.time_to_start_fix = timer_ticks(GAME_TIMER) + 5000; // Set time to start fix
        mold.state = DONE_SPREADING;                             // Specify that spreading is done
    }
}

// Handle the lifecycle of mold (appearance, spreading, fixing, breaking)
void handle_mold_lifecycle(map_data &map, mold_data &mold)
{
    // Handle
    if (mold.state == PREPARE && mold.is_time_to_appear(timer_ticks(GAME_TIMER)))
    {
        mold.q.push({mold.start_c, mold.start_r});
        mold.spread[mold.spreads_count++] = init_loc(mold.start_c, mold.start_r);
        map.tiles[mold.start_c][mold.start_r].kind = MOLDY_TILE;
        write_line("Mold spread started at: " + to_string(mold.start_c) + ", " + to_string(mold.start_r));
        mold.state = SPREADING; // Set state to spreading
    }

    if (mold.state == SPREADING && mold.is_time_to_spread(timer_ticks(GAME_TIMER))) // Spread every second
    {
        spread_mold(map, mold);
    }

    // Handle fixing state of mold
    if (mold.state == DONE_SPREADING)
    {
        for (int i = 0; i < mold.spreads_count; i++)
        {
            map.tiles[mold.spread[i].c][mold.spread[i].r].kind = FIX_TILE;
        }
        mold.state = FIXING; // Set state to fixing
    }

    // Handle broken state of mold
    if (timer_ticks(GAME_TIMER) > mold.time_to_start_fix)
    {
        for (int i = 0; i < mold.spreads_count; i++)
        {
            if (map.tiles[mold.spread[i].c][mold.spread[i].r].kind == FIX_TILE)
            {
                map.tiles[mold.spread[i].c][mold.spread[i].r].kind = BROKEN_TILE;
            }
        }
    }
}

// Function to initialize the map with normal tiles
void init_map(map_data &map)
{
    for (int i = 0; i < MAX_MAP_COLS; i++)
    {
        for (int j = 0; j < MAX_MAP_ROWS; j++)
        {
            map.tiles[i][j].kind = NORMAL_TILE;
        }
    }
}

// Function to initialize the explorer data
void init_explorer(explorer_data &explorer)
{
    init_map(explorer.map);
    explorer.editor_tile_kind = NORMAL_TILE;
    explorer.camera = point_at(0, 0);
}

// Get the color corresponding to a tile kind
color color_for_tile_kind(tile_kind kind)
{
    switch (kind)
    {
    case NORMAL_TILE:
        return color_burly_wood();
    case MOLDY_TILE:
        return color_dark_olive_green();
    case FIX_TILE:
        return color_yellow_green();
    case BROKEN_TILE:
        return color_gray();
    case BORDER_TILE:
        return color_black();
    default:
        return color_white();
    }
}

// Draw a single tile at a specific position
void draw_tile(tile_data tile, int x, int y)
{
    fill_rectangle(color_for_tile_kind(tile.kind), x, y, TILE_WIDTH, TILE_HEIGHT);
}

// Draw the entire map based on the camera position
void draw_map(map_data &map, const point_2d &camera)
{
    int start_col = camera.x / TILE_WIDTH;
    int end_col = (camera.x + screen_width()) / TILE_WIDTH + 1;
    int start_row = camera.y / TILE_HEIGHT;
    int end_row = (camera.y + screen_height()) / TILE_HEIGHT + 1;

    if (start_col < 0)
        start_col = 0;
    if (start_row < 0)
        start_row = 0;

    for (int c = start_col; c < end_col && c < MAX_MAP_COLS; c++)
    {
        for (int r = start_row; r < end_row && r < MAX_MAP_ROWS; r++)
        {
            draw_tile(map.tiles[c][r], c * TILE_WIDTH, r * TILE_HEIGHT);
        }
    }
}

// Draw the explorer and the map
void draw_explorer(explorer_data &explorer)
{
    set_camera_position(explorer.camera);

    clear_screen(color_white());

    draw_map(explorer.map, explorer.camera);
    
    draw_rectangle(color_black(), explorer.camera.x, explorer.camera.y + 10, 50, 50);
    fill_rectangle(color_for_tile_kind(explorer.editor_tile_kind), explorer.camera.x + 10, explorer.camera.y + 20, 30, 30);
    draw_rectangle(color_black(), explorer.camera.x + 10, explorer.camera.y + 20, 30, 30);

    draw_text("Editor", color_black(), explorer.camera.x, explorer.camera.y + 10);

    refresh_screen();
}

// Handle input for editing the map
void handle_editor_input(explorer_data &explorer)
{
    if (key_typed(NUM_1_KEY))
    {
        explorer.editor_tile_kind = NORMAL_TILE;
    }
    if (key_typed(NUM_2_KEY))
    {
        explorer.editor_tile_kind = BORDER_TILE;
    }

    if (mouse_down(LEFT_BUTTON))
    {
        point_2d mouse_pos = mouse_position();
        int c = (mouse_pos.x + explorer.camera.x) / TILE_WIDTH;
        int r = (mouse_pos.y + explorer.camera.y) / TILE_HEIGHT;

        if (c >= 0 && c < MAX_MAP_COLS && r >= 0 && r < MAX_MAP_ROWS)
        {
            if (explorer.editor_tile_kind == NORMAL_TILE)
            {
                if (explorer.map.tiles[c][r].kind == FIX_TILE || explorer.map.tiles[c][r].kind == BORDER_TILE)
                {
                    explorer.map.tiles[c][r].kind = explorer.editor_tile_kind;
                }
            }

            if (explorer.editor_tile_kind == BORDER_TILE)
            {
                if (explorer.map.tiles[c][r].kind == NORMAL_TILE)
                {
                    explorer.map.tiles[c][r].kind = explorer.editor_tile_kind;
                }
            }
        }
    }
}

// Handle general input for the explorer
void handle_input(explorer_data &explorer)
{
    handle_editor_input(explorer);

    if (key_down(LEFT_KEY))
    {
        explorer.camera.x -= 1;
    }
    if (key_down(RIGHT_KEY))
    {
        explorer.camera.x += 1;
    }
    if (key_down(UP_KEY))
    {
        explorer.camera.y -= 1;
    }
    if (key_down(DOWN_KEY))
    {
        explorer.camera.y += 1;
    }
}

// Main function to run the game
int main()
{
    explorer_data explorer;
    init_explorer(explorer);

    // Initialize mold at random position
    int start_c = rnd(0, MAX_MAP_COLS - 1);
    int start_r = rnd(0, MAX_MAP_ROWS - 1);
    mold_data mold = init_mold(start_c, start_r);

    open_window("Map Explorer", 800, 600);

    create_timer(GAME_TIMER);
    start_timer(GAME_TIMER);

    while (!quit_requested())
    {
        handle_input(explorer);

        handle_mold_lifecycle(explorer.map, mold);

        draw_explorer(explorer);

        process_events();
    }

    return 0;
}