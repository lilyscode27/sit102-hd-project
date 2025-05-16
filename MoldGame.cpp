#include "splashkit.h"
#include <vector>
#include <queue>
#include <fstream>
#include <algorithm>

using namespace std;

using std::to_string;

// Constants for window size and map dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int MAX_MAP_ROWS = 40;
const int MAX_MAP_COLS = 40;
const int TILE_WIDTH = 30;
const int TILE_HEIGHT = 30;

// Constants for the game resources
const music GAME_MUSIC = load_music("Game Music", "game-music.mp3");
const sound_effect DRAWING_SOUND = load_sound_effect("Drawing Sound", "drawing-sound.mp3");
const bitmap MOLD_PIC = load_bitmap("Mold Pic", "mold_pic1_mod.png");
const bitmap PAUSE_ICON = load_bitmap("Pause Button", "pause.png");
const bitmap SOUND_ON_ICON = load_bitmap("Sound On", "sound-on.png");
const bitmap SOUND_OFF_ICON = load_bitmap("Sound Off", "sound-off.png");
const bitmap ATTENTION_ICON = load_bitmap("Attention Icon", "attention.png");
const font TEXT_FONT = load_font("Text Font", "DynaPuff-VariableFont_wdth,wght.ttf");
const double MUSIC_VOLUME = 0.3;

// Constants for the game
const string GAME_TIMER = "Game Timer";
const long GAME_UPDATE_INTERVAL = 20000; // Interval for updating game difficulty
const string SCORE_FILE = "score.txt";   // File to store scores

// Constants for the interface
const int BUTTON_WIDTH = 150;
const int BUTTON_HEIGHT = 30;
const int LINE_SPACING = 20;

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

// Enum for game states
enum game_state
{
    PREPARE_GAME,
    PLAYING,
    PAUSING,
    GAME_OVER,
    QUIT
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
    // // Location to start spreading from
    int start_c;
    int start_r;

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

// Structure to represent multiple molds data
struct molds_data
{
    vector<mold_data> v;      // Vector to store current molds
    long time_to_appear_next; // Time for the next mold to appear

    // Check if it's time for the next mold to appear
    bool is_time_to_appear_next(long current_time) const
    {
        return current_time > time_to_appear_next;
    }
};

struct attention_data
{
    int new_r; // The row to draw the attention icon at
    int new_c; // The column to draw the attention icon at
    string visibility;  // Visibility status (left, right, top, bottom)
};

// Structure to represent the current game data
struct game_data
{
    double broken_proportion;   // Proportion of broken tiles
    double border_proportion;   // Proportion of border tiles
    molds_data molds;           // Data for the molds
    long mold_appearance_time;  // Time of mold appearance
    game_state state;           // Current state of the game
    int score;                  // Score of the game
    bool is_player_score_saved; // Flag to indicate if the score is saved

    // Check if the game is over (80% of tiles are broken or blocked by border tiles)
    bool is_game_over()
    {
        return (broken_proportion + border_proportion) >= 80.0;
    }
};

struct game_effect_data
{
    bool is_sound_on; // Flag to indicate if sound is on
};

// Function to save the score to a file
bool save_score_to_file(game_data &game)
{
    std::ofstream score_file(SCORE_FILE, std::ios::app); // Open file in append mode
    if (score_file.is_open())
    {
        score_file << game.score << "\n"; // Write the score to the file
        score_file.close();               // Close the file
        write_line("Score saved to score.txt");
        return true;
    }
    else
    {
        write_line("Failed to open score.txt for writing.");
        return false;
    }
}

// Function to get the top 5 highest scores from the file
std::vector<int> get_top_5_scores()
{
    std::vector<int> scores;
    std::ifstream score_file(SCORE_FILE); // Open the file in read mode

    if (score_file.is_open())
    {
        int score;
        while (score_file >> score) // Read scores from the file
        {
            scores.push_back(score);
        }
        score_file.close(); // Close the file
    }
    else
    {
        write_line("Failed to open score.txt for reading.");
        return {}; // Return an empty vector if the file cannot be opened
    }

    // Sort the scores in descending order
    std::sort(scores.begin(), scores.end(), std::greater<int>());

    // Get the top 5 scores
    if (scores.size() > 5)
    {
        scores.resize(5); // Keep only the top 5 scores
    }

    return scores;
}

// Function to initialize a location
location_data init_loc(int c, int r)
{
    location_data loc;
    loc.c = c;
    loc.r = r;
    return loc;
}

// Function to initialize mold with starting position and default values
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

// Function to initialize molds data
molds_data init_molds()
{
    molds_data molds;
    molds.time_to_appear_next = 0;
    return molds;
}

// Function to initialize the game data
game_data init_game()
{
    game_data game;
    game.broken_proportion = 0.0;
    game.border_proportion = 0.0;
    game.molds = init_molds(); // Initialize molds data
    game.mold_appearance_time = 0;
    game.state = PREPARE_GAME;
    game.score = 0;
    game.is_player_score_saved = false; // Initialize score saved flag
    return game;
}

game_effect_data init_game_effect()
{
    game_effect_data game_effect;
    game_effect.is_sound_on = true; // Initialize sound state
    return game_effect;
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
                mold.q.push({nc, nr});
                mold.last_spread_time = timer_ticks(GAME_TIMER); // Update last spread time
            }
        }
    }

    // Check if the queue is empty, indicating that mold has finished spreading
    if (mold.q.empty())
    {
        mold.time_to_start_fix = timer_ticks(GAME_TIMER) + 5000; // Set time to start fix
        mold.state = DONE_SPREADING;                             // Specify that spreading is done
    }
}

