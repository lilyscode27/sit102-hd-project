#include "splashkit.h"

using std::to_string;

const int MAX_MAP_ROWS = 30;
const int MAX_MAP_COLS = 30;
const int TILE_WIDTH = 30;
const int TILE_HEIGHT = 30;

enum tile_kind
{
    WATER_TILE,
    GRASS_TILE,
    DIRT_TILE,
    SAND_TILE
};

struct tile_data
{
    tile_kind kind;
};

struct map_data
{
    tile_data tiles[MAX_MAP_COLS][MAX_MAP_ROWS];
};

struct explorer_data
{
    map_data map;
    tile_kind editor_tile_kind;
    point_2d camera;
};

void init_map(map_data &map)
{
    for (int i = 0; i < MAX_MAP_COLS; i++)
    {
        for (int j = 0; j < MAX_MAP_ROWS; j++)
        {
            map.tiles[i][j].kind = GRASS_TILE;
        }
    }
}

void init_explorer(explorer_data &explorer)
{
    init_map(explorer.map);
    explorer.editor_tile_kind = GRASS_TILE;
    explorer.camera = point_at(0, 0);
}

void random_map(map_data &map)
{
    for (int i = 0; i < MAX_MAP_COLS; i++)
    {
        for (int j = 0; j < MAX_MAP_ROWS; j++)
        {
            map.tiles[i][j].kind = (tile_kind)rnd(4);
        }
    }
}

color color_for_tile_kind(tile_kind kind)
{
    switch (kind)
    {
    case WATER_TILE:
        return color_blue();
    case GRASS_TILE:
        return color_lawn_green();
    case SAND_TILE:
        return color_blanched_almond();
    case DIRT_TILE:
        return color_rosy_brown();
    default:
        return color_white();
    }
}

void draw_tile(tile_data tile, int x, int y)
{
    fill_rectangle(color_for_tile_kind(tile.kind), x, y, TILE_WIDTH, TILE_HEIGHT);
}

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

void draw_explorer(const explorer_data &explorer)
{
    set_camera_position(explorer.camera);

    clear_screen(color_white());

    draw_map(explorer.map, explorer.camera);

    fill_rectangle(color_white(), 0, 0, TILE_WIDTH + 10, TILE_HEIGHT + 25);
    fill_rectangle(color_for_tile_kind(explorer.editor_tile_kind), 5, 20, TILE_WIDTH, TILE_HEIGHT);

    draw_text("Editor", color_black(), 0, 10);
    draw_text("Kind: " + to_string(((int)explorer.editor_tile_kind) + 1), color_black(), 0, 30);

    refresh_screen();
}

void handle_editor_input(explorer_data &explorer)
{

    if (key_down(R_KEY))
    {
        random_map(explorer.map);
    }

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
        explorer.editor_tile_kind = WATER_TILE;
    }
    if (key_typed(NUM_2_KEY))
    {
        explorer.editor_tile_kind = GRASS_TILE;
    }
    if (key_typed(NUM_3_KEY))
    {
        explorer.editor_tile_kind = DIRT_TILE;
    }
    if (key_typed(NUM_4_KEY))
    {
        explorer.editor_tile_kind = SAND_TILE;
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
    open_window("Map Explorer", 800, 600);

    while (!quit_requested())
    {
        handle_input(explorer);
        draw_explorer(explorer);

        process_events();
    }

    return 0;
}