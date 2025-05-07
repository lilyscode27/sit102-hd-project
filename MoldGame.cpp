#include "splashkit.h"
#include <queue>
using namespace std;

using std::to_string;

const int MAX_MAP_ROWS = 10;
const int MAX_MAP_COLS = 10;
const int TILE_WIDTH = 30;
const int TILE_HEIGHT = 30;
const string GAME_TIMER = "Game Timer";

// Directions for 8-connected neighbors (up, down, left, right, and diagonals)
const int DX[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
const int DY[8] = {0, 0, -1, 1, -1, 1, -1, 1};

enum tile_kind
{
    NORMAL_TILE,
    MOLDY_TILE,
    FIX_TILE,
    BROKEN_TILE,
    BORDER_TILE
};

struct tile_data
{
    tile_kind kind;
};

struct map_data
{
    tile_data tiles[MAX_MAP_ROWS][MAX_MAP_COLS];
};

struct explorer_data
{
    map_data map;
    tile_kind editor_tile_kind;
    point_2d camera;
};

struct spread_data
{
    int r;
    int c;
};

struct mold_data
{
    int start_c;
    int start_r;
    spread_data spread[MAX_MAP_ROWS * MAX_MAP_COLS];
    bool appeared;
    long appear_at_time;

    // Member function to check if it's time for the mold to appear
    bool time_to_appear(long current_time) const
    {
        return !appeared && current_time > appear_at_time;
    }
};

mold_data init_mold(int start_r, int start_c)
{
    mold_data mold;
    mold.start_r = start_r;
    mold.start_c = start_c;
    for (int i = 0; i < MAX_MAP_COLS * MAX_MAP_ROWS; i++)
    {
        mold.spread[i].r = -1;
        mold.spread[i].c = -1;
    }
    mold.appeared = false;
    mold.appear_at_time = current_ticks() + 1000 + rnd(0, 2000); // Random time between 1 and 3 seconds
    return mold;
}

spread_data init_spread(int r, int c)
{
    spread_data spread;
    spread.r = r;
    spread.c = c;
    return spread;
}

void mold_spread_bfs(map_data &map, mold_data &mold)
{
    if (mold.start_c < 0 || mold.start_c >= MAX_MAP_ROWS || mold.start_r < 0 || mold.start_r >= MAX_MAP_COLS)
    {
        write_line("Invalid starting position for mold spread: " + to_string(mold.start_c) + ", " + to_string(mold.start_r));
        return; // Invalid starting position
    }

    queue<pair<int, int>> q;
    q.push({mold.start_c, mold.start_r});

    int i = 0; // Index for the spread array
    mold.spread[i++] = init_spread(mold.start_c, mold.start_r);
    map.tiles[mold.start_c][mold.start_r].kind = MOLDY_TILE;
    write_line("Mold spread started at: " + to_string(mold.start_c) + ", " + to_string(mold.start_r));
    while (!q.empty())
    {
        int r = q.front().first;
        int c = q.front().second;
        q.pop();

        for (int i = 0; i < 8; i++)
        {
            int nr = r + DX[i];
            int nc = c + DY[i];
            if (nr >= 0 && nr < MAX_MAP_ROWS && nc >= 0 && nc < MAX_MAP_COLS)
            {
                if (map.tiles[nr][nc].kind == NORMAL_TILE)
                {
                    mold.spread[i++] = init_spread(nr, nc);
                    map.tiles[nr][nc].kind = MOLDY_TILE;
                    write_line("Mold spread to: " + to_string(nr) + ", " + to_string(nc));
                    q.push({nr, nc});
                }
            }
        }
    }
}

void update_mold(map_data &map, mold_data &mold, long current_time)
{
    if (mold.time_to_appear(current_time)) // Call the member function
    {
        mold.appeared = true;
        mold_spread_bfs(map, mold);
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
    default:
        return color_white();
    }
}

void draw_tile(tile_data tile, int x, int y)
{
    fill_rectangle(color_for_tile_kind(tile.kind), x, y, TILE_WIDTH, TILE_HEIGHT);
}

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

void draw_explorer(explorer_data &explorer)
{
    set_camera_position(explorer.camera);

    clear_screen(color_white());

    draw_map(explorer.map, explorer.camera);

    fill_rectangle(color_white(), 0, 0, TILE_WIDTH + 10, TILE_HEIGHT + 25);
    fill_rectangle(color_for_tile_kind(explorer.editor_tile_kind), 5, 20, TILE_WIDTH, TILE_HEIGHT);

    draw_text("Editor", color_black(), 0, 10);
    draw_text("Kind: " + to_string(((int)explorer.editor_tile_kind) + 1), color_black(), 0, 30);

    // Draw text for the camera position
    draw_text("Camera: " + to_string(explorer.camera.x) + ", " + to_string(explorer.camera.y), color_black(), 0, 60);

    refresh_screen();
}

void handle_editor_input(explorer_data &explorer)
{

    if (mouse_down(LEFT_BUTTON))
    {
        point_2d mouse_pos = mouse_position();
        int c = (mouse_pos.x + explorer.camera.x) / TILE_WIDTH;
        int r = (mouse_pos.y + explorer.camera.y) / TILE_HEIGHT;

        if (c >= 0 && c < MAX_MAP_COLS && r >= 0 && r < MAX_MAP_ROWS)
        {
            explorer.map.tiles[c][r].kind = explorer.editor_tile_kind;
        }
    }

    if (key_typed(NUM_1_KEY))
    {
        explorer.editor_tile_kind = BORDER_TILE;
    }
}

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

int main()
{
    explorer_data explorer;
    init_explorer(explorer);

    // Initialize mold at random position
    int start_r = rnd(0, MAX_MAP_ROWS - 1);
    int start_c = rnd(0, MAX_MAP_COLS - 1);
    mold_data mold = init_mold(start_r, start_c);
    
    open_window("Map Explorer", 800, 600);

    create_timer(GAME_TIMER);
    start_timer(GAME_TIMER);
    while (!quit_requested())
    {
        handle_input(explorer);
        update_mold(explorer.map, mold, timer_ticks(GAME_TIMER));
        draw_explorer(explorer);

        process_events();
    }

    return 0;
}