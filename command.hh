#ifndef command_hh
#define command_hh
#include <unistd.h>     // for fork(), execvp(), dup(), dup2(), pipe(), close()
#include <sys/types.h>  // for pid_t
#include <sys/wait.h>   // for waitpid()
#include <fcntl.h>      // for open(), O_CREAT, O_WRONLY, O_RDONLY, O_APPEND, O_TRUNC
#include <cstdlib>      // for malloc(), free(), exit()
#include <cstdio>       // for perror()
#include <cstring>      // for strdup()

#include "simpleCommand.hh"

// Command Data Structure

struct Command {
  //一个叫做_simpleCommands的vector变量，这个vector存放SimpleCommand指针类型的变量；
  //每个格子里放一个指向SimpleCommand的指针；
  //而SimpleCommand这个vector每个格子存放指向argument的指针；
  //argument这个vector每个格子存放指向一个string的指针。
  std::vector<SimpleCommand *> _simpleCommands; 
  std::string * _outFile;
  std::string * _inFile;
  std::string * _errFile;
  bool _appendOutFile;
  bool _appendErrFile;
  bool _usesPipe;
  bool _background;

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void clear();
  void print();
  void execute();
  void setOutFile(std::string *filename);
  void setAppendOutFile(std::string *filename);
  void setErrFile(std::string *filename);
  void setAppendErrFile(std::string *filename);
  void setInFile(std::string *filename);
  void setBackground();
  void insertPipe();

  static SimpleCommand *_currentSimpleCommand; //_currentSimpleCommand 只是一个SimpleCommand structure类型的指针
};

#endif
