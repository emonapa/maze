// second IZP project
// xlogin: xmatyav00

/*
   můj kód          Smrčka 😭
                     ___
   ／7、          __/-  `.  .-"""-. 
  （ﾟ､ ｡７        \_,` | \-'  /   )`-')
   l、ﾞ~ヽ         "") `"`    \  ((`"`
   じし(_,)ノ    ___Y  ,    .'7 /|
               (_,___/...-` (_/_/ 
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>

typedef struct {
    int rows;
    int cols;
    unsigned char *cells;
} Map;

void DeleteMap(Map *map) {
    if (map != NULL) {
        free(map->cells);
        map->cells = NULL;
        map->rows = 0;
        map->cols = 0;
    }
}

void InitializeMap(Map *map) {
    map->rows = 0;
    map->cols = 0;
    map->cells = NULL;
}

// Creates array map->cells based on number of rows and cols in the text file.
char CreateMap(const char *filename, Map *map) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file '%s' when creating a map.\n", filename);
        return 1; // Error opening file.
    }

    if (fscanf(file, "%d %d", &map->rows, &map->cols) != 2) {
        fclose(file);
        fprintf(stderr, "Error reading the number of rows and columns.\n");
        return 2; // Error reading the number of rows and columns.
    }

    // Memory allocation for 2D unsigned char array.
    map->cells = malloc(sizeof(unsigned char) * (map->rows * map->cols));
    if (map->cells == NULL) {
        fclose(file);
        fprintf(stderr, "Memory allocation error when creating a map.\n");
        return 3; // Memory allocation error.
    }

    // Validate dimensions.
    int actualRows = 0, actualCols = 0;
    char value;
    while (fscanf(file, "%c ", &value) == 1) {
        actualCols++;
        if (actualCols == map->cols) {
            actualCols = 0;
            actualRows++;
        }
    }
    if (!((actualRows == map->rows) && (actualCols == 1))) {
        fclose(file);
        fprintf(stderr, "Dimensions in the file do not match specified rows and columns.\n");
        return 4; // Dimensions do not match.
    }

    fclose(file);
    return 0;
}

// Loading values into map->cells from the text file.
char LoadMap(const char *filename, Map *map) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error when opening a file while loading a map.\n");
        return 1; // Error opening file.
    }

    if (fseek(file, 4, SEEK_SET) != 0) {
        fclose(file);
        fprintf(stderr, "Error setting the position in the file\n");
        return 2;
    }

    // Reading values from a text file into map.
    for (int r = 0; r < map->rows; r++) {
        for (int c = 0; c < map->cols; c++) {
            if (fscanf(file, "%hhu", &map->cells[r * map->cols + c]) != 1) {
                fclose(file);
                fprintf(stderr, "Error when reading values from file.\n");
                return 3; // Error when reading values from file.
            }            
        }
    }

    fclose(file);
    return 0; // Successful load
}

// Searching if there is a wall at the border position.
bool isborder(Map *map, const int r, const int c, const char border) {
    return ((map->cells[map->cols * r + c] >> border) & 1);
}

// Checks if all walls are matching from both sides.
char CheckValidity(Map *map) {
    for (int row = 0; row < map->rows; row++) {
        for (int col = 0; col < map->cols; col++) {
            if (map->cells[map->cols * row + col] > 7) {
                fprintf(stderr, "Invalid number at [%d,%d] in your text file.\n", row, col);
                return 1;
            }
            
            if (col != map->cols-1 && isborder(map, row, col, 1) != isborder(map, row, col+1, 0)) {
                fprintf(stderr, "Invalid right or left wall at [%d,%d].\n", row, col);
                return 2;
            }
            
            if ((row + col) & 1 && row != map->rows-1 && isborder(map, row, col, 2) != isborder(map, row+1, col, 2)) {
                fprintf(stderr, "Invalid lower or upper wall at [%d,%d].\n", row, col);
                return 3;
            }       
        }
    }
    return 0;
}

// Negating entrace wall to make it easier to solve.
void NegateEntraceBit(Map *map, const char r, const unsigned char c) {
    char whichWall;
    if (c == 0) { // From left.
        whichWall = 0;
    }
    else if (c == map->cols - 1) { // From right.
        whichWall = 1;
    }
    else if (r == 0) { // From up.
        whichWall = 2;
    }
    else if (r == map->rows - 1){ // From down.
        whichWall = 2;
    }

    //Inverting entrace bit.
    map->cells[map->cols * r + c] ^= (1 << whichWall); 
}

// Returns the first wall we encouter at entry in to the maze. 
char start_border(Map *map, const int r, const int c, const char leftright) {
    unsigned char direction;

    if (c == 0) { // Left wall.
        direction = 0;
    }
    else if (c == map->cols - 1) { // Right wall.
        direction = 1;
    } 
    else if (r == 0) { // Upper wall.
        direction = 2;
    } 
    else if (r == map->rows - 1) { // Bottom wall
        direction = 3;
    } else {
        fprintf(stderr, "Starting position is not on the edge of the maze.\n");
        return -1;
    }

    if (isborder(map, r, c, direction - (direction == 3 ? 1 : 0))) {
        fprintf(stderr, "Starting position entrance is missing.\n");
        return -1;
    }

    char borderMap[2][2][4] = { 
        /*
            borderMap
            [hand rule]
            [direction of the triangle]
            [direction of the entry]
        */
        { // Left-hand rule
            {0, 2, 1, 2}, // Triangle pointing down
            {1, 2, 1, 0}  // Triangle pointing up
        },
        { // Right-hand rule
            {1, 0, 2, 1}, // Triangle pointing down
            {2, 0, 0, 1}  // Triangle pointing up
        }
    };

    return borderMap[!(leftright + 1)][(r + c) & 1][direction];
}

