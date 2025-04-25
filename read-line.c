/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_BUFFER_LINE 2048
#define HISTORY_SIZE 100
extern void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];
int cursor_pos = 0;
// Simple history array

char *history[HISTORY_SIZE];
int history_length = 0;
int history_index = -1; // 当前正在回溯第几条记录（-1表示还没进入历史）

void read_line_print_usage()
{
  char *usage =
      "\n"
      " ctrl-?       Print usage\n"
      " Backspace    Deletes last character\n"
      " up arrow     See last command in the history\n"
      " Delete (ctrl-D) Deletes character at cursor\n"
      " Arrow keys   Move cursor left/right and navigate history\n"
      " Home (ctrl-A)/End (ctrl-E) Move to start/end\n";

  write(1, usage, strlen(usage));
}

void reset_line_buffer()
{
  line_buffer[0] = '\0';
  line_length = 0;
  cursor_pos = 0;
}

void clear_line()
{
  for (int i = 0; i < cursor_pos; i++)
    write(1, "\b", 1); // "\b" = backspace
  for (int i = 0; i < line_length; i++)
    write(1, " ", 1);
  for (int i = 0; i < line_length; i++)
    write(1, "\b", 1); // "\b" = backspace
}

void insert_char(char ch)
{
  if (line_length >= MAX_BUFFER_LINE - 1)
    return;
  for (int i = line_length; i > cursor_pos; i--)
  {
    line_buffer[i] = line_buffer[i - 1];
  }
  line_buffer[cursor_pos] = ch;
  line_length++;
  write(1, &line_buffer[cursor_pos], line_length - cursor_pos);
  for (int i = 0; i < line_length - cursor_pos - 1; i++)
    write(1, "\b", 1);
  cursor_pos++;
}

void path_complete()
{
  int start = cursor_pos - 1;
  while (start >= 0 && line_buffer[start] != ' ')
    start--;
  start++;
  int prefix_len = cursor_pos - start;
  if (prefix_len == 0)
    return;

  char prefix[MAX_BUFFER_LINE];
  strncpy(prefix, line_buffer + start, prefix_len);
  prefix[prefix_len] = '\0';

  DIR *dir = opendir(".");
  if (!dir)
  {
    perror("opendir");
    return;
  }

  struct dirent *entry;
  char *match = NULL;
  int match_count = 0;
  while ((entry = readdir(dir)) != NULL)
  {
    if (strncmp(entry->d_name, prefix, prefix_len) == 0)
    {
      if (match_count == 0)
      {
        match = strdup(entry->d_name);
      }
      else
      {
        // 找最长公共前缀
        int i = 0;
        while (match[i] && entry->d_name[i] && match[i] == entry->d_name[i])
        {
          i++;
        }
        match[i] = '\0'; // 缩短为公共前缀
      }
      match_count++;
    }
  }
  closedir(dir);

  if (match_count == 0)
  {
    return;
  }
  int match_len = strlen(match);
  for (int i = prefix_len; i < match_len; i++)
  {
    insert_char(match[i]);
  }
  free(match);
}
void path_list_all_matches()
{
  int start = cursor_pos - 1;
  while (start >= 0 && line_buffer[start] != ' ')
  {
    start--;
  }
  start++;
  int prefix_len = cursor_pos - start;
  if (prefix_len <= 0)
    return;

  char prefix[MAX_BUFFER_LINE];
  strncpy(prefix, line_buffer + start, prefix_len);
  prefix[prefix_len] = '\0';
  DIR *dir = opendir(".");
  if (!dir)
    return;

  struct dirent *entry;
  int match_count = 0;

  write(1, "\n", 1); // 换行再列出
  while ((entry = readdir(dir)) != NULL)
  {
    if (strncmp(entry->d_name, prefix, prefix_len) == 0)
    {
      write(1, entry->d_name, strlen(entry->d_name));
      write(1, "\t", 1);
      match_count++;
    }
  }
  closedir(dir);
  if (match_count > 0)
  {
    write(1, "\n", 1);
    //write(1, line_buffer, line_length); // 重新打印当前命令行
    // for (int i = line_length; i > cursor_pos; i--)
    //   write(1, "\b", 1); // 把光标移动回来
    const char *prompt = "myshell>";
    write(1, prompt, strlen(prompt));
  }
}

/*
 * Input a line with some basic editing.
 */
