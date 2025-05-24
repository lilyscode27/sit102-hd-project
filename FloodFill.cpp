#include <iostream>
#include <queue>
using namespace std;

// Directions for 8-connected neighbors (up, down, left, right, and diagonals)
const int DX[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
const int DY[8] = {0, 0, -1, 1, -1, 1, -1, 1};

// Dimensions of the grid
const int ROWS = 5;
const int COLUMNS = 5;

// Function to print the current state of the grid
void print_grid(int grid[ROWS][COLUMNS])
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            cout << grid[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
}

// Recursive implementation of Flood Fill using Depth-First Search (DFS)
void flood_fill_dfs(int grid[ROWS][COLUMNS], int x, int y, int target, int replacement)
{
    // Base case: Check if the current cell is out of bounds or not the target value
    if (x < 0 || x >= ROWS || y < 0 || y >= COLUMNS || grid[x][y] != target)
        return;

    // Replace the current cell with the replacement value
    grid[x][y] = replacement;
    print_grid(grid);

    // Recursively call flood fill for all 8-connected neighbors
    for (int i = 0; i < 8; i++)
    {
        int nx = x + DX[i]; // Calculate the new x-coordinate
        int ny = y + DY[i]; // Calculate the new y-coordinate

        flood_fill_dfs(grid, nx, ny, target, replacement);
    }
}

// Iterative implementation of Flood Fill using Breadth-First Search (BFS)
void flood_fill_bfs(int grid[ROWS][COLUMNS], int start_x, int start_y, int target, int replacement)
{
    // Base case: Check if the starting cell is out of bounds or not the target value
    if (start_x < 0 || start_x >= ROWS || start_y < 0 || start_y >= COLUMNS || grid[start_x][start_y] != target)
        return;

    // Queue to store the cells to be processed
    queue<pair<int, int>> q;

    // Push the starting cell into the queue
    q.push({start_x, start_y});

    // Replace the starting cell with the replacement value
    grid[start_x][start_y] = replacement;
    print_grid(grid);

    // Process the queue until it is empty
    while (!q.empty())
    {
        // Get the front cell from the queue
        int x = q.front().first;
        int y = q.front().second;
        q.pop();

        // Check all 8-connected neighbors
        for (int i = 0; i < 8; i++)
        {
            int nx = x + DX[i]; // Calculate the new x-coordinate
            int ny = y + DY[i]; // Calculate the new y-coordinate

            // Check if the neighbor is within bounds and is the target value
            if (nx >= 0 && nx < ROWS && ny >= 0 && ny < COLUMNS && grid[nx][ny] == target)
            {
                // Replace the neighbor with the replacement value
                grid[nx][ny] = replacement;
                print_grid(grid);

                // Add the neighbor to the queue
                q.push({nx, ny});
            }
        }
    }
}

int main()
{
    // The grid that represents the game area
    int grid1[ROWS][COLUMNS] = {
        {1, 1, 1, 0, 0},
        {1, 1, 0, 0, 0},
        {1, 1, 0, 1, 1},
        {0, 0, 0, 1, 1},
        {0, 0, 0, 0, 0}};
    int grid2[ROWS][COLUMNS] = {
        {1, 1, 1, 0, 0},
        {1, 1, 0, 0, 0},
        {1, 1, 0, 1, 1},
        {0, 0, 0, 1, 1},
        {0, 0, 0, 0, 0}};

    // Initial grid for BFS with all cells as 1
    int grid3[ROWS][COLUMNS] = {
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1}};

    // Perform Flood Fill using DFS
    cout << "Flood Fill using DFS: " << endl;
    flood_fill_dfs(grid1, 1, 1, 1, 2);

    // Perform Flood Fill using BFS
    cout << "Flood Fill using BFS: " << endl;
    flood_fill_bfs(grid3, 1, 1, 1, 2);

    return 0;
}