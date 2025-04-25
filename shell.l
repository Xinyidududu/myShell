
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include "y.tab.hh"
#include "shell.hh"
#include <string.h>

extern YY_BUFFER_STATE yy_create_buffer(FILE *file, int size);
extern void yypush_buffer_state(YY_BUFFER_STATE new_buffer);
extern void yypop_buffer_state();
//////////// Start added code ///////////

extern "C" char * read_line();

int mygetc(FILE * f) {
	static char *p;
	char ch;
	if (!isatty(0)) {
		// stdin is not a tty. Call real getc
		return getc(f);
	}
	// stdin is a tty. Call our read_line.
	if (p==NULL || *p == 0) {
		char * s = read_line();
		p = s;
	}
	ch = *p;
	p++;
	return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////////// End added code ///////////

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

">>" {
  return GREATGREAT;
}

"<" {
  return LESS;
}

"2>" {
  return GREAT2;
}

">&" {
  return GREATAMP;
}

">>&" {
  return GREATGREATAMP;
}

"|" {
  return PIPE;
}

"&" {
  return AMPERSAND;
}

\$\([^\n]*\) {
	// --- get key words ---
	std::string str = yytext;
	str.erase(0, 2); // rm "$(" starts from 0, rm two chars
	str.pop_back(); // rm ")"
	char *subCommand = const_cast<char*>(str.c_str());
	//str = str.substr(2, str.size() - 3);
  
	int tempin = dup(0);
	int tempout = dup(1);

	int pin[2];	// shell -> subshell stdin（虽然我们没用上）
	int pout[2];	// subshell stdout -> shell
	pipe(pin);
	pipe(pout);

	// 写子进程要执行的 命令给它（subshell 会从pin[0]读）
	write(pin[1], subCommand, strlen(subCommand));
	write(pin[1], "\n", 1);
	close(pin[1]);	// 写完就关写端
	int ret = fork();

	if (ret == 0) {
		dup2(pin[0], 0);	 // fdpipein[0] -> stdin
		close(pin[0]);	     // 关掉旧描述符
		dup2(pout[1], 1);	 // stdout -> fdpipeout[1]
		close(pout[1]);     // 关掉旧写端
		execl("/proc/self/exe", "/proc/self/exe", (char*)NULL);
		_exit(1);
	//} else if (ret > 0){
		//close(pin[0]);
		//std::string commandToSend = str + "\n";
		//write(pin[1], commandToSend.c_str(), commandToSend.size());
		//close(pin[1]);
		//close(pout[1]);
	}else if (ret < 0) {
		perror("fork");
		exit(1);
	}

	close(pin[0]);
	close(pout[1]);
	
	// 读取子进程的输出
	char c;
	char *buffer = (char *)malloc(4096);
	int bytesRead = 0;
	while (bytesRead < 4095) {
    char c;
    ssize_t result = read(pout[0], &c, 1);
    if (result <= 0) {
        break;
    }
    buffer[bytesRead++] = (c == '\n') ? ' ' : c;
	}
	buffer[bytesRead] = '\0';
	close(pout[0]); //cuz shell finished reading
	waitpid(ret, NULL, 0);
	// 父进程恢复原始输入输出
	dup2(tempin, 0);
	dup2(tempout, 1);
	close(tempin);
	close(tempout);

	// 将输出反向注入回扫描器
	for (int j = bytesRead - 1; j >= 0; j--) {
		myunputc(buffer[j]); //unput is scanning from right to left!!
	}

	free(buffer);
}


["][^\n\"]*["] {
        yylval.cpp_string = new std::string(yytext);
        *yylval.cpp_string = yylval.cpp_string->substr(1, yylval.cpp_string->length()-2);
        return WORD;
}

[^ \t\n|><&]*\\[^ \t\n]* {
	char *str = strdup(yytext);
	char *newstr = (char*) malloc(strlen(str)+1);
	int i = 0;  // newstr 索引
	for (int j = 0; str[j] != '\0'; j++) {
		if (str[j] == '\\') {
        		if (str[j + 1] == '\\') {
            		// 处理 `\\` -> `\`
            			newstr[i++] = '\\';
            			j++;  // 跳过第二个 `\`
        		} else {
            		// 处理 `\X` -> `X`
            			newstr[i++] = str[++j];  // 跳过 `\` 直接存 X
        		}
    		} else {
        		newstr[i++] = str[j];  // 直接存普通字符
    		}
	}	
	newstr[i] = '\0';
	yylval.cpp_string = new std::string(newstr);
	free(str);
	free(newstr);
	return WORD;
}

source[ \t]+[^ \t\n]+ {
    char *filename = strdup(yytext + 7);
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "source: cannot open %s\n", filename);
        free(filename);
        return 0;
    }
	//fprintf(stderr, ">>> sourcing file: %s\n", filename);
    YY_BUFFER_STATE buf = yy_create_buffer(f, YY_BUF_SIZE); 
    yypush_buffer_state(buf); // 把这个 buffer 放到栈顶, 从现在开始，yylex() 就会从这个 buffer 获取内容（不再是 stdin）
    free(filename);
	yyparse();
}

<<EOF>> {
    yypop_buffer_state();
    if (yy_buffer_stack_top == 0) {
        yyterminate();
    }
	Shell::prompt();
}

[^ \t\n|><][^ \t\n|><]*  {
  /* Assume that file names have only alpha chars */
  //printf("WORD: %s\n", yytext);
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}