// Function to handle the lifecycle of mold (appearance, spreading, fixing, breaking)
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

// Function to update the game data
void update_game(game_data &game, const map_data &map)
{
    int broken_tiles = 0;
    int border_tiles = 0;

    for (int i = 0; i < MAX_MAP_COLS; i++)
    {
        for (int j = 0; j < MAX_MAP_ROWS; j++)
        {
            if (map.tiles[i][j].kind == BROKEN_TILE)
            {
                broken_tiles++;
            }
            else if (map.tiles[i][j].kind == BORDER_TILE)
            {
                border_tiles++;
            }
        }
    }

    game.broken_proportion = static_cast<double>(broken_tiles) * 100 / (MAX_MAP_COLS * MAX_MAP_ROWS);
    game.border_proportion = static_cast<double>(border_tiles) * 100 / (MAX_MAP_COLS * MAX_MAP_ROWS);
}

// Function to initialize the explorer data
void init_explorer(explorer_data &explorer)
{
    init_map(explorer.map);
    explorer.editor_tile_kind = NORMAL_TILE;
    explorer.camera = point_at(0, 0);
}

// Function to get the color corresponding to a tile kind
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

// Function to draw a single tile at a specific position
void draw_tile(tile_data tile, int x, int y)
{
    fill_rectangle(color_for_tile_kind(tile.kind), x, y, TILE_WIDTH, TILE_HEIGHT);
}

// Function to determine whether a mold is off the visible map
attention_data is_mold_visible(int mold_start_c, int mold_start_r, const point_2d &camera)
{
    attention_data attention;

    // Calculate the visible range of columns and rows
    int map_start_c = camera.x / TILE_WIDTH;
    int map_end_c = (camera.x + screen_width()) / TILE_WIDTH;
    int map_start_r = camera.y / TILE_HEIGHT;
    int map_end_r = (camera.y + screen_height()) / TILE_HEIGHT;

    // Determine the relative position of the mold
    if (mold_start_c < map_start_c)
    {
        attention.visibility = "Left";
        attention.new_r = mold_start_r - map_start_r;

        // Ensure the new row is within bounds
        if (attention.new_r < 0) 
            attention.new_r = 0;
        else if (attention.new_r >= WINDOW_HEIGHT / TILE_HEIGHT)
            attention.new_r = WINDOW_HEIGHT / TILE_HEIGHT - 1;
    }

    else if (mold_start_c > map_end_c)
    {
        attention.visibility = "Right";
        attention.new_r = mold_start_r - map_start_r;

        // Ensure the new row is within bounds
        if (attention.new_r < 0)
            attention.new_r = 0;
        else if (attention.new_r >= WINDOW_HEIGHT / TILE_HEIGHT)
            attention.new_r = WINDOW_HEIGHT / TILE_HEIGHT - 1;
    }

    else if (mold_start_r < map_start_r)
    {
        attention.visibility = "Top";
        attention.new_c = mold_start_c - map_start_c;

        // Ensure the new column is within bounds
        if (attention.new_c < 0)
            attention.new_c = 0;
        else if (attention.new_c >= WINDOW_WIDTH / TILE_WIDTH)
            attention.new_c = WINDOW_WIDTH / TILE_WIDTH - 1;
    }

    else if (mold_start_r > map_end_r)
    {
        attention.visibility = "Bottom";
        attention.new_c = mold_start_c - map_start_c;

        // Ensure the new column is within bounds
        if (attention.new_c < 0)
            attention.new_c = 0;
        else if (attention.new_c >= WINDOW_WIDTH / TILE_WIDTH)
            attention.new_c = WINDOW_WIDTH / TILE_WIDTH - 1;
    }

    else
    {
        attention.visibility = "Visible";
    }

    return attention;
}