void SolveMaze(Map map, int r, int c, int crossed, char leftrigth) {
/* 
         (r+c) % 2 == 1:        (r+c) % 2 == 0:
			    ^			    _______________
		      ╱   ╲			    ╲      0      ╱
		  0 ╱       ╲ 1			2 ╲         ╱ 1
		  ╱           ╲			    ╲	  ╱
		╱_______2_______╲		       v
*/

    int whereToMove[2][3][3] = {   // moves[orientation][edge] == {dr, dc, crossed}
        {{-1,0,2}, {0,1,0}, {0,-1,1}},
        {{0,-1,1}, {0,1,2}, {1,0,0}} 
        };

    int edge, *move;
    unsigned char sumParity;
    char mazeSolvability = 0;
    bool modifyTriangle = true;
    NegateEntraceBit(&map, r, c);

    while (1) {
        sumParity = (r+c) & 1;
        if (r < 0 || r >= map.rows || c < 0 || c >= map.cols) {        
            return;
        }

        printf("%d,%d\n", r+1, c+1);
        edge = crossed;      

        while (1) {          
            edge = (edge + leftrigth + 3) % 3; // Turn and look for an open edge.
            mazeSolvability++;
            // abs(edge - 2*(!sumParity)) Unites 2 different edge counting.
            if (!(isborder(&map, r,c, abs(edge - 2*(!sumParity))))) {
                mazeSolvability = 0;
                break;
            }
            if (mazeSolvability == 3) {
                //After three changes the process repeats, thus its tucked on triangle.
                return;
            }
        } 

        // After modifing the triangle, we need to put it in to original state.
        if (modifyTriangle) {
            NegateEntraceBit(&map, r, c);
            modifyTriangle = false;
        }
        
        move = whereToMove[sumParity][edge]; // After looking up the move, update the
        r += move[0];                        // current row number,
        c += move[1];                        // current column number,
        crossed = move[2];                   // current edge number.
    }
}

// Returns direction that would go backwards. 
char InvertDirection(char direction) {
    if (direction == 0) {
        return 1;
    }
    else if (direction == 1) {
        return 0;
    } else {
        return 2;
    }
}

typedef struct {
    int row;
    int col;
} Point;

// Direction is int bcs compiler is doing booo.
bool ShortestPath(Map map, bool *visited, int r, int c, int direction, Point *path, int *pathLength) {
/* 
         (r+c) % 2 == 1:        (r+c) % 2 == 0:
			    ^			    _______________
		      ╱   ╲			    ╲      2      ╱
		  0 ╱       ╲ 1			0 ╲         ╱ 1
		  ╱           ╲			    ╲	  ╱
		╱_______2_______╲		       v
*/
    static char moves[2][3][2] = {
        {{0,-1}, {0,1}, {-1,0}},
        {{0,-1}, {0,1}, {1,0}}
    };

    r += moves[(r+c) & 1][direction][0];
    c += moves[(r+c) & 1][direction][1];

    if (visited[map.cols * r + c]) {
        return false;
    }
    
    visited[map.cols * r + c] = true;

    // This validating is performed when a new instance is called.
    if (r < 0 || r >= map.rows || c < 0 || c >= map.cols) { // Outside position
        return true;
    }

    // This validating is performed when the only other way is in inverted direction.
    if ((map.cells[map.cols * r + c] ^ (1 << InvertDirection(direction))) == 0b111) {
        return false;
    }

    // For any other direction
    for (char direc = 0; direc < 3; direc++) {
        // that is (available && not in inverted direction),
        if ( !(isborder(&map, r, c, direc)) && (direc != InvertDirection(direction)) ) {
            // we call new instance with wanted direction.
            if (ShortestPath(map, visited, r, c, direc, path, pathLength)) {
                path[*pathLength].row = r + 1;
                path[*pathLength].col = c + 1;
                (*pathLength)++;
                return true;
            } 
        }         
    }
    return false;
}

