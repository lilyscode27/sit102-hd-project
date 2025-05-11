#include "splashkit.h"
#include <vector>
#include <queue>
using namespace std;

using std::to_string;

// TODO: Check if the border tile has taken up more than 50% of remaining normal tiles. If so, pause the game and tell the user to remove some border tiles.
// TODO: Display the percentage of broken tiles on the screen.
//

// Constants for map dimensions and tile sizes
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int MAX_MAP_ROWS = 40;
const int MAX_MAP_COLS = 40;
const int TILE_WIDTH = 30;
const int TILE_HEIGHT = 30;
const string GAME_TIMER = "Game Timer";
const long GAME_UPDATE_INTERVAL = 20000; // Interval for game updates in milliseconds

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
    FIXING,
    BROKEN
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

    long appear_at_time;    // Time when the mold appears
    long last_spread_time;  // Last time mold spread
    long time_to_start_fix; // Time to start fixing mold

    location_data spread[MAX_MAP_COLS * MAX_MAP_ROWS]; // Array to track spreads locations
    int spreads_count;                                 // Counter for the number of spreads

    queue<pair<int, int>> q; // Queue for BFS during spreading

    // Check if it's time for the mold to spread
    bool is_time_to_spread(long current_time) const
    {
        return current_time >= last_spread_time + 1000;
    }
};

struct molds_data
{
    vector<mold_data> v; // Vector to store all molds
    long time_to_appear_next;

    bool is_time_to_appear_next(long current_time) const
    {
        return current_time > time_to_appear_next;
    }
};

struct game_data
{
    double broken_proportion;
    double border_proportion;
    long last_update_dif_time;
    double mold_appearance_speed;

    bool is_time_to_update(long current_time) const
    {
        return current_time > last_update_dif_time + GAME_UPDATE_INTERVAL;
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

    mold.appear_at_time = timer_ticks(GAME_TIMER); // Set appearance time
    mold.last_spread_time = 0;                     // Initialize last spread time
    mold.time_to_start_fix = 0;                    // Initialize time to start fix

    // Initialize the spread locations
    for (int i = 0; i < MAX_MAP_COLS * MAX_MAP_ROWS; i++)
    {
        mold.spread[i].c = -1;
        mold.spread[i].r = -1;
    }
    mold.spreads_count = 0;

    return mold;
}

molds_data init_molds()
{
    molds_data molds;
    molds.time_to_appear_next = 0;
    return molds;
}

game_data init_game()
{
    game_data game;
    game.broken_proportion = 0.0;
    game.border_proportion = 0.0;
    game.last_update_dif_time = 0;
    game.mold_appearance_speed = 1;
    return game;
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
                //write_line("Mold spread to: " + to_string(nc) + ", " + to_string(nr));
                mold.q.push({nc, nr});
                mold.last_spread_time = timer_ticks(GAME_TIMER); // Update last spread time
            }
        }
    }

    // Check if the queue is empty, indicating that mold has finished spreading
    if (mold.q.empty())
    {
        //write_line("Mold spread finished.");
        mold.time_to_start_fix = timer_ticks(GAME_TIMER) + 5000; // Set time to start fix
        mold.state = DONE_SPREADING;                             // Specify that spreading is done
    }
}

