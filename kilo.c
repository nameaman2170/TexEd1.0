		/*INCLUDES*/
#include<stdio.h>
#include<ctype.h>
#include<stdlib.h>
#include<termios.h>
#include<unistd.h>
#include<errno.h>
#include<sys/ioctl.h>
		/*DEFINES*/
#define CTRL_KEY(k) ((k) & 0x1f)

		/*DATA*/
		
struct editorConfig{
	int screenrows;
	int screencols;
	struct termios orig_termios;
};
struct editorConfig E;
		/*TERMINAL*/

void die(const char *s){
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(s);
	exit(1);
}
void disableRawMode(){
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
		die("tcsetattr");	
}
void enableRawMode(){
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
		die("tcgetattr");
	atexit(disableRawMode);
	struct termios raw = E.orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_cflag |= (CS8);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		die("tcsetattr");
}
char editorReadKey(){
	int nread;
	char c;
	while(nread = read(STDIN_FILENO, &c, 1) != 1){
		if(nread == -1 && errno != EAGAIN)
			die("read");
	}
	return c;
}
// in windows read() does not work properly and 
// goes in infinite loop
int getCursorPosition(int *rows, int *cols){
	char buf[32];
	unsigned int i = 0;

	if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
		
	while(i < sizeof(buf)-1 ){
		if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if(buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';
	if(buf[0] != '\x1b' || buf[1] != '[') return -1;
	if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
	
	return 0;
}
int getWindowSize(int *rows, int *cols){
	struct winsize ws;
	
	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
		if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) 
			return getCursorPosition(rows,cols);
	}else{
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

		/*Append Buffer*/
struct abuf{
	char *b;
	int len;
};
#define ABUF_INIT {NULL,0};

void abAppend(struct buf *ab, const char *s, int len ){
	char *newstr = realloc(ab->b, ab->len + len);
	if(newstr == NULL) return;
	memcpy(&newtsr[ab->len], s, len);
	ab->b = newstr;
	ab->len += len;
}
void abFree(struct abuf *ab){
	free(ab->b);
}
		/*input*/

void editorProcessKeypress(){
	char c = editorReadKey();
	switch(c){
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
	}
}

	/*OUTPUT*/


void editorDrawRows(){
	for(int i = 0; i < E.screenrows; i++){
		write(STDOUT_FILENO,"~", 1);
		if(i < E.screenrows-1)
			write(STDOUT_FILENO,"\r\n", 2);
	}
}

void editorRefreshScreen(){
	write(STDOUT_FILENO, "\x1b[2J", 4);//x1b for escape(27)
	write(STDOUT_FILENO, "\x1b[H", 3);
	editorDrawRows();
	write(STDOUT_FILENO, "\x1b[H", 3);
}
	
	/*INIT*/
void initEditor(){
	if(getWindowSize(&E.screenrows, &E.screencols) == -1)
		die("getWindowSize");
}
int main(){
	enableRawMode();
	initEditor();
	while (1){
		editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}
