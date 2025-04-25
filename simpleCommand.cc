#include <cstdio>
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>
#include <dirent.h>
#include <regex.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <iostream>

#include "simpleCommand.hh"
#include "shell.hh"
#define MAXFILENAME 1024

SimpleCommand::SimpleCommand()
{
  _arguments = std::vector<std::string *>();
}

SimpleCommand::~SimpleCommand()
{
  // iterate over all the arguments and delete them
  for (auto &arg : _arguments)
  {
    delete arg;
  }
}
void SimpleCommand::insertArgumentLiteral(const std::string &arg)
{
  _arguments.push_back(new std::string(arg));
}

void expandWildcard(char *prefix, char *suffix)
{

  if (suffix[0] == '\0')
  {
    Command::_currentSimpleCommand->insertArgument(new std::string(prefix));
    return;
  }
  char *s = strchr(suffix, '/'); // find first / in suffix
  char component[MAXFILENAME];
  if (s != NULL)
  {
    strncpy(component, suffix, s - suffix); // suffix = "src/*.cpp", s = 4, component = src, suffix = *.cpp
    component[s - suffix] = '\0';
    suffix = s + 1;
  }
  else
  {
    strcpy(component, suffix);
    suffix = suffix + strlen(suffix);
  }
  if (!strchr(component, '*') && !strchr(component, '?') && suffix[0] != '\0')
  {
    char newPrefix[MAXFILENAME];
    if (strcmp(prefix, "") == 0)
    {
      snprintf(newPrefix, MAXFILENAME, "%s", component);
    }
    else if (strcmp(prefix, "/") == 0)
    {
      snprintf(newPrefix, MAXFILENAME, "/%s", component);
    }
    else
    {
      snprintf(newPrefix, MAXFILENAME, "%s/%s", prefix, component); // connect prefix and component to prefix/component as a new var newPrefix, limit length to MAXFILENAME
    }
    expandWildcard(newPrefix, suffix);
    return;
  }

  char regexStr[MAXFILENAME * 2];
  char *a = component, *r = regexStr;
  *r++ = '^';
  while (*a)
  {
    if (*a == '*')
    {
      *r++ = '.';
      *r++ = '*';
    }
    else if (*a == '?')
    {
      *r++ = '.';
    }
    else if (*a == '.')
    {
      *r++ = '\\';
      *r++ = '.';
    }
    else
    {
      *r++ = *a;
    }
    a++;
  }
  *r++ = '$';
  *r = '\0';
  // fprintf(stderr, "[REGEX] generated pattern = %s\n", regexStr);

  regex_t re;
  if (regcomp(&re, regexStr, REG_EXTENDED | REG_NOSUB) != 0)
  // &re: stores the compiled regular expression structure;
  // regexStr: the regular expression string you want to match (such as "^.*\\.cpp$");
  // REG_EXTENDED: uses extended POSIX regular expression syntax;
  // REG_NOSUB: means we don't care about the capture group result, only whether it matches.
  {
    return;
  }
  char dir[MAXFILENAME];
  if (strcmp(prefix, "") == 0)
    strcpy(dir, ".");
  else
    strcpy(dir, prefix);

  DIR *d = opendir(dir);
  if (!d)
  {
    return;
  }
  std::vector<std::string> matches;
  struct dirent *ent;
  while ((ent = readdir(d)) != NULL)
  {
    if (ent->d_name[0] == '.' && component[0] != '.')
      continue;

    if (regexec(&re, ent->d_name, 1, NULL, 0) == 0)
    {
      matches.push_back(ent->d_name);
    }
  }
  std::sort(matches.begin(), matches.end());
  for (const auto &match : matches)
  {
    char newPrefix[MAXFILENAME];
    if (strcmp(prefix, "") == 0)
    {
      sprintf(newPrefix, "%s", match.c_str());
    }
    else if (strcmp(prefix, "/") == 0)
    {
      snprintf(newPrefix, MAXFILENAME, "/%s", match.c_str());
    }
    else
    {
      sprintf(newPrefix, "%s/%s", prefix, match.c_str());
    }
    // fprintf(stderr, "[MATCH] full path = %s\n", newPrefix);
    char *suffixCopy = strdup(suffix);
    expandWildcard(newPrefix, suffixCopy);
    free(suffixCopy);
  }
  closedir(d);
  regfree(&re);
}

