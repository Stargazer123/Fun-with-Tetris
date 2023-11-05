/*
 Simple terminal client/server Tetris for Linux ncurses.
 By: Fredrik Roos 2021
 mail: info@crazedout.com
 gcc -O3 -Wall -std=c99 -o netris netris.c -lncurses -lpthread
 Get ncurses:
 sudo apt-get install libncurses5-dev libncursesw5-dev
*/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#define SERVER 0
#define CLIENT 1
#define START_X 5
#define START_Y 1
#define COLS 16
#define ROWS 20

#define MAX 6000
#define SA struct sockaddr

#define GAME_ON		"GAME_ON"
#define GAME_OVER 	"GAME_OVER"
#define QUIT 		"QUIT"
#define CUR_BLOCK_NET   "CUR_BLOCK_NET:"

int PORT = 8080;
int MODE = -1;
int SOCKET_READ = 0;
int SOCKET_WRITE = 0;

WINDOW *gamewin,*gframe,*gamewin_net,*gframe_net;

char title[] = "T E T R I S";
char begin[] = "PRESS 'SPACE'\n   TO BEGIN\n\n USE ARROW KEYS\n\n  By TMB 2021 \n\n   'q' quits\n";
char beginc[] = "WAITING FOR\n   SERVER TO\n     BEGIN\n\n USE ARROW KEYS\n\n  By TMB 2021 \n\n   'q' quits\n";
char you_name[64] = "YOU\0";
char buddy_name[64] = "BUDDY\0";
char score[24];
int  points = 0;
char brick = '#';

int SPEED = 40000;
int GAME_SPEED = 55000;

int cur_block[16][2];
int temp_block[16][2];
int bl_0[16][2] = {{1,1},{2,1},{1,2},{2,2},{1,1},{2,1},{1,2},{2,2},{1,1},{2,1},{1,2},{2,2},{1,1},{2,1},{1,2},{2,2}};
int bl_1[16][2] = {{1,1},{2,1},{2,2},{2,3},{2,1},{0,2},{1,2},{2,2},{1,0},{1,1},{1,2},{2,2},{1,1},{2,1},{3,1},{1,2}};
int bl_2[16][2] = {{1,1},{2,1},{2,2},{3,2},{2,1},{1,2},{2,2},{1,3},{0,1},{1,1},{1,2},{2,2},{2,1},{1,2},{2,2},{1,3}};
int bl_3[16][2] = {{2,0},{2,1},{2,2},{2,3},{0,2},{1,2},{2,2},{3,2},{2,0},{2,1},{2,2},{2,3},{0,2},{1,2},{2,2},{3,2}};
int bl_4[16][2] = {{2,1},{0,2},{1,2},{2,2},{1,1},{2,1},{2,2},{2,3},{1,1},{2,1},{3,1},{1,2},{1,0},{1,1},{1,2},{2,2}};
int bl_5[16][2] = {{2,0},{1,1},{2,1},{1,2},{1,1},{2,1},{2,2},{3,2},{2,0},{1,1},{2,1},{1,2},{1,1},{2,1},{2,2},{3,2}};
int bl_6[16][2] = {{1,1},{2,1},{3,1},{2,2},{2,0},{1,1},{2,1},{3,1},{2,0},{2,1},{3,1},{2,2},{2,0},{1,1},{2,1},{2,2}};

int cur_block_net[17];
char cur_block_net_tmp[128];

int bricks[MAX];
int t_bricks[MAX];
int n_bricks[MAX];
int ibricks = 0;
int nbricks = 0;
int del_bricks[COLS-2];

int x = START_X;
int y = START_Y;
int n = 0;
int maxX = 0;
int maxY = 0;
int scrW = 0;
int scrH = 0;
int _pause = 0;
int game_on=1;

void usleep(long nanosecs); // Needed for use of -std=c99

