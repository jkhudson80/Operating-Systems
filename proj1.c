//COP4610
//Project 1
//James Hudson, Artir Hyseni, and Gustavo Valery


//We started with the parser_help.c code then built on from there

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>																										//for execv

typedef struct
{
	char** tokens;
	int numTokens;
} instruction;

void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);


int main() {
	char* token = NULL;
	char* temp = NULL;
	int county = 0;

	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;

	while (1) {
		printf("%s@%s:%s>", getenv("USER"), getenv("MACHINE"), getenv("PWD"));

		// loop reads character sequences separated by whitespace
		do {
			//scans for next token and allocates token var to size of scanned token
			scanf("%ms", &token);
			temp = (char*)malloc((strlen(token) + 1) * sizeof(char));

			int i;
			int start = 0;
			for (i = 0; i < strlen(token); i++) {
				//pull out special characters and make them into a separate token in the instruction
				if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {
					if (i-start > 0) {
						memcpy(temp, token + start, i - start);
						temp[i-start] = '\0';
						addToken(&instr, temp);
					}

					char specialChar[2];
					specialChar[0] = token[i];
					specialChar[1] = '\0';

					addToken(&instr,specialChar);

					start = i + 1;
				}
			}

			if (start < strlen(token)) {																				//******need to ask about in office hours
				memcpy(temp, token + start, strlen(token) - start);
				temp[i-start] = '\0';
				addToken(&instr, temp);
			}

			//free and reset variables
			free(token);
			free(temp);

			token = NULL;
			temp = NULL;
		} while ('\n' != getchar());    //until end of line is reached


		int i;
		for (i = 0; i < instr.numTokens; i++) {															//going through all the separated instructions that were inputted
			if(instr.tokens[i][0] == '$') {																	//converting environment variables to there actual value
				memcpy(instr.tokens[i], &instr.tokens[i][1], strlen(instr.tokens[i]));
				if(getenv(instr.tokens[i]) != NULL)	{														//if valid env variable
					temp = (char *) malloc(strlen(getenv(instr.tokens[i])) + 1);
					strcpy(temp, getenv(instr.tokens[i]));
					instr.tokens[i] = (char *) malloc(strlen(temp) + 1);
					strcpy(instr.tokens[i], temp);
					free(temp);
					temp = NULL;
				}
				else																														//if invalid env variable
					strcpy(instr.tokens[i], "");
			}
		}

		//Part 4 checking for file paths
		 for (i = 0; i < instr.numTokens; i++) {
		 	int j;
			char *tempy1 = NULL;
			char *tempy2 = NULL;
			char *tempy3 = NULL;

		 	// For every character in token
		 	for (j = 0; j < strlen(instr.tokens[i]); j++) {
	 			if(j == 0) {
	 				if(instr.tokens[i][0] == '~') {
						if (strlen(instr.tokens[i]) == 1)
	 						strcpy(instr.tokens[i], getenv("HOME"));
					else if(strlen(instr.tokens[i]) > 1) {
							if(instr.tokens[i][1] == '/') {
								memcpy(instr.tokens[i], &instr.tokens[i][1], strlen(instr.tokens[i]));				//last parameter is the num of bytes to be copied. normally would have to put a + 1 after strlen to account for null character but we are removing the $ character from the front so it isn't needed here
								tempy1 = (char *) malloc(strlen(getenv("HOME")) + 1);													//+1 required for the null character at the end of a char array. malloc makes space in tempy1 var the size of strlen(getenv("Home"))
								strcpy(tempy1, getenv("HOME"));																							 //copy the env var "HOME" into the space that was just created for it within the tempy1 var
								tempy2 = (char *) malloc(strlen(instr.tokens[i]) + 1);
								strcpy(tempy2, instr.tokens[i]);
								tempy3 = (char *) malloc(strlen(instr.tokens[i]) + strlen(getenv("HOME")) + 1);  //enough space to hold the concatenation of instr.tokens[i] and getenv("HOME")
								strcpy(tempy3, tempy1);																													//concatenation in two steps. first copy tempy1 into the space that was created in tempy3
								strcat(tempy3, tempy2);																															//then concatenate tempy2 on to the end of what is in tempy3
								instr.tokens[i] = (char *) malloc(strlen(tempy3) + 1);												//copying the memory in tempy3 into instr.tokens[i]
								strcpy(instr.tokens[i], tempy3);

								free(tempy1);																																	//freeing and resetting the variables that we were just using
								free(tempy2);
								free(tempy3);
								tempy1 = NULL;
								tempy2 = NULL;
								tempy3 = NULL;
							}
						}
	 				}
	 			}
	 		}
	 	}


		//checking for some errors
		if(!strcmp(instr.tokens[instr.numTokens - 1], "<") || !strcmp(instr.tokens[instr.numTokens - 1], ">")) {
			printf("bash: syntax error near unexpected token newline\n");																																		//error message and clearing instructions since invalid stuff was inputted
			clearInstruction(&instr);
		}

//---------------------------------------------------------------------------working on
		//identifying where all the commands are
		int comspots[instr.numTokens];																														//initializing to this size because there can't be more commands then there are tokens
		int numofcom = 0;																																				//the number of commands, valid or invalid, that the user typed in
		//first token is always going to be a command except for when it is a & where the & will be ignored and the next token is a command that will still be executed in the foreground per the second bullet point of part 9
		for (i = 0; i < instr.numTokens; i++) {
			if(i == 0 && (strcmp(instr.tokens[0], "&") != 0)) {																						//if the first token is not a & then it is a commands
				comspots[numofcom] = i;
				numofcom++;
			}
			else if ((strcmp(instr.tokens[i-1], "|") == 0) || (strcmp(instr.tokens[i-1], "&") == 0)) {
				comspots[numofcom] = i;
				numofcom++;
			}
		}
		//now we know what spots contain commands

		//checking to make sure the arguement isn't any of the built in commands that we have to build ourselves for part 10
		if(strcmp(instr.tokens[0], "exit") != 0 && strcmp(instr.tokens[0], "cd") != 0 && strcmp(instr.tokens[0], "echo") != 0 && strcmp(instr.tokens[0], "alias") != 0 && strcmp(instr.tokens[0], "unalias")) {
			temp = (char *) malloc(strlen(instr.tokens[0]) + 6);																				//+6 because of the normal + 1 + the characters in "/bin/"
			strcpy(temp, "/bin/");
			strcat(temp, instr.tokens[0]);																															//concatenating the beginning of the file path and the executable's name to be a path to pass to the execv func
			//counting how many parameters there are for the command
			county = 0;
			for(i = 1; i < instr.numTokens; i++) {
				if (i == instr.numTokens)																																	//case to handle if only a command was entered with no parameters (like "ls")
					break;
				else if ((strcmp(instr.tokens[i], "<") == 0) || (strcmp(instr.tokens[i], ">") == 0) || (strcmp(instr.tokens[i], "|") == 0) || (strcmp(instr.tokens[i], "&") == 0))
					break;
				else
					county++;
			}

			//creating parameter list to be passed to execv()
			char* parmlist[county + 2]; 																																//size of parm list is county + 2. county for each parameter, 1 for the command name, and one for the NULL at the end of the list
			parmlist[0] = temp;
			for(i = 1; i < county + 1; i++) {																														//adding all the parameters to the parmlist
					parmlist[i] = instr.tokens[i];
			}
			parmlist[county + 1] = NULL;

			pid_t pid = fork();
			if(pid == 0) {																																							//pid of 0 means this fork is the child i believe.
				execv(temp, parmlist);																																		//this is what executes all the prebuilt commands. temp is the command name (at the end of the path) and parmlist is all the parameters for that command
				printf("bash: %s: command not found\n", instr.tokens[0]);																	//if an invalid command was inputted then execv returns and hits this line to display an error code
				exit(1);
			}
//			else if ((pid = fork()) == -1)																														//this code was making the prompt print out twice for some reason so i just commented it out for now
//				printf("Forking Error");																																//*** need to add this back in some capacity
		}

//-----------------------------------------------------------------------------

		//going through and executing all commands
		for (i = 0; i < instr.numTokens; i++) {
			//"echo" command
			//*******echo is one of the built in commands in part 10**** this works but later we should just use the built in since we know that works completely correctly for every single test case
			if(!strcmp(instr.tokens[i], "echo")) {													//***the strcmp function returns 0 if the two strings are equal and a nonzero number if they aren't equal
				int j;
				for (j = i+1; j < instr.numTokens; j++) {
					if (!strcmp(instr.tokens[j], "|") || !strcmp(instr.tokens[j], "<") || !strcmp(instr.tokens[j], ">") || !strcmp(instr.tokens[j], "&"))					//echo command shouldn't print these 4 special characters
						break;
					else if (instr.tokens[j] != NULL)														//if you don't hit a special character keep printing instructions until the end
						printf("%s ", instr.tokens[j]);
				}
				printf("\n");
				break;
			}
		}


		addNull(&instr);
		//printTokens(&instr);
		clearInstruction(&instr);
	}

	return 0;
}


//reallocates instruction array to hold another token
//allocates for new token within instruction array
void addToken(instruction* instr_ptr, char* tok)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**) malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**) realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	//allocate char array for new token in new slot
	instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
	strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

	instr_ptr->numTokens++;
}

void addNull(instruction* instr_ptr)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;
	instr_ptr->numTokens++;
}

//int starter added to indicate where in list of instructions you want to start printing from
void printTokens(instruction* instr_ptr)
{
	int i;
	printf("Tokens:\n");
	for (i = 0; i < instr_ptr->numTokens; i++) {
		if ((instr_ptr->tokens)[i] != NULL)
			printf("%s\n", (instr_ptr->tokens)[i]);
	}
}

void clearInstruction(instruction* instr_ptr)
{
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++)
		free(instr_ptr->tokens[i]);

	free(instr_ptr->tokens);

	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
}