void expandWildcardsIfNecessary(char *arg)
{
  // --- Return if arg does not contain ‘*’ or ‘?’ ---
  if (!strchr(arg, '*') && !strchr(arg, '?'))
  {
    Command::_currentSimpleCommand->insertArgument(new std::string(arg));
    return;
  }
  size_t beforeCount = Command::_currentSimpleCommand->_arguments.size();
  // fprintf(stderr, "[DEBUG] wildcard input = %s\n", arg);
  if (arg[0] == '/')
  {
    char *start = strdup(arg + 1);
    expandWildcard((char *)"/", start);
    free(start);
  }
  else if (strchr(arg, '/'))
  {
    expandWildcard((char *)"", arg);
  }
  else
  {
    // Convert “*” -> “.*”
    // “?” -> “.”
    // “.” -> “\.” and others you need
    // Also add ^ at the beginning and $ at the end to match
    // the begining the end of the word.
    // Allocate enough space ing ant for regular expression
    // --- wildcard converts to regex ---
    char *reg = (char *)malloc(2 * strlen(arg) + 10);
    char *a = arg;
    char *r = reg;
    *r = '^';
    r++;
    while (*a)
    {
      if (*a == '*')
      {
        *r = '.';
        r++;
        *r = '*';
        r++;
      }
      else if (*a == '?')
      {
        *r = '.';
        r++;
      }
      else if (*a == '.')
      {
        *r = '\\';
        r++;
        *r = '.';
        r++;
      }
      else
      {
        *r = *a;
        r++;
      }
      a++;
    }
    *r = '$';
    r++;
    *r = '\0';

    // --- compile regx ---
    regex_t re;
    int result = regcomp(&re, reg, REG_EXTENDED | REG_NOSUB);
    if (result != 0)
    {
      perror("regcomp");
      free(reg);
      return;
    }

    // --- open current dir ---
    DIR *dir = opendir(".");
    if (dir == NULL)
    {
      perror("opendir");
      regfree(&re);
      free(reg);
      return;
    }

    // --- go through all files in current dir ---
    struct dirent *ent;
    int maxEntries = 20;
    int nEntries = 0;
    char **array = (char **)malloc(maxEntries * sizeof(char *));
    while ((ent = readdir(dir)) != NULL) // read one file from cur dir
    {
      if (ent->d_name[0] == '.' && arg[0] != '.') // if the file name starts from . but our command is not .*, we skip it
      {
        continue;
      }
      // Check if filename matches the compiled regex
      if (regexec(&re, ent->d_name, 1, NULL, 0) == 0)
      {
        // Ensure space in array
        if (nEntries == maxEntries)
        {
          maxEntries *= 2;
          array = (char **)realloc(array, maxEntries * sizeof(char *));
          assert(array != NULL);
        }

        // Add matched filename
        array[nEntries++] = strdup(ent->d_name);
      }
    }
    closedir(dir);
    regfree(&re);
    free(reg);

    // --- sort results ---
    std::sort(array, array + nEntries, [](const char *a, const char *b)
              { return strcmp(a, b) < 0; });

    // --- insert into arguments
    for (int i = 0; i < nEntries; i++)
    {
      Command::_currentSimpleCommand->insertArgument(new std::string(array[i]));
      free(array[i]);
    }
    free(array);
  }
  if (Command::_currentSimpleCommand->_arguments.size() == beforeCount) {
    Command::_currentSimpleCommand->insertArgumentLiteral(std::string(arg));
  }
}