void set_cur_block(int bl[16][2]);
void set_cur_block_net(const char* string);
void cur_block_to_string(char *string);
void render();
void debug(int arr[], int size);
void rand_block();
void pack(int r);
void delete_bricks(int r);
int  check_rows();
void render();
void clean();
void game_over();
int  reset(int r);
int  brick_at(int x, int y);
int  check_move_right(int bl[16][2], int r);
int  check_move_left(int bl[16][2], int r);
int  check_rotate(int bl[16][2], int r);
int  bricks_to_string(char* string);
void string_to_nbricks(const char* string);

int  socket_desc;
char bricks_str[MAX];
char net_tmp[MAX];

pthread_t thread_id;

char *get_IP(char *host);
int  init_client(const char* ip, int port);
int  init_server(int port);
char ip[16] = "127.0.0.1";
char buddyIP[INET_ADDRSTRLEN];
char ntmp[MAX];

char *get_IP(char *host) {

	struct hostent *hent;
	int iplen = 15;
	char *ip = (char *)malloc(iplen+1);

	memset(ip, 0, iplen+1);
	if((hent = gethostbyname(host)) == NULL){
	  perror("Can't get IP");
	  exit(1);
	}

	if(inet_ntop(AF_INET,(void *)hent->h_addr_list[0], ip, iplen) == NULL){
	  perror("Can't resolve the host");
	  exit(1);
  	}

  return ip;
}


int init_client(const char* ip, int port){

        int sockfd;
        struct sockaddr_in servaddr;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
                printf("Socket creation failed on %d..\n",port);
                exit(0);
        }

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(&ip[0]);
        servaddr.sin_port = htons(port);

        if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                printf("Connection with the server failed: %d...\n",port);
                exit(0);
        }

 return sockfd;
}

int init_server(int port) {

        unsigned int sockfd, connfd, len;
        struct sockaddr_in servaddr, cli;
	struct sockaddr_in* pV4Addr;
	struct in_addr ipAddr;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
                printf("socket creation failed...\n");
                exit(0);
        }

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);


        if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
                printf("Socket bind failed...\n");
                exit(0);
        }

        if ((listen(sockfd, 5)) != 0) {
                printf("Listen failed...\n");
                exit(0);
        }
        len = sizeof(cli);

	printf("Waiting for client connection on port %d\n", PORT);
        connfd = accept(sockfd, (SA*)&cli, &len);
	pV4Addr = (struct sockaddr_in*)&cli;
	ipAddr = pV4Addr->sin_addr;
	inet_ntop(AF_INET, &ipAddr, buddyIP, INET_ADDRSTRLEN );

	if (connfd < 0) {
                printf("server accept failed...\n");
                exit(0);
        }

 return connfd;
}

int startsw(const char* str, const char* str2, int len){

        int n=0;
        if(strlen(str)<strlen(str2)) return -1;
        for(int i = 0; i < len; i++){
         if(str[i]!=str2[i]) n++;
        }

 return n;
}

int r;
void *net_read(void *vargp){

	while(1){
	  r = read(SOCKET_READ,&ntmp[0],MAX);
	  if(strcmp(&ntmp[0],GAME_ON)==0) {
	   _pause=0;
	   game_on=1;
 	  }
	  else if(strcmp(&ntmp[0],GAME_OVER)==0) {
	   game_over();
 	   break;
	  }
	  else if(strcmp(&ntmp[0],QUIT)==0) {
	   flash();
	   printf("QUIT\n");
	   game_on=0;
 	   break;
	  }else if(startsw(&ntmp[0],CUR_BLOCK_NET,14)==0){
	    set_cur_block_net(&ntmp[14]);
	  } else {
	    string_to_nbricks(&ntmp[0]);
	    memset(&ntmp[0],0,MAX);
 	  }
	}

  return NULL; // Must return otherwise: gcc warning.
}

void _do_nothing(){}

void net_write(const char* str, int len){

	int r = 0;
	r = write(SOCKET_WRITE, str, len);
	if(r<len) _do_nothing();

}

void cur_block_to_string(char *string){

	memset(&cur_block_net_tmp[0],0,128);
	strcpy(string, "CUR_BLOCK_NET:");
	for(int i = n; i < n+4; i++){
         sprintf(&cur_block_net_tmp[0],"%d,%d,", cur_block[i][0]+y, cur_block[i][1]+x);
         strcat(string, &cur_block_net_tmp[0]);
        }

}

