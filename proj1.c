//COP4610
//Project 1
//James Hudson, Artir Hyseni, and Gustavo Valery


//We started with the parser_help.c code then built on from there

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
				if(getenv(instr.tokens[i]) != NULL)															//if valid env variable
					strcpy(instr.tokens[i], getenv(instr.tokens[i]));
				else																														//if invalid env variable
					strcpy(instr.tokens[i], "");
			}
		}

		//Part 4 checking for file paths
		// for (i = 0; i < instr.numTokens; i++) {	
		// 	int j;	
		// 	// For every character in token
		// 	for (j = 0; i < strlen(instr.tokens[i]); i++) {
		// 		printf("j: %d\n", j);
		// 		if(j == 0) {
		// 			if(instr.tokens[i][1] == '~') {
		// 				memcpy(instr.tokens[i], &instr.tokens[i][1], strlen(instr.tokens[i]));
		// 				strcpy(instr.tokens[i], strcat(getenv("HOME"), temp));
		// 				printf("Tilda: %s\n", instr.tokens[i]);
		// 			}
		// 		}

		// 		// If there is a '/' then we are dealing with a path
		// 		if(instr.tokens[i][j] == '/') {
		// 		}
		// 	}
		// }

		//checking for some errors
		if(!strcmp(instr.tokens[instr.numTokens - 1], "<") || !strcmp(instr.tokens[instr.numTokens - 1], ">")) {
			printf("bash: syntax error near unexpected token newline\n");																																		//error message and clearing instructions since invalid stuff was inputted
			clearInstruction(&instr);
		}


		//going through and executing all commands
		for (i = 0; i < instr.numTokens; i++) {
			//"echo" command
			//*******echo is one of th ebuilt in commands in part 10**** this works but later we should just use the built in since we know that works completely correctly for every single test case
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