std::string expandTilde(const std::string &arg)
{
  if (arg[0] != '~')
    return arg;
  std::string result;
  size_t slashPos = arg.find('/');
  // get user name, if we can find '/', and if not
  std::string userPart = (slashPos == std::string::npos) ? arg.substr(1) : arg.substr(1, slashPos - 1);
  // 如果没有 /，那就没有后缀路径，直接设为空字符串 ""。 如果有 /，就从 / 开始提取剩下的路径，比如 /dir.
  std::string suffix = (slashPos == std::string::npos) ? "" : arg.substr(slashPos);
  struct passwd *pw = nullptr;
  if (userPart.empty())
  {
    pw = getpwuid(getuid());
  }
  else
  {
    pw = getpwnam(userPart.c_str());
  }
  if (pw)
  {
    result = pw->pw_dir + suffix;
  }
  else
  {
    result = arg;
  }
  return result;
}

bool isRedirectSymbol(const std::string &arg)
{
  return arg == ">" || arg == ">>" || arg == "<" || arg == "2>" || arg == ">&" || arg == ">>&";
}

void SimpleCommand::insertArgument(std::string *argument)
{
  // simply add the argument to the vector
  if (!argument) // if null
  {
    fprintf(stderr, "Error: NULL argument in insertArgument\n");
    return;
  }
  std::string arg = *argument;
  // --- check wildcard ---
  if ((arg.find('*') != std::string::npos || arg.find('?') != std::string::npos) && !isRedirectSymbol(arg) && arg.find("${") == std::string::npos)
  {
    expandWildcardsIfNecessary((char *)arg.c_str());
    delete argument;
    return;
  }

  // --- check if need variable expansion ---
  if (isRedirectSymbol(arg))
  {
    _arguments.push_back(argument); // 原样加入，不进行 expansion
    return;
  }

  // --- check if it is ~ ---
  arg = expandTilde(arg);                   // if it is ~, we expand it
  std::string expanded;                     // the real argument after expansion
  for (size_t i = 0; i < arg.length(); i++) // go through all chars in arg
  {
    if (arg[i] == '$' && i + 1 < arg.length() && arg[i + 1] == '{') // if this arg starts ${..., else we handle it as normal argument
    {
      size_t j = i + 2; // the first char next to {
      std::string varName;

      while (j < arg.length() && arg[j] != '}')
      {
        varName += arg[j]; // get all chars for this args, because it can be "SHELL"
        j++;
      }

      if (j < arg.length() && arg[j] == '}') // if j is the last char which near }
      {
        i = j; // we have already scan the whole line, change i's spot

        if (varName == "$")
        {
          expanded += std::to_string(getpid());
        }
        else if (varName == "?")
        {
          expanded += std::to_string(Shell::_lastStatus);
        }
        else if (varName == "!")
        {
          expanded += std::to_string(Shell::_lastBackGroundid);
        }
        else if (varName == "_")
        {
          expanded += Shell::_lastArgument;
        }
        else if (varName == "SHELL")
        {
          expanded += Shell::_shellPath;
        }
        else
        {
          const char *val = getenv(varName.c_str()); // for case ${HOME}, HOME = /home/guan90
          if (val)
          {
            expanded += val; // expand += "/home/guan90"
          }
        }
      }
      else
      // If the closing curly brace } is not found (which is a syntax error),
      // we keep it as it is. For example, if the input is ${HOME, we keep it as $HOME.
      {
        expanded += "$" + varName;
      }
    }
    else
    {
      expanded += arg[i]; // normal arguments
    }
  }
  _arguments.push_back(new std::string(expanded)); // no matter it needs expand or not, push it as an argument
  delete argument;                                 // free std::string *argument from yacc (new -> delete)
}

// Print out the simple command
void SimpleCommand::print()
{
  // for (auto & arg : _arguments) {
  //   std::cout << "\"" << arg << "\" \t";
  // }
  // // effectively the same as printf("\n\n");
  // std::cout << std::endl;
  for (size_t i = 0; i < _arguments.size(); i++)
  {
    printf("\"%s\" ", _arguments[i]->c_str());
  }
  printf("\n");
}