// Function to draw the entire map based on the camera position
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

// Frunction to draw the prepare game interface
void prepare_interface(game_data &game)
{
    clear_screen(color_yellow_green());
    set_camera_position(point_at(0, 0));

    draw_bitmap(MOLD_PIC, (WINDOW_WIDTH - 200) / 2, 0);

    draw_text("Welcome to Moldbound!", color_dark_olive_green(), "Text Font", 50, 120, (WINDOW_HEIGHT - BUTTON_HEIGHT) / 2 - BUTTON_HEIGHT * 3 - 50 + 50);

    if (button("Start Game: Easy", rectangle_from((WINDOW_WIDTH - BUTTON_WIDTH) / 2, (WINDOW_HEIGHT - BUTTON_HEIGHT) / 2 - BUTTON_HEIGHT * 2 + 50, BUTTON_WIDTH, BUTTON_HEIGHT)))
    {
        game.mold_appearance_time = 10000;
        game.state = PLAYING;
        reset_timer(GAME_TIMER);
        start_timer(GAME_TIMER);
    }
    if (button("Start Game: Medium", rectangle_from((WINDOW_WIDTH - BUTTON_WIDTH) / 2, (WINDOW_HEIGHT - BUTTON_HEIGHT) / 2 - BUTTON_HEIGHT + 50, BUTTON_WIDTH, BUTTON_HEIGHT)))
    {
        game.mold_appearance_time = 8000;
        game.state = PLAYING;
        reset_timer(GAME_TIMER);
        start_timer(GAME_TIMER);
    }
    if (button("Start Game: Hard", rectangle_from((WINDOW_WIDTH - BUTTON_WIDTH) / 2, (WINDOW_HEIGHT - BUTTON_HEIGHT) / 2 + 50, BUTTON_WIDTH, BUTTON_HEIGHT)))
    {
        game.mold_appearance_time = 5000;
        game.state = PLAYING;
        reset_timer(GAME_TIMER);
        start_timer(GAME_TIMER);
    }

    if (button("Exit Program", rectangle_from((WINDOW_WIDTH - BUTTON_WIDTH) / 2, (WINDOW_HEIGHT - BUTTON_HEIGHT) / 2 + BUTTON_HEIGHT + 50, BUTTON_WIDTH, BUTTON_HEIGHT)))
    {
        game.state = QUIT;
    }

    vector<int> top_scores = get_top_5_scores();
    for (int i = 0; i < top_scores.size(); i++)
    {
        draw_text(to_string(i + 1) + ". " + to_string(top_scores[i]), color_black(), "Text Font", 20, (WINDOW_WIDTH - BUTTON_WIDTH) / 2 + 50, (WINDOW_HEIGHT - BUTTON_HEIGHT) / 2 + BUTTON_HEIGHT + LINE_SPACING + LINE_SPACING * (i + 1) + 50);
    }
}

