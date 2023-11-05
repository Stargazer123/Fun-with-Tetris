/*
 Simple terminal Tetris for Linux ncurses.
 By: Fredrik Roos 2021
 mail: info@crazedout.com
 gcc -Wall -O3 -std=c99 -o tetris tetris.c -lncurses
 Get ncurses:
 sudo apt-get install libncurses5-dev libncursesw5-dev
*/

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>

#define START_X 5
#define START_Y 1
#define COLS 16
#define ROWS 20

WINDOW* gamewin,*gframe;

char title[] = "T E T R I S";
char begin[] = "PRESS 'SPACE'\n   TO BEGIN\n\n USE ARROW KEYS\n\n  By TMB 2021 \n\n   'q' quits\n   ' ' drop\n   'p' pause\n";
char score[24];
int  points = 0;
char brick = '#';

int SPEED = 40000;
int GAME_SPEED = 50000;
int FALL_SPEED = 15000;

int cur_block[16][2];
int temp_block[16][2];
int bl_0[16][2] = {{1,1},{2,1},{1,2},{2,2},{1,1},{2,1},{1,2},{2,2},{1,1},{2,1},{1,2},{2,2},{1,1},{2,1},{1,2},{2,2}};
int bl_1[16][2] = {{1,1},{2,1},{2,2},{2,3},{2,1},{0,2},{1,2},{2,2},{1,0},{1,1},{1,2},{2,2},{1,1},{2,1},{3,1},{1,2}};
int bl_2[16][2] = {{1,1},{2,1},{2,2},{3,2},{2,1},{1,2},{2,2},{1,3},{0,1},{1,1},{1,2},{2,2},{2,1},{1,2},{2,2},{1,3}};
int bl_3[16][2] = {{2,0},{2,1},{2,2},{2,3},{0,2},{1,2},{2,2},{3,2},{2,0},{2,1},{2,2},{2,3},{0,2},{1,2},{2,2},{3,2}};
int bl_4[16][2] = {{2,1},{0,2},{1,2},{2,2},{1,1},{2,1},{2,2},{2,3},{1,1},{2,1},{3,1},{1,2},{1,0},{1,1},{1,2},{2,2}};
int bl_5[16][2] = {{2,0},{1,1},{2,1},{1,2},{1,1},{2,1},{2,2},{3,2},{2,0},{1,1},{2,1},{1,2},{1,1},{2,1},{2,2},{3,2}};
int bl_6[16][2] = {{1,1},{2,1},{3,1},{2,2},{2,0},{1,1},{2,1},{3,1},{2,0},{2,1},{3,1},{2,2},{2,0},{1,1},{2,1},{2,2}};

int bricks[2048];
int t_bricks[2048];
int ibricks = 0;
int del_bricks[COLS-2];

int x = START_X;
int y = START_Y;
int n = 0;
int maxX = 0;
int maxY = 0;
int scrW = 0;
int scrH = 0;
int falling = 0;
int _pause = 0;
int game_on=1;

void usleep(long nanosecs); // Needed for use of -std=c99

void set_cur_block(int bl[16][2]);
void render();
void debug(int arr[], int size);
void rand_block();
void pack(int r);
void delete_bricks(int r);
void check_rows();
void set_cur_block(int bl[16][2]);
void render();
void clean();
void game_over();
int  reset(int r);
int  brick_at(int x, int y);
int  check_move_right(int bl[16][2], int r);
int  check_move_left(int bl[16][2], int r);
int  check_rotate(int bl[16][2], int r);

void rand_block(){

	srand(time(0));
	int r = rand() % 7;
	switch(r){
	  case 0:
	   set_cur_block(bl_0);
	 break;
	 case 1:
	   set_cur_block(bl_1);
	 break;
	 case 2:
	   set_cur_block(bl_2);
	 break;
	 case 3:
	   set_cur_block(bl_3);
	 break;
	 case 4:
	   set_cur_block(bl_4);
	 break;
	 case 5:
	   set_cur_block(bl_5);
	 break;
	 case 6:
	   set_cur_block(bl_6);
	 break;
	}
}

void clean(){

	int n = 0;
	for(int i = 0; i < ibricks; i++){
	  if(bricks[i]>-1) t_bricks[n++]=bricks[i];
	}

	for(int i = 0; i < n; i++){
	 bricks[i] = t_bricks[i];
	}
	ibricks = n;
}

void pack(int r){

	clean();
	for(int i = 0; i < ibricks; i++){
	 if(bricks[i]<r && bricks[i] >-1) {
	   bricks[i]++;
	 }
	 i++;
	}
}

void delete_bricks(int r){

	for(int i = 0; i < ibricks; i++){
	  bricks[del_bricks[i]] = -1;
	  bricks[del_bricks[i]+1] = -1;
	}

	for(int i = 0; i < COLS-2; i++) {
	  del_bricks[i] = 0;
	}
	pack(r);
}

void check_rows(){

	int n = 0;
	int d = 0;

	for(int r = 0; r < ROWS; r++){
	  for(int i = 0; i < ibricks; i++){
	    if(bricks[i++]==r) {
	     del_bricks[d++] = i-1;
	     n++;
	   }
	  }
	  if(n==COLS-2) {
	   delete_bricks(r);
	   points+=100;
	  }
	  n=0;
	  d=0;
	}
}

int brick_at(int x, int y){

	int ix,iy;
	for(int i = 0; i < ibricks-2; i++){
	  iy = bricks[i];
	  ix = bricks[i+1];
	  i++;
	  if(ix==x && iy==y) return 1;
	}

return 0;
}

