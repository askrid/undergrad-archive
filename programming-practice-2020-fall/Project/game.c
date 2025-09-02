#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>

#define SIZE_GRID 4
#define FREQ_TWO 4

void clearScreen();
void putRandom(int grid[][SIZE_GRID]);
void drawGrid(int grid[][SIZE_GRID], unsigned long score);
void swapInt(int *i, int *j);
void moveLeft(int grid[][SIZE_GRID], int *move_success, unsigned long *score);
void moveRight(int grid[][SIZE_GRID], int *move_success, unsigned long *score);
void moveUp(int grid[][SIZE_GRID], int *move_success, unsigned long *score);
void moveDown(int grid[][SIZE_GRID], int *move_success, unsigned long *score);
int isLost(int grid[][SIZE_GRID]);
int check2048(int grid[][SIZE_GRID]);
void setBufferedInput(int enable);

int main(void){
    int grid[SIZE_GRID][SIZE_GRID] = {0,};
    int move_success, have_won = 0;
    unsigned long score = 0;
    char key;

    srand(time(NULL));

    putRandom(grid);
    putRandom(grid);

    setBufferedInput(0);

    while(!isLost(grid)){
        clearScreen();
        drawGrid(grid, score);

        if(!have_won && check2048(grid)){
            have_won = 1;

            printf("You win!\tContinue?(y/n) : ");

            if(getchar() == 'n'){
                rewind(stdin);
                break;
            }
            else{
                rewind(stdin);
                continue;
            }
        }

        if(key == 'q'){
            printf("Quit game?(y/n) : ");

            if(getchar() == 'y'){
                rewind(stdin);
                break;
            }
            else{
                rewind(stdin);
                key = ' ';
                continue;
            }
        }

        key = getchar();
        rewind(stdin);
        
        switch(key){
            case 'a':
            case 'D':
                moveLeft(grid, &move_success, &score);  break;
            case 'd':
            case 'C':
                moveRight(grid, &move_success, &score); break;
            case 'w':
            case 'A':
                moveUp(grid, &move_success, &score);    break;
            case 's':
            case 'B':
                moveDown(grid, &move_success, &score);  break;
            default:
                continue;
            }

        if(move_success)    putRandom(grid);
    }

    clearScreen();
    drawGrid(grid, score);

    setBufferedInput(1);

    printf("Game over.\n\n");
    
    return 0;
}

void clearScreen(){
    printf("\033[2J");
    printf("\033[H");
}

void putRandom(int grid[][SIZE_GRID]){
    int value = (rand() % FREQ_TWO) ? 2 : 4;  // The probability of the value being 4 is (1 / FREQ_TWO).
    int idx = rand(), row, col;

    do{
        row = (idx/SIZE_GRID) % SIZE_GRID;
        col = idx % SIZE_GRID;
        idx++;
    }while(grid[row][col] != 0);

    grid[row][col] = value;

    return;
}

void drawGrid(int grid[][SIZE_GRID], unsigned long score){
    int row, col;

    printf("Score : %lu\n\n", score);
    for(row=0; row<SIZE_GRID; row++){
        for(col=0; col<SIZE_GRID; col++){
            if(!grid[row][col])             printf("[     ]");
            else if(grid[row][col] < 10)    printf("[\033[37m%3d\033[0m  ]", grid[row][col]);
            else if(grid[row][col] < 100)   printf("[\033[33m%4d\033[0m ]", grid[row][col]);
            else if(grid[row][col] < 1000)  printf("[\033[35m%4d\033[0m ]", grid[row][col]);
            else if(grid[row][col] < 10000) printf("[\033[36m%5d\033[0m]", grid[row][col]);
            else                            printf("[\033[34m%d\033[0m]", grid[row][col]);
        }
        printf("\n\n");
    }

    return;
}

void swapInt(int *i, int *j){
    int tmp;

    tmp = *i;
    *i = *j;
    *j = tmp;

    return;
}

void moveLeft(int grid[][SIZE_GRID], int *move_success, unsigned long *score){
    int row, col, i;

    *move_success = 0;

    for(row=0; row<SIZE_GRID; row++){
        for(col=1, i=0; col<SIZE_GRID; col++){
            if(!grid[row][col]){
                continue;
            }
            else if(!grid[row][i]){
                swapInt(&grid[row][col], &grid[row][i]);
                *move_success = 1;
            }
            else if(grid[row][col] != grid[row][i]){
                swapInt(&grid[row][col], &grid[row][++i]);
                if(col != i) *move_success = 1;
            }
            else{ // if(grid[row][col] == grid[row][i])
                *score += 2*grid[row][col];
                grid[row][i++] += grid[row][col];
                grid[row][col] = 0;
                *move_success = 1;
            }
        }
    }

    return;
}

