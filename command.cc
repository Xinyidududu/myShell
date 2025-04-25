/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>

#include <iostream>

#include "command.hh"
#include "shell.hh"
// extern "C" {
//     #include "lex.yy.cc"
// }
int yyparse(void);

Command::Command()
{
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _usesPipe = false;
    _appendErrFile = false;
    _appendOutFile = false;
}
void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear()
{
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands)
    {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if (_outFile)
    {
        delete _outFile;
    }
    _outFile = NULL;

    if (_inFile)
    {
        delete _inFile;
    }
    _inFile = NULL;

    if (_errFile)
    {
        delete _errFile;
    }
    _errFile = NULL;

    _background = false;
    _usesPipe = false;
    _appendErrFile = false;
    _appendOutFile = false;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for (auto &simpleCommand : _simpleCommands)
    {
        printf("  %-3d ", i++);
        simpleCommand->print();
    }

    printf("\n\n");
    printf("  Output       Input        Error        Background\n");
    printf("  ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s\n",
           _outFile ? _outFile->c_str() : "default",
           _inFile ? _inFile->c_str() : "default",
           _errFile ? _errFile->c_str() : "default",
           _background ? "YES" : "NO");
    //_usesPipe ? "YES" : "NO");
    printf("\n\n");
}

void Command::setOutFile(std::string *filename)
{
    // if (_outFile)
    // {
    //     fprintf(stderr, "Error: multiple output redirection\n");
    //     return;
    // }
    _outFile = filename;
}

void Command::setAppendOutFile(std::string *filename)
{
    // if (_outFile)
    // {
    //     fprintf(stderr, "Error: multiple output redirection\n");
    //     return;
    // }
    _outFile = filename;
    _appendOutFile = true;
}

void Command::setErrFile(std::string *filename)
{
    // if (_errFile)
    // {
    //     fprintf(stderr, "Error: multiple output redirection\n");
    //     return;
    // }
    _errFile = filename;
}

void Command::setAppendErrFile(std::string *filename)
{
    // if (_errFile)
    // {
    //     fprintf(stderr, "Error: multiple output redirection\n");
    //     return;
    // }
    _errFile = filename;
    _appendErrFile = true;
}

void Command::setBackground()
{
    _background = true;
}

void Command::insertPipe()
{
    // printf("Command::insertPipe\n");
    _usesPipe = true;
}

void Command::setInFile(std::string *filename)
{
    // if (_inFile)
    // {
    //     fprintf(stderr, "Error: multiple input redirection\n");
    //     return;
    // }
    _inFile = filename;
}

