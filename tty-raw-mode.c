
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>

/* 
 * Sets terminal into raw mode. 
 * This causes having the characters available
 * immediately instead of waiting for a newline. 
 * Also there is no automatic echo.
 */
static struct termios saved_attributes;
void reset_input_mode(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}
void tty_raw_mode(void)
{
	struct termios tty_attr;

	//tcgetattr(0, &tty_attr);
	tcgetattr(STDIN_FILENO, &saved_attributes);
	atexit(reset_input_mode);
	/* Set raw mode. */
	tty_attr = saved_attributes;
	tty_attr.c_lflag &= (~(ICANON|ECHO));
	tty_attr.c_cc[VTIME] = 0;
	tty_attr.c_cc[VMIN] = 1;
     
	//tcsetattr(0,TCSANOW,&tty_attr);
	tcsetattr(STDIN_FILENO, TCSANOW, &tty_attr);
}
