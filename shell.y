
/*
 * CS-413 Spring 98
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE LESS PIPE GREATGREAT GREATERROR GREATGREATERROR ERROR EXIT STAR

%union	{
		char   *string_val;
	}

%{
extern "C" 
{
	int yylex();
	char *string;
	void yyerror (char const *s);
}
#define yylex yylex
#include <stdio.h>
#include <cstring>
#include <glob.h>
#include "command.h"
%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: simple_command
        ;

simple_command:	
	pipe_array iomodifier_opt iomodifier_inpt background NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	|pipe_array iomodifier_inpt iomodifier_opt background NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	| error NEWLINE { yyerrok; }
	;
pipe_array:
	pipe_array PIPE command_and_args
	|command_and_args
	;
command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {char * in = (char *) malloc(sizeof($1)* sizeof(char));
	in=$1;
			if(strstr(in,"*")!= NULL ||strstr(in,"?")!= NULL  ){
			glob_t output;
			if(glob(in,GLOB_ERR,NULL,&output) !=0 ){
				perror("Error in wildcarding");
				Command::_currentCommand.prompt();
			}
			for(size_t k = 0; k < output.gl_pathc;k++){
				 printf("   Yacc: insert argument \"%s\"\n", output.gl_pathv[k]);
				Command::_currentSimpleCommand->insertArgument(output.gl_pathv[k]);
			}
		}
		else{
		
		
            printf("   Yacc: insert argument \"%s\"\n", $1);

	       Command::_currentSimpleCommand->insertArgument( $1 );
		   }
		   
	}
	
	
	;

command_word:
	WORD {
		string = $1;
		if(strcmp(string, "exit") == 0){
			printf("GOOD BYE !! \n");
			exit(0);
		}
               printf("   Yacc: insert command \"%s\"\n", $1);
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_opt:
	GREAT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._appendFlag=0;
		Command::_currentCommand._outFile = $2;
	}
	|GREATERROR WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._appendFlag=0;
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
	}
	| GREATGREAT WORD{
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._appendFlag=1;
		Command::_currentCommand._outFile = $2;

	}
	| GREATGREATERROR WORD{
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._appendFlag=1;
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
	}
	

	| /* can be empty */ 
	;

iomodifier_inpt:
	LESS WORD {
		printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
	}
	| /* can be empty */ 
	;
background:
	ERROR {
		printf("   Yacc: background ");
		Command::_currentCommand._background = 1;
	}
	| /* can be empty */ 
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