void Command::execute()
{
    // Don't do anything if there are no simple commands
    if (_simpleCommands.size() == 0)
    {
        Shell::prompt();
        return;
    }

    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperr = dup(2);

    int fdin;
    if (_inFile)
    {
        fdin = open(_inFile->c_str(), O_RDONLY);
    }
    else
    {
        // Use default input
        fdin = dup(tmpin);
    }
    int ret;
    int fdout;
    int fderr;

    for (size_t i = 0; i < _simpleCommands.size(); i++)
    {
        dup2(fdin, 0);
        close(fdin);
        if (i == _simpleCommands.size() - 1)
        {                 // last command
            if (_outFile) // handle outfile
            {
                fdout = open(_outFile->c_str(), _appendOutFile ? O_WRONLY | O_CREAT | O_APPEND : O_WRONLY | O_CREAT | O_TRUNC, 0666);
            }
            else
            {
                fdout = dup(tmpout);
            }
            if (_errFile) // handle errorfile
            {
                fderr = open(_errFile->c_str(), _appendErrFile ? O_WRONLY | O_CREAT | O_APPEND : O_WRONLY | O_CREAT | O_TRUNC, 0666);
            }
            else
            {
                fderr = dup(tmperr);
            }
        }
        else
        { // not the last command - add pipe
            int fdpipe[2];
            pipe(fdpipe);
            fdout = fdpipe[1]; // out
            fdin = fdpipe[0];  // in
        }
        dup2(fdout, 1); // redir output and error
        close(fdout);
        dup2(fderr, 2);
        close(fderr);

        std::string cmd = *_simpleCommands[i]->_arguments[0]; // get current command
        // --- setenv ---
        if (cmd == "setenv")
        {
            if (_simpleCommands[i]->_arguments.size() < 3)
            {
                fprintf(stderr, "Usage: setenv VAR VALUE\n");
            }
            else
            {
                if (setenv(_simpleCommands[i]->_arguments[1]->c_str(), _simpleCommands[i]->_arguments[2]->c_str(), 1) != 0)
                {
                    perror("sentenv");
                }
            }
            clear();
            Shell::prompt();
            dup2(tmpin, 0);
            dup2(tmpout, 1);
            dup2(tmperr, 2);
            close(tmpin);
            close(tmpout);
            close(tmperr);
            return;
        }
        // --- unsetenv ---
        if (cmd == "unsetenv")
        {
            if (_simpleCommands[i]->_arguments.size() < 2)
            {
                fprintf(stderr, "Usage: unsetenv VAR\n");
            }
            else
            {
                if (unsetenv(_simpleCommands[i]->_arguments[1]->c_str()) != 0)
                {
                    perror("unsentenv");
                }
            }
            clear();
            Shell::prompt();
            dup2(tmpin, 0);
            dup2(tmpout, 1);
            dup2(tmperr, 2);
            close(tmpin);
            close(tmpout);
            close(tmperr);
            return;
        }
        // --- cd ---
        if (cmd == "cd")
        {
            const char *path = (_simpleCommands[i]->_arguments.size() > 1) ? _simpleCommands[i]->_arguments[1]->c_str() : getenv("HOME");
            if (chdir(path) != 0)
            {
                fprintf(stderr, "cd: can't cd to notfound\n");
            }
            clear();
            Shell::prompt();
            dup2(tmpin, 0);
            dup2(tmpout, 1);
            dup2(tmperr, 2);
            close(tmpin);
            close(tmpout);
            close(tmperr);
            return;
        }

        if (!_simpleCommands.empty())
        {
            const auto &lastCmd = _simpleCommands.back();
            std::string lastArg = "";
            for (size_t j = 0; j < lastCmd->_arguments.size(); ++j) // from the start of args(arg[0]) to the end
            {
                const std::string &arg = *lastCmd->_arguments[j];
                if (isRedirectSymbol(arg)) // if we meet the redirection; $(_)
                {
                    j++;
                    continue;
                }
                lastArg = arg;
            }
            Shell::_lastArgument = lastArg;
        }

        ret = fork(); // Create child process
        if (ret == 0)
        {
            // Convert SimpleCommand arguments to char* (non-const)
            const char **args = (const char **)malloc((_simpleCommands[i]->_arguments.size() + 1) * sizeof(char *)); // pointer char array
            for (size_t j = 0; j < _simpleCommands[i]->_arguments.size(); j++)
            {
                args[j] = _simpleCommands[i]->_arguments[j]->c_str();
            }
            args[_simpleCommands[i]->_arguments.size()] = NULL; // make sure the end of agrs is null(this is the execvp rule)
            // --- printenv ---
            if (std::string(args[0]) == "printenv")
            {
                extern char **environ;
                char **p = environ;
                while (*p != NULL)
                {
                    printf("%s\n", *p);
                    p++;
                }
                free(args);
                // dup2(tmpin, 0);
                // dup2(tmpout, 1);
                // dup2(tmperr, 2);
                // close(tmpin);
                // close(tmpout);
                // close(tmperr);
                exit(0);
            }
            execvp(args[0], (char *const *)args); // execvp(command like ls, grep, arguments like -l)
            perror("execvp");
            free(args);
            dup2(tmpin, 0);
            dup2(tmpout, 1);
            dup2(tmperr, 2);
            close(tmpin);
            close(tmpout);
            close(tmperr);
            exit(1);
        }
    }
    dup2(tmpin, 0);
    dup2(tmpout, 1);
    dup2(tmperr, 2);
    close(tmpin);
    close(tmpout);
    close(tmperr);

    if (_background)
    {
        Shell::_lastBackGroundid = ret; // get last process's (run in the background) pid ${!}
    }
    else
    {
        // waitpid(ret, NULL, 0);
        int status;
        waitpid(ret, &status, 0); // 等待最后一个前台命令
        if (WIFEXITED(status))
        {
            Shell::_lastStatus = WEXITSTATUS(status); // exit code of last command ${?}
        }
        else
        {
            Shell::_lastStatus = -1; // 异常退出
        }
        // NULL means don't care about the child process's exit status.
        // 0 means block and wait until the child process exits.
    }
    // Add execution here
    // For every simple command fork a new process
    // Setup i/o redirection
    // and call exec

    // Clear to prepare for next command
    clear();

    // Print new prompt
    Shell::prompt();
}

SimpleCommand *Command::_currentSimpleCommand;