void set_cur_block_net(const char* str){

        char tmp[4];
        int n = 0;
        char c = 0;
        int b = 0;

        for(int i = 0; i < strlen(&str[0]); i++){
          c = str[i];
          if(c==',') {
          tmp[n] = '\0';
          cur_block_net[b++] = atoi(&tmp[0]);
          n=0;
          continue;
        }else{
          tmp[n++] = c;
        }
       }
}

void string_to_nbricks(const char* str){

 	char tmp[4];
 	int n = 0;
 	char c = 0;
 	int b = 0;

 	for(int i = 0; i < strlen(&str[0]); i++){
   	  c = str[i];
   	  if(c==',') {
    	  tmp[n] = '\0';
	  n_bricks[b++] = atoi(&tmp[0]);
    	  n=0;
    	  continue;
   	}else{
    	  tmp[n++] = c;
   	}
	 nbricks=b+1;
       }
}


int bricks_to_string(char* string){

	int i;
	char temp[128];
 	for(i = 0; i < ibricks; i++){
	  sprintf(&temp[0],"%d,",bricks[i]);
	  strcat(string,&temp[0]);
	}

 return i;
}


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

int check_rows(){

	int res = 0;
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
	   res++;
	  }
	  n=0;
	  d=0;
	}
 return res;
}

