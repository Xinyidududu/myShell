#ifndef shell_hh
#define shell_hh

#include "command.hh"

struct Shell {

  static void prompt();
  static int _lastStatus;
  static int _lastBackGroundid;
  static std::string _shellPath;
  static std::string _lastArgument;
  static Command _currentCommand;
  
};
bool isRedirectSymbol(const std::string &arg);
#endif