// Function to draw the playing interface
void playing_interface(const explorer_data &explorer, game_data &game)
{
    set_camera_position(explorer.camera);

    clear_screen(color_white());

    draw_map(explorer.map, explorer.camera);

    // Draw the attention icon for molds that are off the visible map
    if (!game.molds.v.empty())
    {
        for (int i = 0; i < game.molds.v.size(); i++)
        {
            if (game.molds.v[i].state == SPREADING)
            {
                attention_data attention = is_mold_visible(game.molds.v[i].start_c, game.molds.v[i].start_r, explorer.camera);
                if (attention.visibility == "Left")
                {
                    draw_bitmap(ATTENTION_ICON, explorer.camera.x, explorer.camera.y + attention.new_r * TILE_HEIGHT);
                }
                else if (attention.visibility == "Right")
                {
                    draw_bitmap(ATTENTION_ICON, explorer.camera.x + WINDOW_WIDTH - TILE_WIDTH, explorer.camera.y + attention.new_r * TILE_HEIGHT);
                }
                else if (attention.visibility == "Top")
                {
                    draw_bitmap(ATTENTION_ICON, explorer.camera.x + attention.new_c * TILE_WIDTH, explorer.camera.y);
                }
                else if (attention.visibility == "Bottom")
                {
                    draw_bitmap(ATTENTION_ICON, explorer.camera.x + attention.new_c * TILE_WIDTH, explorer.camera.y + WINDOW_HEIGHT - TILE_HEIGHT);
                }
            }
        }
    }

    // Draw the editor to change tile kind
    draw_text("Editor: Enter 1 for BORDER_TILE, 2 for NORMAL_TILE", color_sea_green(), "Text Font", 15, explorer.camera.x, explorer.camera.y);
    draw_rectangle(color_black(), explorer.camera.x, explorer.camera.y + 10 + LINE_SPACING, 50, 50);
    fill_rectangle(color_for_tile_kind(explorer.editor_tile_kind), explorer.camera.x + 10, explorer.camera.y + 20 + LINE_SPACING, 30, 30);
    draw_rectangle(color_black(), explorer.camera.x + 10, explorer.camera.y + LINE_SPACING * 2, 30, 30);

    draw_text("Percentage of map unavailable: " + to_string(game.broken_proportion + game.border_proportion) + "%", color_sea_green(), "Text Font", 15, explorer.camera.x, explorer.camera.y + 10 + LINE_SPACING * 4);

    if (button("Pause Game", rectangle_from(WINDOW_WIDTH - BUTTON_WIDTH, 0, BUTTON_WIDTH, BUTTON_HEIGHT)))
    {
        game.state = PAUSING;
        pause_timer(GAME_TIMER);
    }
}

// Function to draw the pausing interface
void pausing_interface(game_data &game)
{
    if (button("Resume Game", rectangle_from((WINDOW_WIDTH - BUTTON_WIDTH) / 2, (WINDOW_HEIGHT - BUTTON_HEIGHT) / 2, BUTTON_WIDTH, BUTTON_HEIGHT)))
    {
        game.state = PLAYING;
        resume_timer(GAME_TIMER);
    }
    if (button("Quit Game", rectangle_from((WINDOW_WIDTH - BUTTON_WIDTH) / 2, (WINDOW_HEIGHT - BUTTON_HEIGHT) / 2 + BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT)))
    {
        game.state = PREPARE_GAME;
    }
}

// Function to draw the game over interface
void game_over_interface(explorer_data explorer, game_data &game)
{
    draw_text("Game Over", color_red(), "Text Font", 15, explorer.camera.x + 300 + 70, explorer.camera.y + 280);
    draw_text("You survived for " + to_string(game.score) + " seconds.", color_red(), "Text Font", 15, explorer.camera.x + 300, explorer.camera.y + 280 + LINE_SPACING);

    if (game.is_player_score_saved == false)
    {
        // Save the score to a file
        game.is_player_score_saved = save_score_to_file(game);
    }

    if (button("Back to Home", rectangle_from((WINDOW_WIDTH - BUTTON_WIDTH) / 2, (WINDOW_HEIGHT - BUTTON_HEIGHT) / 2 + 40, BUTTON_WIDTH, BUTTON_HEIGHT)))
    {
        game.state = PREPARE_GAME;
    }
    if (button("Exit Program", rectangle_from((WINDOW_WIDTH - BUTTON_WIDTH) / 2, (WINDOW_HEIGHT - BUTTON_HEIGHT) / 2 + 40 + BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT)))
    {
        game.state = QUIT;
    }
}

// Function to draw the corresponding interface based on the game state
void draw_explorer(const explorer_data &explorer, game_data &game, game_effect_data &game_effect)
{

    switch (game.state)
    {
    case PREPARE_GAME:
        prepare_interface(game);
        break;
    case PLAYING:
        playing_interface(explorer, game);
        break;
    case PAUSING:
        pausing_interface(game);
        break;
    case GAME_OVER:
        game_over_interface(explorer, game);
        break;
    case QUIT:
        break;
    }

    // Draw the sound button
    bitmap sound_state;
    if (music_volume() == 0)
    {
        sound_state = SOUND_OFF_ICON;
    }
    else
    {
        sound_state = SOUND_ON_ICON;
    }

    if (bitmap_button(sound_state, rectangle_from(WINDOW_WIDTH - 52, WINDOW_HEIGHT - 52, 50, 50)))
    {
        if (music_volume() == 0)
        {
            set_music_volume(MUSIC_VOLUME);
            game_effect.is_sound_on = true;
        }
        else
        {
            set_music_volume(0);
            game_effect.is_sound_on = false;
        }
    }

    draw_interface();
    refresh_screen();
}

