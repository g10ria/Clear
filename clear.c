#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define CTRL_KEY(k) ((k)&0x1f)

struct editorConfig {
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

struct editorConfig E;

void disableRawMode();
void enableRawMode();
void kill(const char *s);
char editorReadKey();
int getWindowDims();

void editorProcessKeyPress();

void editorRefreshScreen();
void editorDrawRows();

void initEditor();

/*** terminal ***/

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        kill("tcsetattr");
}

void enableRawMode()
{
    atexit(disableRawMode);

    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        kill("tcgetattr");
    struct termios raw = E.orig_termios;

    // turning off various flags in the "dump" output
    raw.c_lflag &= ~(IXON);                                   // disable CTRL S/Q/M
    raw.c_oflag &= ~(OPOST);                                  // turn off post-processing
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); // who knows
    raw.c_cflag |= (CS8);                                     // who knows
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN | ICRNL);  // turn of canonical mode, turn off CTRL C/Z/V

    // c_cc = control characters, array of bytes (controls various terminal settings)
    raw.c_cc[VMIN] = 0;  // min # of bytes needed before read() can return
    raw.c_cc[VTIME] = 1; // max amount of time to wait before read() returns (in tenths of a second)

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        kill("tcsetattr");
}

void kill(const char *msg)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(msg);
    exit(1);
}

char editorReadKey() {
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if ((nread==-1 && errno != EAGAIN)) kill("read");
    }
    return c;
}

int getCursorPosition(int *rows, int *cols) {
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    printf("\r\n");
    char c;
}

int getWindowDims(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*** input ***/

void editorProcessKeyPress() {
    char c = editorReadKey();

    switch(c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

/*** output ***/

void editorRefreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3);  // position cursor @ (1,1)

    editorDrawRows();

    write(STDOUT_FILENO, "\x1b[H", 3);
}

void editorDrawRows() {
    for(int y=0;y<E.screencols;y++) {
        write(STDOUT_FILENO, "~\r\n",3);
    }
}

/*** run ***/

void initEditor() {
    if (getWindowDims(&E.screenrows, &E.screencols) == -1) kill("getWindowSize");
}

int main(void) {
    enableRawMode();
    initEditor();

    while(1) {
        editorRefreshScreen();
        editorProcessKeyPress();
    }
    return 0;
}