// Handle the lifecycle of mold (appearance, spreading, fixing, breaking)
void handle_mold_lifecycle(map_data &map, mold_data &mold)
{
    if (mold.state == PREPARE)
    {
        mold.q.push({mold.start_c, mold.start_r});
        mold.spread[mold.spreads_count++] = init_loc(mold.start_c, mold.start_r);
        map.tiles[mold.start_c][mold.start_r].kind = MOLDY_TILE;
        mold.last_spread_time = timer_ticks(GAME_TIMER); // Set the appear_at_time to the current time + 1 second
        mold.state = SPREADING;                          // Set state to spreading
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
    if (mold.state == FIXING && timer_ticks(GAME_TIMER) > mold.time_to_start_fix)
    {
        for (int i = 0; i < mold.spreads_count; i++)
        {
            if (map.tiles[mold.spread[i].c][mold.spread[i].r].kind == FIX_TILE)
            {
                map.tiles[mold.spread[i].c][mold.spread[i].r].kind = BROKEN_TILE;
            }
        }
        mold.state = BROKEN; // Set state to broken
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

// Function to check if there is space available for mold to spread
bool is_space_available(map_data &map)
{
    for (int i = 0; i < MAX_MAP_COLS; i++)
    {
        for (int j = 0; j < MAX_MAP_ROWS; j++)
        {
            if (map.tiles[i][j].kind == NORMAL_TILE)
            {
                return true;
            }
        }
    }
    return false;
}

void update_game(game_data &game, const map_data &map)
{
    int normal_tiles = 0;
    int broken_tiles = 0;
    int border_tiles = 0;

    for (int i = 0; i < MAX_MAP_COLS; i++)
    {
        for (int j = 0; j < MAX_MAP_ROWS; j++)
        {
            if (map.tiles[i][j].kind == NORMAL_TILE)
            {
                normal_tiles++;
            }
            else if (map.tiles[i][j].kind == BROKEN_TILE)
            {
                broken_tiles++;
            }
            else if (map.tiles[i][j].kind == BORDER_TILE)
            {
                border_tiles++;
            }
        }
    }

    game.broken_proportion = static_cast<double>(broken_tiles) * 100 / (normal_tiles + broken_tiles);
    game.border_proportion = static_cast<double>(border_tiles) * 100 / (normal_tiles + border_tiles);
    // write_line("Broken proportion: " + to_string(game.broken_proportion));
    // write_line("Border proportion: " + to_string(game.border_proportion));
}

// Function to check if the game is over (all tiles are broken or blocked by border tiles)
bool is_game_over(map_data &map)
{
    for (int i = 0; i < MAX_MAP_COLS; i++)
    {
        for (int j = 0; j < MAX_MAP_ROWS; j++)
        {
            if (map.tiles[i][j].kind != BROKEN_TILE && map.tiles[i][j].kind != BORDER_TILE)
            {
                return false;
            }
        }
    }
    return true;
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
void draw_map(const map_data &map, const point_2d &camera)
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
void draw_explorer(const explorer_data &explorer, const game_data &game)
{
    set_camera_position(explorer.camera);

    clear_screen(color_white());

    draw_map(explorer.map, explorer.camera);

    draw_rectangle(color_black(), explorer.camera.x, explorer.camera.y + 10, 50, 50);
    fill_rectangle(color_for_tile_kind(explorer.editor_tile_kind), explorer.camera.x + 10, explorer.camera.y + 20, 30, 30);
    draw_rectangle(color_black(), explorer.camera.x + 10, explorer.camera.y + 20, 30, 30);

    draw_text("Editor", color_black(), explorer.camera.x, explorer.camera.y + 10);

    draw_text("Broken tiles proportion: " + to_string(game.broken_proportion) + "%", color_black(), explorer.camera.x, explorer.camera.y + 70);
    draw_text("Border tiles proportion: " + to_string(game.border_proportion) + "%", color_black(), explorer.camera.x, explorer.camera.y + 90);

    if (game.border_proportion > 50)
    {
        draw_text("Warning: Too many border tiles!", color_red(), explorer.camera.x, explorer.camera.y + 110);
    }

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
        explorer.camera.x -= 2;
    }
    if (key_down(RIGHT_KEY))
    {
        explorer.camera.x += 2;
    }
    if (key_down(UP_KEY))
    {
        explorer.camera.y -= 2;
    }
    if (key_down(DOWN_KEY))
    {
        explorer.camera.y += 2;
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

    molds_data molds;

    open_window("Map Explorer", WINDOW_WIDTH, WINDOW_HEIGHT);

    game_data game = init_game();

    create_timer(GAME_TIMER);
    start_timer(GAME_TIMER);

    while (!quit_requested())
    {
        if (is_game_over(explorer.map))
        {
            write_line("Game Over! All tiles are broken.");
            write_line("You survived for " + to_string(timer_ticks(GAME_TIMER) / 1000) + " seconds.");
            break;
        }
        handle_input(explorer);

        int start_c, start_r;

        if (is_space_available(explorer.map))
        {
            if (molds.is_time_to_appear_next(timer_ticks(GAME_TIMER)))
            {
                //write_line("New mold appeared!");

                do
                {
                    start_c = rnd(0, MAX_MAP_COLS - 1);
                    start_r = rnd(0, MAX_MAP_ROWS - 1);
                    //write_line("New mold position: " + to_string(start_c) + ", " + to_string(start_r));
                } while (explorer.map.tiles[start_c][start_r].kind != NORMAL_TILE);

                mold_data new_mold = init_mold(start_c, start_r); // Initialize new mold
                molds.v.push_back(new_mold);                      // Add new mold to the vector

                molds.time_to_appear_next = timer_ticks(GAME_TIMER) + 5000 * game.mold_appearance_speed + rnd(0, 2000); // Set time for next mold appearance
            }
        }

        // Update all molds
        if (!molds.v.empty())
        {
            for (int i = 0; i < molds.v.size(); i++)
            {
                // Handle mold lifecycle
                handle_mold_lifecycle(explorer.map, molds.v[i]);

                // Check if mold lifecycle is finished
                if (molds.v[i].state == BROKEN)
                {
                    molds.v.erase(molds.v.begin() + i); // Remove finished mold
                    // write_line("Mold lifecycle finished.");
                    i--; // Adjust index after erasing
                }
            }
        }

        // Update game state
        update_game(game, explorer.map);

        // Check if it's time to update the game
        if (game.is_time_to_update(timer_ticks(GAME_TIMER)))
        {
            if (game.mold_appearance_speed - 0.1 > 0)
            {
                game.mold_appearance_speed -= 0.1;
                game.last_update_dif_time = timer_ticks(GAME_TIMER);
                write_line("Mold appearance speed increased: " + to_string(game.mold_appearance_speed));
            }
        }

        draw_explorer(explorer, game);

        process_events();
    }

    return 0;
}