// Function to handle input for editing the map
void handle_editor_input(explorer_data &explorer, game_effect_data &game_effect)
{
    if (key_typed(NUM_1_KEY))
    {
        explorer.editor_tile_kind = BORDER_TILE;
    }
    if (key_typed(NUM_2_KEY))
    {
        explorer.editor_tile_kind = NORMAL_TILE;
    }

    if (mouse_down(LEFT_BUTTON))
    {

        point_2d mouse_pos = mouse_position();
        int c = (mouse_pos.x + explorer.camera.x) / TILE_WIDTH;
        int r = (mouse_pos.y + explorer.camera.y) / TILE_HEIGHT;

        if (c >= 0 && c < MAX_MAP_COLS && r >= 0 && r < MAX_MAP_ROWS)
        {

            if (!sound_effect_playing("Drawing Sound"))
            {
                if (game_effect.is_sound_on)
                {
                    play_sound_effect("Drawing Sound");
                }
            }

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
    else
    {
        stop_sound_effect("Drawing Sound");
    }
}

// Function to handle general input for the explorer
void handle_input(explorer_data &explorer, game_effect_data &game_effect)
{
    handle_editor_input(explorer, game_effect);

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
    play_music("Game Music");
    set_music_volume(MUSIC_VOLUME);

    explorer_data explorer;
    game_data game = init_game();
    game_effect_data game_effect = init_game_effect();

    set_interface_accent_color(color_dark_olive_green(), 1.0);
    set_interface_font("Text Font");

    open_window("Moldbound", WINDOW_WIDTH, WINDOW_HEIGHT);

    create_timer(GAME_TIMER);

    while (!quit_requested())
    {
        // Play background music
        if (!music_playing() && game_effect.is_sound_on)
        {
            play_music("Game Music");
        }

        process_events();
        draw_explorer(explorer, game, game_effect);

        // Handle the prepare game state
        if (game.state == PREPARE_GAME)
        {
            init_explorer(explorer);
            game = init_game();
        }

        // Handle the playing state
        if (game.state == PLAYING)
        {
            // Handle player input
            handle_input(explorer, game_effect);

            // Check if the game is over
            if (game.is_game_over())
            {
                game.score = timer_ticks(GAME_TIMER) / 1000;
                game.state = GAME_OVER;
            }

            // Check if there is space available for mold to spread
            if (is_space_available(explorer.map))
            {
                if (game.molds.is_time_to_appear_next(timer_ticks(GAME_TIMER)))
                {
                    int start_c, start_r;
                    do
                    {
                        start_c = rnd(0, MAX_MAP_COLS - 1);
                        start_r = rnd(0, MAX_MAP_ROWS - 1);
                    } while (explorer.map.tiles[start_c][start_r].kind != NORMAL_TILE);

                    mold_data new_mold = init_mold(start_c, start_r); // Initialize new mold
                    game.molds.v.push_back(new_mold);                 // Add new mold to the vector

                    // game.molds.time_to_appear_next = timer_ticks(GAME_TIMER) + 5000 * game.mold_appearance_speed + rnd(0, 2000); // Set time for next mold appearance
                    game.molds.time_to_appear_next = timer_ticks(GAME_TIMER) + game.mold_appearance_time + rnd(0, 2000); // Set time for next mold appearance
                }
            }

            // Update current molds
            if (!game.molds.v.empty())
            {
                for (int i = 0; i < game.molds.v.size(); i++)
                {
                    // Handle mold lifecycle
                    handle_mold_lifecycle(explorer.map, game.molds.v[i]);

                    // Check if mold lifecycle is finished
                    if (game.molds.v[i].state == BROKEN)
                    {
                        game.molds.v.erase(game.molds.v.begin() + i); // Remove finished mold
                        i--;                                          // Adjust index after erasing
                    }
                }
            }

            // Update game data
            update_game(game, explorer.map);
        }

        // Handle the quit game state
        if (game.state == QUIT)
        {
            return 0;
        }
    }

    // Free resources
    free_all_sound_effects();
    free_all_music();

    return 0;
}