char *read_line()
{
  int tab_count = 0;
  static int initialized = 0;
  if (!initialized)
  {
    for (int i = 0; i < HISTORY_SIZE; i++)
      history[i] = NULL;
    initialized = 1;
  }
  // Set terminal in raw mode
  tty_raw_mode();
  line_length = 0;
  cursor_pos = 0;

  while (1)
  {
    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch >= 32 && ch != 127)
    {
      insert_char(ch);
      tab_count = 0;
    }
    else if (ch == 10)
    {
      // Enter
      write(1, &ch, 1);
      line_buffer[line_length] = '\0';

      if (line_length > 0)
      {
        if (history_length < HISTORY_SIZE)
        {
          history[history_length++] = strdup(line_buffer);
        }
        else
        {
          // 删除最旧的，腾出空间
          free(history[0]);
          for (int i = 1; i < HISTORY_SIZE; i++)
          {
            history[i - 1] = history[i];
          }
          history[HISTORY_SIZE - 1] = strdup(line_buffer);
        }
      }
      else if (line_length == 0)
      {
        // 打印换行（因为 Enter 触发了回车）
        write(1, "", 1);

        // 手动打印 prompt（你可以根据自己的样式修改）
        const char *prompt = "myshell>";
        write(1, prompt, strlen(prompt));

        // 清空缓冲区（防御性）
        line_buffer[0] = '\0';
        line_length = 0;
        cursor_pos = 0;
        tab_count = 0;
        break;
      }
      tab_count = 0;
      break;
    }
    else if (ch == 31)
    {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0] = 0;
      tab_count = 0;
      break;
    }
    else if (ch == 8 || ch == 127) // backspace
    {
      if (cursor_pos > 0)
      {
        for (int i = cursor_pos - 1; i < line_length - 1; i++)
          line_buffer[i] = line_buffer[i + 1];
        line_length--;
        cursor_pos--;
        write(1, "\b", 1);
        write(1, &line_buffer[cursor_pos], line_length - cursor_pos);
        write(1, " ", 1);
        for (int i = 0; i < line_length - cursor_pos + 1; i++)
          write(1, "\b", 1);
      }
      tab_count = 0;
    }
    else if (ch == 4)
    { // ctrl-D
      if (cursor_pos < line_length)
      {
        for (int i = cursor_pos; i < line_length - 1; i++)
        {
          line_buffer[i] = line_buffer[i + 1];
        }
        line_length--;
        write(1, &line_buffer[cursor_pos], line_length - cursor_pos);
        write(1, " ", 1);
        for (int i = 0; i <= line_length - cursor_pos; i++)
          write(1, "\b", 1);
      }
      tab_count = 0;
    }
    else if (ch == 1)
    { // ctrl-A
      while (cursor_pos > 0)
      {
        write(1, "\b", 1);
        cursor_pos--;
      }
      tab_count = 0;
    }
    else if (ch == 5)
    { // ctrl-E
      while (cursor_pos < line_length)
      {
        write(1, &line_buffer[cursor_pos], 1);
        cursor_pos++;
      }
      tab_count = 0;
    }
    else if (ch == 9)
    {
      if (tab_count == 1)
      {
        path_list_all_matches();
        tab_count = 0;
      }
      else
      {
        path_complete();
        tab_count = 1;
      }
    }
    else if (ch == 27) // esc
    {
      char ch1;
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1 == 91)
      {
        if (ch2 == 68 && cursor_pos > 0)
        { // left
          write(1, "\b", 1);
          cursor_pos--;
          tab_count = 0;
        }
        else if (ch2 == 67 && cursor_pos < line_length)
        { // right
          write(1, &line_buffer[cursor_pos], 1);
          cursor_pos++;
          tab_count = 0;
        }
        else if (ch2 == 65) // ↑ up arrow
        {
          if (history_length == 0)
            continue;
          if (history_index + 1 < history_length)
            history_index++;
          clear_line();
          // printf("history_length - 1 - history_index = %d\n", history_length - 1 - history_index);
          strcpy(line_buffer, history[history_length - 1 - history_index]);
          line_length = strlen(line_buffer);
          cursor_pos = line_length;
          write(1, line_buffer, line_length);
          tab_count = 0;
        }
        else if (ch2 == 66)
        { // down
          if (history_index > 0)
          {
            history_index--;
            clear_line();
            strcpy(line_buffer, history[history_length - 1 - history_index]);
            line_length = strlen(line_buffer);
            cursor_pos = line_length;
            write(1, line_buffer, line_length);
            tab_count = 0;
          }
          else if (history_index == 0)
          {
            history_index = -1;
            clear_line();
            line_buffer[0] = '\0';
            line_length = 0;
            cursor_pos = 0;
            tab_count = 0;
          }
        }
      }
    }
  }

  // Add eol and null char at the end of string
  line_buffer[line_length] = '\n';
  line_length++;
  line_buffer[line_length] = '\0';

  return line_buffer;
}