void moveRight(int grid[][SIZE_GRID], int *move_success, unsigned long *score){
    int row, col, i;

    *move_success = 0;

    for(row=0; row<SIZE_GRID; row++){
        for(col=SIZE_GRID-2, i=SIZE_GRID-1; col>-1; col--){
            if(!grid[row][col]){
                continue;
            }
            else if(!grid[row][i]){
                swapInt(&grid[row][col], &grid[row][i]);
                *move_success = 1;
            }
            else if(grid[row][col] != grid[row][i]){
                swapInt(&grid[row][col], &grid[row][--i]);
                if(col != i) *move_success = 1;
            }
            else{ // if(grid[row][col] == grid[row][i])
                *score += 2*grid[row][col];
                grid[row][i--] += grid[row][col];
                grid[row][col] = 0;
                *move_success = 1;
            }
        }
    }

    return;
}

void moveUp(int grid[][SIZE_GRID], int *move_success, unsigned long *score){
    int row, col, i;

    *move_success = 0;

    for(col=0; col<SIZE_GRID; col++){
        for(row=1, i=0; row<SIZE_GRID; row++){
            if(!grid[row][col]){
                continue;
            }
            else if(!grid[i][col]){
                swapInt(&grid[row][col], &grid[i][col]);
                *move_success = 1;
            }
            else if(grid[row][col] != grid[i][col]){
                swapInt(&grid[row][col], &grid[++i][col]);
                if(row != i) *move_success = 1;
            }
            else{ // if(grid[row][col] == grid[i][col])
                *score += 2*grid[row][col];
                grid[i++][col] += grid[row][col];
                grid[row][col] = 0;
                *move_success = 1;
            }
        }
    }

    return;
}

void moveDown(int grid[][SIZE_GRID], int *move_success, unsigned long *score){
    int row, col, i;

    *move_success = 0;

    for(col=0; col<SIZE_GRID; col++){
        for(row=SIZE_GRID-2, i=SIZE_GRID-1; row>-1; row--){
            if(!grid[row][col]){
                continue;
            }
            else if(!grid[i][col]){
                swapInt(&grid[row][col], &grid[i][col]);
                *move_success = 1;
            }
            else if(grid[row][col] != grid[i][col]){
                swapInt(&grid[row][col], &grid[--i][col]);
                if(row != i) *move_success = 1;
            }
            else{ // if(grid[row][col] == grid[i][col])
                *score += 2*grid[row][col];
                grid[i--][col] += grid[row][col];
                grid[row][col] = 0;
                *move_success = 1;
            }
        }
    }

    return;
}

int isLost(int grid[][SIZE_GRID]){
    int row, col;

    for(row=0; row<SIZE_GRID; row++){
        for(col=0; col<SIZE_GRID; col++){
            if(!grid[row][col]) return 0;
        }
    }

    for(row=0; row<SIZE_GRID; row++){
        for(col=0; col<SIZE_GRID-1; col++){
            if(grid[row][col] == grid[row][col+1]) return 0;
        }
    }

    for(col=0; col<SIZE_GRID; col++){
        for(row=0; row<SIZE_GRID-1; row++){
            if(grid[row][col] == grid[row+1][col]) return 0;
        }
    }

    return 1;
}

int check2048(int grid[][SIZE_GRID]){
    int row, col;

    for(row=0; row<SIZE_GRID; row++){
        for(col=0; col<SIZE_GRID; col++){
            if(grid[row][col] == 2048) return 1;
        }
    }

    return 0;
}

void setBufferedInput(int enable){
	static int enabled = 1;
	static struct termios old;
	struct termios new;

	if (enable && !enabled) {
		// restore the former settings
		tcsetattr(0,TCSANOW,&old);
		// set the new state
		enabled = 1;
	} else if (!enable && enabled) {
		// get the terminal settings for standard input
		tcgetattr(0,&new);
		// we want to keep the old setting to restore them at the end
		old = new;
		// disable canonical mode (buffered i/o) and local echo
		new.c_lflag &=(~ICANON & ~ECHO);
		// set the new settings immediately
		tcsetattr(0,TCSANOW,&new);
		// set the new state
		enabled = 0;
	}
}