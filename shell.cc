#include <cstdio>
#include <signal.h>
#include <iostream>
#include <limits.h>
#include <unistd.h>
#include "shell.hh"
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern int YY_BUF_SIZE;
void yypush_buffer_state(YY_BUFFER_STATE new_buffer);
void yy_switch_to_buffer(YY_BUFFER_STATE new_buffer);
void yypop_buffer_state(void);
YY_BUFFER_STATE yy_create_buffer(FILE *file, int size);
#define YY_BUF_SIZE 32768
// extern YY_BUFFER_STATE YY_CURRENT_BUFFER;
void yyrestart(FILE *file);
int yyparse(void);

int Shell::_lastStatus = 0;
int Shell::_lastBackGroundid = 0;
std::string Shell::_lastArgument = "";
std::string Shell::_shellPath = "";
extern "C" void reset_line_buffer();
extern "C" void disp(int sig)
{
  // fprintf( stderr, "\nsig:%d      Ouch!\n", sig);
  write(1, "^C\n ", 3);  // 输出提示符等
  reset_line_buffer();
  Shell::prompt();
}

extern "C" void zombie(int sig)
{
  // int pid = wait3(0, 0, NULL);
  while (waitpid(-1, NULL, WNOHANG) > 0)
  {
  };
  //	printf("[%d] exited.\n", pid);
}

void Shell::prompt()
{
  if (isatty(0))
  {
    printf("myshell>");
    fflush(stdout);
  }
  fflush(stdout);
}

int main()
{
  struct sigaction sa;
  sa.sa_handler = disp;
  sigemptyset(&sa.sa_mask); // make sure dont do anything about other actions
  sa.sa_flags = SA_RESTART; // will recall the disp if control-c

  if (sigaction(SIGINT, &sa, NULL))
  {
    perror("sigaction");
    exit(2);
  }

  struct sigaction sigZombie;
  sigZombie.sa_handler = zombie;
  sigemptyset(&sigZombie.sa_mask);
  sigZombie.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sigZombie, NULL))
  {
    perror("sigaction");
    exit(-1);
  }
  char path[PATH_MAX];
  if(realpath("/proc/self/exe", path) != NULL){ //conver a relative path to absolute path
    // /proc/self/exe can get the path of your shell executable.
    Shell::_shellPath = path;
  }else {
    perror("realpath");
    Shell::_shellPath = "";
  }
  bool ranRCFile = false;

  // FILE *rcfile = fopen(".shellrc", "r");
  // if (rcfile)
  // {
  //   yyrestart(rcfile); //clean up current buf and put rcfile into it
  //   yyparse();
  //   yyrestart(stdin); //go back to stdin
  //   fclose(rcfile);
  //   ranRCFile = true;
  // }

  if (isatty(0) && !ranRCFile)
  {
    Shell::prompt();
  }
  yyparse();
}

Command Shell::_currentCommand;