int brick_at(int x, int y){

	int ix,iy;
	for(int i = 2; i < ibricks; i++){
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

	memset(&ntmp[0],0,MAX);
	bricks_to_string(&ntmp[0]);

	if(game_on) net_write(&ntmp[0],strlen(&ntmp[0]));

	x=START_X;
	y=START_Y;

	SPEED = GAME_SPEED;
	rand_block();

        if(check_move_down(cur_block,r)==0) return 0;

 return 1;
}


void render(){

 	werase(gamewin);
 	werase(gamewin_net);
	box(gamewin_net,ACS_VLINE,ACS_HLINE);
	wattron(gamewin_net,A_REVERSE);
	box(gamewin,ACS_VLINE,ACS_HLINE);
	wattron(gamewin,A_REVERSE);

	for(int i = 0; i < ibricks; i++){
	   mvwaddch(gamewin, bricks[i], bricks[i+1],brick);
	   i++;
	}

	for(int i = 0; i < nbricks; i++){
	   mvwaddch(gamewin_net, n_bricks[i], n_bricks[i+1], brick);
	   i++;
	 }

	if(_pause==0){
	for(int i = 0; i < 8; i++){
	   mvwaddch(gamewin_net, cur_block_net[i], cur_block_net[i+1], brick);
	   i++;
	 }
	}

	for(int i = n; i < n+4; i++){
           mvwaddch(gamewin,cur_block[i][0]+y,cur_block[i][1]+x,brick);
         }

	wattroff(gamewin,A_REVERSE);
	wattroff(gamewin_net,A_REVERSE);

	refresh();
	mvwaddstr(gframe,0,8,&you_name[0]);
	mvwaddstr(gframe_net,0,7,&buddy_name[0]);
	mvwaddstr(gframe,1,4,&title[0]);
	mvwaddstr(gframe_net,1,4,&title[0]);
	if(_pause==1){
	  if(MODE==SERVER){
	   mvwaddstr(gamewin,6,2,&begin[0]);
	  }else{
	   mvwaddstr(gamewin,6,2,&beginc[0]);	  }
	  box(gamewin,ACS_VLINE,ACS_HLINE);
	}
	sprintf(&score[0],"SCORE: %d", points);
	mvwaddstr(gframe, ROWS+2, 5, &score[0]);
	wrefresh(gframe);
	wrefresh(gamewin);
	wrefresh(gframe_net);
	wrefresh(gamewin_net);

}

int game(){

	int ch = 0;
	int tick = 0;
	int tr;

	initscr();
	noecho();
	nodelay(stdscr,TRUE);
	curs_set(0);

	initscr();
	noecho();
	nodelay(stdscr,TRUE);
	curs_set(0);

	box(stdscr,ACS_VLINE,ACS_HLINE);
	start_color();
	init_pair(1,COLOR_BLACK,COLOR_WHITE);
	getmaxyx(stdscr,scrH,scrW);

	gframe = newwin(ROWS+4,COLS+4,((scrH/2)-(ROWS/2))-2,((scrW/2)-(COLS/2))-2-20);
	gamewin = newwin(ROWS,COLS,(scrH/2)-(ROWS/2),(scrW/2)-(COLS/2)-20);
	wbkgd(gamewin, COLOR_PAIR(1));
	box(gframe, ACS_VLINE,ACS_HLINE);
	box(gamewin, ACS_VLINE, ACS_HLINE);

	gframe_net = newwin(ROWS+4,COLS+4,((scrH/2)-(ROWS/2))-2,(((scrW/2)-(COLS/2))-2)+20);
	gamewin_net = newwin(ROWS,COLS,(scrH/2)-(ROWS/2),(scrW/2)-(COLS/2)+20);
	wbkgd(gamewin_net, COLOR_PAIR(1));
	box(gframe_net, ACS_VLINE,ACS_HLINE);
	box(gamewin_net, ACS_VLINE, ACS_HLINE);

	rand_block();

	refresh();
	wrefresh(gframe_net);
	wrefresh(gamewin_net);
	wrefresh(gframe);
	wrefresh(gamewin);

	_pause = 1;
	render();

	getmaxyx(gamewin, maxY, maxX);

	while(game_on){
	 ch = getch();
	 if(ch=='q') {
          net_write("QUIT\0",5);
	  usleep(SPEED);
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
	 else if(ch==' ' && _pause==1 && MODE==SERVER) {
	  if(_pause==1) _pause=0;
	  net_write("GAME_ON\0",8);
	 }else if(ch=='m'){
	   n+=2;
	 }

	 // SLEEPER i.e GAME TICKER.
	 usleep(SPEED);

	 if(tick>6){
	  if(check_move_down(cur_block,n)){
	   y++;
	   memset(&net_tmp[0],0,MAX);
           cur_block_to_string(&net_tmp[0]);
	   net_write(&net_tmp[0],strlen(&net_tmp[0]));
	  }else{
	    if(!reset(n)) {
	     net_write("GAME_OVER\0",10);
	     game_over();
	    }
	  }
	  tick=0;
	 }

	if(!_pause){
	   tick++;
	   render();
	 }
	}

	endwin();

 return 0;
}

void game_over(){

	flash();
	points=0;
 	memset(bricks,0,MAX*sizeof(int));
 	ibricks=0;
 	memset(n_bricks,0,MAX*sizeof(int));
	nbricks=0;
	_pause=1;
	x = START_X;
	y = START_Y;
	render();

}

void usage(){

	 printf("netris - simple terminal Linux for ncurses\n");
	 printf("by Fredrik Roos 2021 info@crazedout.com\n\n");
	 printf("usage:\n");
	 printf("nettet -client [IP] [PORT] Default=8080\n");
	 printf("nettet -server [PORT]\n\n");
	 exit(1);
}


int main(int args, char* argv[]){

	if(args<2) {
	 usage();
	}else{
	 if(strcmp(argv[1],"-client")==0) {
	  if(args<4) usage();
	  MODE = CLIENT;
	  strcpy(&ip[0],argv[2]);
	  PORT = atoi(argv[3]);
	  SOCKET_WRITE = init_client(&ip[0],PORT);
	  SOCKET_READ = init_server(PORT+1);
	  strcpy(&buddy_name[0],&buddyIP[0]);
	 }
	 else if(strcmp(argv[1],"-server")==0) {
	  MODE = SERVER;
	  if(args==3) PORT=atoi(argv[2]);
	  SOCKET_READ = init_server(PORT);
   	  SOCKET_WRITE = init_client(&buddyIP[0],PORT+1);
	  strcpy(&buddy_name[0],&buddyIP[0]);
	 }else usage();
	}
	pthread_create(&thread_id,NULL, net_read, NULL);
        game();
	endwin();
	close(SOCKET_READ);
	close(SOCKET_WRITE);


 return 0;
}
