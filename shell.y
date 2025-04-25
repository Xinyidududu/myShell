
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [ | cmd [arg]* ]* [ [> filename] [< filename] [2> filename] 
 *    [ >& filename] [>> filename] [>>& filename] ]* [&]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires
{
#include <string>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD
%token NOTOKEN GREAT NEWLINE GREATGREAT GREAT2 GREATAMP GREATGREATAMP LESS PIPE AMPERSAND

%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"

void yyerror(const char * s);
int yylex();

%}

%%

goal:
  command_list
  ;

command_list:
  command_line
  | command_list command_line
  ;

command_line:
  pipe_list iomodifier_list background_opt NEWLINE {
    //printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | NEWLINE
  | error NEWLINE { yyerrok; }
  ;

pipe_list:
  cmd_with_args
  | pipe_list PIPE cmd_with_args {
      //printf("   Yacc: insert pipe\n");
      Shell::_currentCommand.insertPipe();
    }
  ;

cmd_with_args:
  WORD {
    if (strcmp($1->c_str(), "exit") == 0){
      printf("Good bye!!\n");
      exit(1);
    }
    //printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument($1);
    Shell::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);
  } arg_list


arg_list:
  arg_list WORD {
    //printf("   Yacc: insert argument \"%s\"\n", $2->c_str());
    Command::_currentSimpleCommand->insertArgument($2);
  }
  | /* empty */
  ;

iomodifier:
  GREAT WORD {
    if (Shell::_currentCommand._outFile) {
      printf("Ambiguous output redirect.\n");
      //exit(0);
    } else {
      //printf("   Yacc: redirect stdout to \"%s\"\n", $2->c_str());
      Shell::_currentCommand.setOutFile($2);
    }
  }
  | GREATGREAT WORD {
    if (Shell::_currentCommand._outFile) {
      printf("Ambiguous output redirect.\n");
      //exit(0);
    } else {
      //printf("   Yacc: append stdout to \"%s\"\n", $2->c_str());
      Shell::_currentCommand.setAppendOutFile($2);
    }
  }
  | GREAT2 WORD {
    if (Shell::_currentCommand._errFile) {
      printf("Ambiguous output redirect.\n");
      //exit(0);
    } else {
      //printf("   Yacc: redirect stderr to \"%s\"\n", $2->c_str());
      Shell::_currentCommand.setErrFile($2);
    }
  }
  | GREATAMP WORD {
    if (Shell::_currentCommand._outFile || Shell::_currentCommand._errFile) {
      printf("Ambiguous output redirect.\n");
      //exit(0);
    } else {
      //printf("   Yacc: redirect stdout+stderr to \"%s\"\n", $2->c_str());
      Shell::_currentCommand.setOutFile(new std::string(*$2));
      Shell::_currentCommand.setErrFile(new std::string(*$2));
    }
  }
  | GREATGREATAMP WORD {
    if (Shell::_currentCommand._outFile || Shell::_currentCommand._errFile) {
      printf("Ambiguous output redirect.\n");
      //exit(0);
    } else {
      //printf("   Yacc: append stdout+stderr to \"%s\"\n", $2->c_str());
      Shell::_currentCommand.setAppendOutFile(new std::string(*$2));
      Shell::_currentCommand.setAppendErrFile(new std::string(*$2));
    }
  }
  | LESS WORD {
    if (Shell::_currentCommand._inFile) {
      printf("Ambiguous output redirect.\n");
      //exit(0);
    } else {
      //printf("   Yacc: redirect input from \"%s\"\n", $2->c_str());
      Shell::_currentCommand.setInFile($2);
    }
  }
  ;

iomodifier_list:
  iomodifier_list iomodifier
  | /* empty */
  ;

background_opt:
  AMPERSAND {
    //printf("   Yacc: run command in background\n");
    Shell::_currentCommand.setBackground();
  }
  | /* empty */
  ;

%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

#if 0
main()
{
  yyparse();
}
#endif