// Preparing ShortestPath function.
void findShortestPath(Map map, int r, int c) {
    bool *visited = (bool *)malloc((map.rows * map.cols) * sizeof(bool));
    for (int i = 0; i < map.rows * map.cols; i++) {
        visited[i] = false;
    }
    bool foundExit = false;
    
    // The project specification specifies that we have to write 
    // from beginning to end, so the auxiliary array path is added.
    Point *path = (Point *)malloc((map.rows * map.cols) * sizeof(Point));
    int pathLength = 0;

    NegateEntraceBit(&map, r, c);
    for (char direc = 0; direc < 3; direc++) {
        if ( !(map.cells[map.cols * r + c] & (1 << direc)) ) {
            foundExit |= (ShortestPath(map, visited, r, c, direc, path, &pathLength));
            printf("%d,%d\n", r + 1, c + 1);
        }     
    }

    if (foundExit) {
        for (int i = pathLength-1; i >= 0; i--) {
            printf("%d,%d\n", path[i].row, path[i].col);
        }
    } else {
        printf("There is no exit in maze.\n");
    }

    DeleteMap(&map);
    free(visited);
    free(path);
}

void printHelp() {
    printf("Usage: ./maze [options] [file]\n");
    printf("Options:\n");
    printf("  --help                 Display this help message\n");
    printf("  --test [file]          Run test with the specified file\n");
    printf("  --rpath r c [file]     Find the right-hand path starting at position r, c\n");
    printf("  --lpath r c [file]     Find the left-hand path starting at position r, c\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("You need to specify arguments.\n");
        printf("Use --help for usage information.\n");
        return 1;
    }

    char option;
    int r, c;
    char *filename = NULL;
    char leftright;
    Map map;
    InitializeMap(&map);

    struct option longOptions[] = {
        {"help", no_argument, 0, 'h'},
        {"test", required_argument, 0, 't'},
        {"rpath", required_argument, 0, 'r'},
        {"lpath", required_argument, 0, 'l'},
        {"shortest", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    while ((option = getopt_long(argc, argv, "ht:r:l:s:", longOptions, NULL)) != -1) {
        switch (option) {
            case 'h':
                printHelp();
                return 0;
            case 't':
                if (optarg != NULL) {
                    char resCreateMap = CreateMap(optarg, &map);
                    LoadMap(optarg, &map);
                    if (resCreateMap || CheckValidity(&map)) {
                        printf("Invalid\n");
                    }
                    else {
                        printf("Valid\n");
                    }
                    DeleteMap(&map);
                    return 0;
                } else {
                    printf("Error: Specify a file for testing.\n");
                    printf("Use --help for usage information.\n");
                    return 1;
                }
            case 'r':
                if (argc != 5) {
                    fprintf(stderr, "Error in arguments.\n");
                    printf("Use --help for usage information.\n");
                    return 1;
                }
                r = atoi(optarg);
                c = atoi(argv[optind]);
                filename = argv[optind + 1];
                // Going counterclockwise.
                leftright = -1;
                break;
            case 'l':
                if (argc != 5) {
                    fprintf(stderr, "Error in arguments.\n");
                    printf("Use --help for usage information.\n");
                    return 1;
                }
                r = atoi(optarg);
                c = atoi(argv[optind]);
                filename = argv[optind + 1];
                // Going clockwise.
                leftright = 1;
                break;
            case 's':
                if (argc != 5) {
                    fprintf(stderr, "Error in arguments.\n");
                    printf("Use --help for usage information.\n");
                    return 1;
                }
                r = atoi(optarg);
                c = atoi(argv[optind]);
                filename = argv[optind + 1];
                leftright = 0;
                break;
            default:
                printf("Error: Unknown command. Use --help for usage information.\n");
                return 1;
        }
    }

    if (r == 0 || c == 0) {
        fprintf(stderr, "Error in arguments.\n");
        printf("Use --help for usage information.\n");
        return 1;
    }

    char mapCreator = 0;
    mapCreator = CreateMap(filename, &map) || LoadMap(filename, &map) || CheckValidity(&map);

    if (mapCreator != 0) {
        DeleteMap(&map);
        return 1;
    }

    if (leftright == 0) { // leftright == 0 <=> shortest path
        findShortestPath(map, r - 1, c - 1);
        return 0;
    } 

    char crossed = start_border(&map, r-1, c-1, leftright);
    if (crossed == -1) {
        DeleteMap(&map);
        return 1;
    }

    SolveMaze(map, r-1, c-1, crossed, leftright);

    DeleteMap(&map);
    
    return 0;
}