void set_cur_block(int bl[16][2]){

	for(int i = 0; i < 16; i++){
	  for(int j = 0; j < 2; j++){
		cur_block[i][j] = bl[i][j];
	  }
	}
	n=0;
}

int check_move_left(int bl[16][2], int r){

        for(int i = r; i < r+4; i++){
 	  if(brick_at(bl[i][1]+(x-1),bl[i][0]+y)) return 0;
          if((bl[i][1]+x)-1 < 1) return 0;
        }

 return 1;
}

int check_move_right(int bl[16][2], int r){

	for(int i = r; i < r+4; i++){
 	  if(brick_at(bl[i][1]+(x+1),bl[i][0]+y)) return 0;
	  if((bl[i][1]+x)+1 >= maxX-1) return 0;
	}

 return 1;
}

int check_move_down(int bl[16][2], int r){

	for(int i = r; i < r+4; i++){
	 if(brick_at(bl[i][1]+x,bl[i][0]+(y+1))) return 0;
	 if((bl[i][0]+y)+1 >= maxY-1) return 0;
	}

 return 1;
}

int check_rotate(int bl[16][2], int r){

        for(int i = 0; i < 16; i++){
          for(int j = 0; j < 2; j++){
                temp_block[i][j] = bl[i][j];
          }
        }

	if(check_move_left(temp_block,r)==0) return 0;
	if(check_move_right(temp_block,r)==0) return 0;
	if(check_move_down(temp_block,r)==0) return 0;

  return 1;
}

int reset(int r){

	for(int i = n; i < n+4; i++){
	  bricks[ibricks] = cur_block[i][0]+y;
	  ibricks++;
	  bricks[ibricks] = cur_block[i][1]+x;
	  ibricks++;
	}
	check_rows();

	x=START_X;
	y=START_Y;

	SPEED = GAME_SPEED;

	falling = 0;
	rand_block();

        if(check_move_down(cur_block,r)==0) return 0;

 return 1;
}

void render(){

 	werase(gamewin);
	box(gamewin,ACS_VLINE,ACS_HLINE);
	wattron(gamewin,A_REVERSE);

	for(int i = 0; i < ibricks; i++){
	   mvwaddch(gamewin, bricks[i], bricks[i+1],brick);
	   i++;
	}

	for(int i = n; i < n+4; i++){
	   mvwaddch(gamewin,cur_block[i][0]+y,cur_block[i][1]+x,brick);
	 }
	wattroff(gamewin,A_REVERSE);

	refresh();
	mvwaddstr(gframe,1,4,&title[0]);
	if(_pause==1){
	  mvwaddstr(gamewin,6,2,&begin[0]);
	  box(gamewin,ACS_VLINE,ACS_HLINE);
	}
	sprintf(&score[0],"SCORE: %d", points);
	mvwaddstr(gframe, ROWS+2, 5, &score[0]);
	wrefresh(gframe);
	wrefresh(gamewin);
}

void game_over(){

	points=0;
	flash();
 	ibricks=0;
	_pause=1;
	render();
}

int main(int args, char* argv[]){

	int ch = 0;
	int tick = 0;
	int tr;

	initscr();
	noecho();
	nodelay(stdscr,TRUE);
	curs_set(0);
	box(stdscr,ACS_VLINE,ACS_HLINE);
	start_color();
	init_pair(1,COLOR_BLACK,COLOR_WHITE);
	getmaxyx(stdscr,scrH,scrW);
	gframe = newwin(ROWS+4,COLS+4,((scrH/2)-(ROWS/2))-2,((scrW/2)-(COLS/2))-2);
	gamewin = newwin(ROWS,COLS,(scrH/2)-(ROWS/2),(scrW/2)-(COLS/2));
	wbkgd(gamewin, COLOR_PAIR(1));
	box(gframe, ACS_VLINE,ACS_HLINE);
	box(gamewin, ACS_VLINE, ACS_HLINE);
	rand_block();

	refresh();
	wrefresh(gframe);
	wrefresh(gamewin);

	_pause = 1;
	render();

	getmaxyx(gamewin, maxY, maxX);

	while(game_on){
	 ch = getch();
	 if(ch=='q') {
          break;
	 }
	 else if(ch==65 || ch=='s') {
	  tr=n;
	  n+=4;
	  if(n>12) n=0;
	  if(!check_rotate(cur_block,n)) n = tr;
	 }else if(ch==66){
	  tr=n;
	  n-=4;
	  if(n<0) n = 12;
	  if(!check_rotate(cur_block,n)) n = tr;
         }
	 else if(ch==67 || ch=='d') {
	  if(check_move_right(cur_block,n)) x+=1;
	 }
	 else if(ch==68 || ch=='a') {
	  if(check_move_left(cur_block,n)) x-=1;
	 }
	 else if(ch==' ') {
	  if(_pause==1) _pause=0;
	  else{
           falling = 1;
	   SPEED = FALL_SPEED;
	  }
	 }else if(ch=='m'){
	   n+=2;
	 }

	 // SLEEPER i.e GAME TICKER.
	 usleep(SPEED);

	 if(tick>6){
	  if(check_move_down(cur_block,n)){
	   y++;
	  }else{
	    if(!reset(n)) game_over();
	  }
	  tick=0;
	 }else if(ch=='p' || ch == KEY_ENTER) {
	   _pause = !_pause;
	 }

	if(!_pause){
	   tick++;
	   render();
	 }
	}

	endwin();

 return 0;
}


