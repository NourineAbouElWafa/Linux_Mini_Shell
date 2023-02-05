/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "command.h"
#include <iostream>
#include <fstream>
#include <glob.h>
#include <time.h>
#define DTTMFMT "%Y-%m-%d %H:%M:%S "
#define DTTMSZ 21
using namespace std;

//function to get current time
static char *getDtTm (char *buff) {
    time_t t = time (0);
    strftime (buff, DTTMSZ, DTTMFMT, localtime (&t));
    return buff;
}
//function that writes in log file
void log_file(int signal_num){
  char buff[DTTMSZ];
  ofstream LogFile;
  LogFile.open("logs.txt",ios::app);
  // Write to the file
  LogFile << getDtTm(buff) << "child terminated\n" ;

  // Close the file
  LogFile.close();
  return;
}


SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;

}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile || _errFile ) {
		free( _outFile );
		// printf("out cleared");
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	// if ( _errFile ) {
	// 	free( _errFile );
	// }

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_appendFlag=0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("\n");
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t ", _simpleCommands[i]->_arguments[ j ] );
			
		}
	}
	

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background     append\n" );
	printf( "  ------------ ------------ ------------ ------------   ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s  %-12s \n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO",_appendFlag?"  YES":"  NO");

	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
	

	// Print contents of Command data structure
	print();

	// Add execution here
	// saving user default input in case no input file in the command
	int defaultin = dup( 0 ); 
	// saving user default output in case no output file in the command
	int defaultout = dup( 1 ); 

	int defaulterr = dup( 2 );
	// saving user default error in case no error file in the command
	int fin;
	int fout;
	int errout;
     // in case of finding an input file in the coomand 
	if(_inputFile){
		// reading from input file and setting initial input
		 fin = open(_inputFile, O_RDONLY);
		if ( fin < 0 ) {
			perror( "ls : create inputfile" );
			exit( 2 );
		}
		//input file redirection
		dup2( fin, 0 );
		//closing the input file
		close( fin );
	}
	else{
		//using default input
		fin=dup(defaultin);
		//closing the default input file
		close(fin);
	}


	
	int i ;
	int pid ;
    int fdpipe[2];	
	for (  i = 0 ; i< _numberOfSimpleCommands; i++ ) {
		//input redirection
		dup2(fin, 0);
		close(fin);
		//if it is the last command redirect output
		if (i == _numberOfSimpleCommands-1){
			// checking if we should append to the output file
			if(_outFile && _appendFlag){
		 		fout = open( _outFile, O_WRONLY|O_APPEND|O_CREAT,0666);
				if ( fout < 0 ) {
					perror( "ls : create outfile" );
					exit( 2 );
				}
				
			}
			// checking if we should overwrite the output file
			else if (_outFile && !_appendFlag){
				fout = creat( _outFile, 0666 );
				
			}
			else{
				// using default output
				fout=dup( defaultout);
				
			}
			
			// checking if we should overwrite the error file
			if(_errFile && !_appendFlag){
		 		errout = creat(_errFile, 0666 );
				if ( errout < 0 ) {
					perror( "ls : create errorfile" );
					exit( 2 );
				}
				
			}
			// checking if we should append to the error file
			else if (_errFile && _appendFlag){
				errout=open(_errFile,O_WRONLY|O_APPEND|O_CREAT,0666);
				
				
			}
			else{
				//using default error
				errout=dup( defaulterr);
				
			}
			//error redirection
			 dup2( errout, 2 );
			close( errout );
		}
		// creating pipe since it's not the last simple command
		else{
			pipe(fdpipe); 
			fout = fdpipe[1]; 
			fin = fdpipe[0]; 
		}
		//cd command
		if(!strcmp(_simpleCommands[i]->_arguments[0],"cd")){
			// changing the directory to home whenever the command cd has no arguments
			if(_simpleCommands[i]->_arguments[1]== NULL){
				chdir(getenv("HOME"));
			}
			// normal cd 
			chdir(_simpleCommands[i]->_arguments[1]);
		}
	     //output redirection
		dup2(fout,1);
		close(fout);
		//creating a log file whenever a child process is terminated
		signal(SIGCHLD,log_file);
		//forking a new process for every simple command a new process
		pid=fork();
		if ( pid < 0 ) {
		perror( "ls: fork\n");
		exit( 2 );
	   }


	if (pid == 0) { 
		close( defaultin );
		close( defaultout );
		close( defaulterr );
		//calling exec
		execvp(_simpleCommands[i]->_arguments[0],_simpleCommands[i]->_arguments);
		exit(2);
	}


	}

    //restore default input output error files to the default
	
	dup2( defaultin, 0 );
	dup2( defaultout, 1 );
	dup2( defaulterr, 2 );
	close( defaultin );
	close( defaultout );
	close( defaulterr );
	//checking if program should run in the background
	if(!_background){
	waitpid( pid, 0, 0 );}

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{   printf("\033[1;37m");
	char tmp[256];
    getcwd(tmp, 256);
	printf("\033[1;34m~%s$",tmp);
	printf("\033[1;37m");
	fflush(stdout);
	
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);
void handleControl( int signal_num){
	printf("\n");
	Command::_currentCommand.prompt();
	
}

int 
main()
{
	signal(SIGINT,handleControl);
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}