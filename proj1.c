//COP4610
//Project 1
//James Keifer Hudson, Artir Hyseni, and Gustavo Valery


//We started with the parser_help.c code then built on from there

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>																										//for execv() to execute built in commands
#include <sys/stat.h>																									//for stat() to check for valid file paths
#include <fcntl.h>																										//for some of the I/O stuff
#include <sys/wait.h>

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
	int county2 = 0;

	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;

	while (1)
	{
		printf("%s@%s:%s>", getenv("USER"), getenv("MACHINE"), getenv("PWD"));

		// loop reads character sequences separated by whitespace
		do
		{
			//scans for next token and allocates token var to size of scanned token
			scanf("%ms", &token);
			temp = (char*)malloc((strlen(token) + 1) * sizeof(char));

			int i;
			int start = 0;
			for (i = 0; i < strlen(token); i++)
			{
				//pull out special characters and make them into a separate token in the instruction
				if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&')
				{
					if (i-start > 0)
					{
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

			if (start < strlen(token))
			{
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
//--------------------------------- end of parserhelp.c stuff ------------------------------------------------------------------------

		//Some variables that will help later
		int i, j, k;	
		int pipeLocation, pipeLocation2 = 0;																											//for for loops
		int truth, truth2 = 0;																											//for true false stuff
		char *tempy1 = NULL;																								//temporary vars for strings to hold intermitent calculations
		char *tempy2 = NULL;
		char *tempy3 = NULL;
		int outred, inred, pipe = 0;																				//truth values for if there is output redirection, input redirection, and piping for a given command
		int pipeError = 0;

		//SECTION: Converting environmental variables
		for (i = 0; i < instr.numTokens; i++) 															//going through all the separated instructions that were inputted
		{
			for(j = 0; j < strlen(instr.tokens[i]); j++)											//going through each letter of a token individually
			if(instr.tokens[i][j] == '$') 																		//converting environment variables to there actual value
			{
				county = 0;
				k = 0;
				for (k = j + 1; k < strlen(instr.tokens[i]); k++)								//going through and getting the env var name into temp. This process stops at the end of the token or if you hit a / (because this would be in a filepath)
				{
					if (instr.tokens[i][k] == '/')
						break;
					else
						county++;
				}																																//county now stores how many characters the name of the environmental variable is

				if (county > 0)																																									//if someone types "echo $" it should just print "$" so we don;t want to do all this stuff and remove the $
				{
					if (county == 1 || county == 2)																																//This bit here is to avoid seg faults. There is no env var that are 1 or 2 character long so we can just assume that it is an invalid env var and set instr.tokens[i] to blank
					{
						strcpy(instr.tokens[i], "");
						break;
					}
					//if a env var with 3 or more characters was inputted it might be valid so we have to check
					memcpy(instr.tokens[i], &instr.tokens[i][j + 1], strlen(instr.tokens[i]));										//removes the $ from the front of the env var
					temp = (char *) malloc(county + 1);																														//allocates space for temp to hold the name of the env var
					strncpy(temp, instr.tokens[i], county);																												//copies the env var from the beginning of instr.tokens[i] to temp based on the length of the env var that was counted earlier with county
					if(strlen(temp) != strlen(instr.tokens[i]))																										//this part is helpful when distinguishing the difference between just an env var and an env var with stuff attached to the end like in a file path
						memcpy(instr.tokens[i], &instr.tokens[i][j + county], strlen(instr.tokens[i]) - strlen(temp) + 1);

					if(getenv(temp) != NULL)																										//if valid env variable
					{
						tempy1 = (char *) malloc(strlen(getenv(temp)) + strlen(instr.tokens[i]) + 1);								//allocating space in tempy to hold the value of the env var, the stuff connected to it in the input, and the null char at the end
						strcpy(tempy1, getenv(temp));																																//tempy1 now holds the actual value of the env var
						if(strcmp(temp, instr.tokens[i]) != 0)																											//essentially says if there was anything attached after the env var in the input then concatonate it to the end
							strcat(tempy1, instr.tokens[i]);
						instr.tokens[i] = (char *) malloc(strlen(tempy1) + 1);																			//allocating space in intrs.tokens[i] for the final answer
						strcpy(instr.tokens[i], tempy1);
						free(temp);
						temp = NULL;
						free(tempy1);
						tempy1 = NULL;
					}
					else																														//if invalid env variable
							strcpy(instr.tokens[i], "");
				}
			}
		}
		//Done converting environmental variables


		//SECTION: Identifying where all the commands are
		int comspots[instr.numTokens];																														//initializing to this size because there can't be more commands then there are tokens
		int numofcom = 0;																																				//the number of commands, valid or invalid, that the user typed in
		//first token is always going to be a command except for when it is a & where the & will be ignored and the next token is a command that will still be executed in the foreground per the second bullet point of part 9
		for (i = 0; i < instr.numTokens; i++)
		{
			if(i == 0 && (strcmp(instr.tokens[0], "&") != 0)) 																						//if the first token is not a & then it is a commands
			{
				comspots[numofcom] = i;
				numofcom++;
			}
			else if ((strcmp(instr.tokens[i-1], "|") == 0) || (strcmp(instr.tokens[i-1], "&") == 0))
			{
				comspots[numofcom] = i;
				numofcom++;
			}
		}
		//Done identifying where all the commands are


		//SECTION: Identifying where all the file paths are
		//(a file path isn't considered a file path if it is part of an echo command)
		int filespots[instr.numTokens];																													//array that tells what tokens are file paths
		int numfiles = 0;																																				//how many file paths there are
		truth = 0;
		for(i = 0; i < instr.numTokens; i++)
		{
			//Checking for file paths that are just ".", "..", "~"
			if ((strcmp(instr.tokens[i], ".") == 0) || (strcmp(instr.tokens[i], "..") == 0) || (strcmp(instr.tokens[i], "~") == 0)) 			//any of these tokens represents a file path
			{
				for(j = i; j >= 0; j--)
				{
					if(strcmp(instr.tokens[j], "echo") == 0)
					{
						truth = 1;
						break;
					}
					else if ((strcmp(instr.tokens[j], "<") == 0) || (strcmp(instr.tokens[j], ">") == 0) || (strcmp(instr.tokens[j], "|") == 0) || (strcmp(instr.tokens[j], "&") == 0))
						break;
				}
				if (truth == 0)
				{
					filespots[numfiles] = i;
					numfiles++;
				}
				truth = 0;
			}
			//Checking for file paths that are strings that contain a "/"
			for (j = 0; j < strlen(instr.tokens[i]); j++)																						//going through each letter in a token to look for a "/"
			{
				if(instr.tokens[i][j] == '/') 																												//if the token contains a "/" then it is representing a file path
				{
					truth = 0;
					for(k = i; k >= 0; k--)
					{
						if(strcmp(instr.tokens[k], "echo") == 0)
						{
							truth = 1;
							break;
						}
						else if ((strcmp(instr.tokens[i], "<") == 0) || (strcmp(instr.tokens[i], ">") == 0) || (strcmp(instr.tokens[i], "|") == 0) || (strcmp(instr.tokens[i], "&") == 0))
							break;
					}
					if (truth == 0)
					{
						filespots[numfiles] = i;
						numfiles++;
						break;																																					//making sure that if a token has multiple "/" then it doesn't get counted a multiple files
					}
					truth = 0;
				}
			}
		}
		//Done identifying where all the files are

		//Going through and converting file paths that are just "." to  $pwd
		for(i = 0; i < numfiles; i++)
		{
			if(strcmp(instr.tokens[filespots[i]], ".") == 0)
			{
				instr.tokens[filespots[i]] = (char *) malloc(strlen(getenv("PWD")) + 1);
				strcpy(instr.tokens[filespots[i]], getenv("PWD"));
			}
		}


		//SECTION: Converting files to there true paths
		//now that we know where all the file tokens are located we can convert them to the appropriate full paths
		for (i = 0; i < numfiles; i++)
		{
			tempy1 = NULL;
			tempy2 = NULL;
			tempy3 = NULL;
			//Going though the list of files that are located using the filespots[] array
			for (j = 0; j < strlen(instr.tokens[filespots[i]]); j++)															//iterating through each charater in the tokens that were designated as files
			{
				//Converting Tildas
				if (j == 0 && instr.tokens[filespots[i]][j] == '~')																						//just a tilda
				{
					//if just a tilda
					if (strlen(instr.tokens[filespots[i]]) == 1)
						strcpy(instr.tokens[filespots[i]], getenv("HOME"));
					//if tilda at the beginning of a longer file path
					else if(strlen(instr.tokens[filespots[i]]) > 1)
					{
							memcpy(instr.tokens[filespots[i]], &instr.tokens[filespots[i]][1], strlen(instr.tokens[filespots[i]]));				//last parameter is the num of bytes to be copied. normally would have to put a + 1 after strlen to account for null character but we are removing the $ character from the front so it isn't needed here
							tempy1 = (char *) malloc(strlen(getenv("HOME")) + 1);													//+1 required for the null character at the end of a char array. malloc makes space in tempy1 var the size of strlen(getenv("Home"))
							strcpy(tempy1, getenv("HOME"));																							 //copy the env var "HOME" into the space that was just created for it within the tempy1 var
							tempy2 = (char *) malloc(strlen(instr.tokens[filespots[i]]) + 1);
							strcpy(tempy2, instr.tokens[filespots[i]]);
							tempy3 = (char *) malloc(strlen(instr.tokens[filespots[i]]) + strlen(getenv("HOME")) + 1);  //enough space to hold the concatenation of instr.tokens[i] and getenv("HOME")
							strcpy(tempy3, tempy1);																													//concatenation in two steps. first copy tempy1 into the space that was created in tempy3
							strcat(tempy3, tempy2);																															//then concatenate tempy2 on to the end of what is in tempy3
							instr.tokens[filespots[i]] = (char *) malloc(strlen(tempy3) + 1);												//copying the memory in tempy3 into instr.tokens[i]
							strcpy(instr.tokens[filespots[i]], tempy3);
							free(tempy1);																																	//freeing and resetting the variables that we were just using
							free(tempy2);
							free(tempy3);
							tempy1 = NULL;
							tempy2 = NULL;
							tempy3 = NULL;
					}
				} //Done with tildas

				//Converting Relative paths
				if (j == 0 && instr.tokens[filespots[i]][j] != '/')																					//Relative path is like if you type if "cd OS" (assuming OS is a directory in your current directory) it is the same as saying "cd $PWD/OS"
				{
					tempy1 = (char *) malloc(strlen(getenv("PWD")) + 1);
					strcpy(tempy1, getenv("PWD"));
					tempy2 = (char *) malloc(strlen(instr.tokens[filespots[i]]) + 1);
					strcpy(tempy2, instr.tokens[filespots[i]]);
					tempy3 = (char *) malloc(2);																															//tempy3 just holding a "/" to go right before the relative path that the user typed in and the pwd that is being put before it
					strcpy(tempy3,"/");
					instr.tokens[filespots[i]] = NULL;
					instr.tokens[filespots[i]] = (char *) malloc(strlen(tempy1) + strlen(tempy2) + 2);
					strcat(instr.tokens[filespots[i]], tempy1);																								//PWD
					strcat(instr.tokens[filespots[i]], tempy3);																								//"/"
					strcat(instr.tokens[filespots[i]], tempy2);																								//the relative path that was typed in
					free(tempy1);
					free(tempy2);
					tempy1 = NULL;
					tempy2 = NULL;
				}//done with relative paths

				//Converting Double Dots(..)
				truth2 = 0; 																																															//this just stores true false if you called .. on the root directory so that we can know that there is an error and not run the command. This is needed because built in commands like "ls /.." will be accepted as valid and still run even though that our project instructions tell us to make that invalid
				if (instr.tokens[filespots[i]][j] == '.' && j != strlen(instr.tokens[filespots[i]]) - 1 && instr.tokens[filespots[i]][j+1] == '.')					//middle condion to make sure first "." isn't the last character in the files token because otherwise trying to check for a second "." would go out of bounds.
				{
					if (j == 1 && instr.tokens[filespots[i]][j-1] == '/')																							//case when using .. on the root directory like "/.."
					{
						truth2 = 1;
						break;
					}
					else if (j > 1 && instr.tokens[filespots[i]][j-1] != '/')																					//case where .. is not at the beginning of the file and isn't preceded by a / like ~/OS/hat..
						break;
					else if (j != strlen(instr.tokens[filespots[i]]) - 2 && instr.tokens[filespots[i]][j+2] != '/')			//case where the .. isnt at the end of the file like "~/.." and isn't followed by a "/". ex would be "~/OS/..boo/scaredyou"
						break;
					else																																															//valid use of .. here so now we need to do the removing of the directory and stuff
					{
							county = 0;
							//figuring out how much of the middle of the file path we have to remove to turn the file path into its parent
							for (k = j-2; k >= 0; k--)																																		//the j-1 part is to skip the first "/" that comes right before the ".." so that we can look for the previous one.
							{
								if (instr.tokens[filespots[i]][k] == '/')
								{
									county = k;
									break;
								}
							}
							county = county + 1;
							tempy1 = (char *) malloc(county + 1);
							strncpy(tempy1, instr.tokens[filespots[i]], county);

							county = 0;
							if(strlen(instr.tokens[filespots[i]]) > j+3) 																		//if there was stuff after the double dot
							{
								for (k = j + 3; k < strlen(instr.tokens[filespots[i]]); k++)									//counting how much stuff is after the double dot
								{
									county = county + 1;
								}
								tempy2 = (char *) malloc(county + 1);																					//allocating memory for the stuff after the double dot
								tempy2[county] = '\0';
								county = 0;
								for (k = j + 3; k < strlen(instr.tokens[filespots[i]]); k++)									//going through character by character and grabbing the stuff that follows "../"
								{
									tempy2[county] = instr.tokens[filespots[i]][k];
									county = county + 1;
								}
								//tempy2 now holding the stuff that comes after the "../"
								//I tried doing this little chunk straight into the instr.tokens[] but it didn't work for some reason so I decided to use tempy3 as an intermediate between the calculations and instr.tokens[]
								tempy3 = (char *) malloc(strlen(tempy1) + strlen(tempy2) + 1);
								strcpy(tempy3, tempy1);
								strcat(tempy3, tempy2);																												//tempy3 now holds the final answer, just need to put it in instr.tokens[filespots[i]]

								instr.tokens[filespots[i]] = NULL;
								instr.tokens[filespots[i]] = (char *) malloc(strlen(tempy3) + 1);
								strcpy(instr.tokens[filespots[i]], tempy3);
							}
							else																																//else if there is not stuff that comes after the .. (besdies a possible "/")
							{
								instr.tokens[filespots[i]] = NULL;
								instr.tokens[filespots[i]] = (char *) malloc(strlen(tempy1) + 1);						//if nothing after the "../" then the final file is just the stuff before the "../" which is already stored in tempy1
								strcpy(instr.tokens[filespots[i]], tempy1);
							}
							county = 0;
							free(tempy1);
							free(tempy2);
							free(tempy3);
							tempy1 = NULL;
							tempy2 = NULL;
							tempy3 = NULL;
					}
				}//done with double dots

				//Single Dots "."
				//Removing the single dot from the middle of the path.
				//"./blah" gets $PWD appended to the front of it already with the relative path stuff so the "." would be in the middle now.
				//Just need to remove the "/." from "abc/./123"
				if (j < strlen(instr.tokens[filespots[i]]) - 1 && instr.tokens[filespots[i]][j - 1] == '/' && instr.tokens[filespots[i]][j] == '.' && instr.tokens[filespots[i]][j + 1] == '/')
				{
					tempy1 = (char *) malloc(j);
					strncpy(tempy1, instr.tokens[filespots[i]], j -1);
					//tempy1 now holding the stuff before the "/./"

					county = 0;																																		//counting how many things come after the "/." (including the trailing "/")
					for(k = j + 1; k < strlen(instr.tokens[filespots[i]]); k++)
					{
						county = county + 1;
					}
					tempy2 = (char *) malloc(county + 1);																					//allocating memory for the stuff after the double dot
					tempy2[county] = '\0';
					county = 0;
					for(k = j + 1; k < strlen(instr.tokens[filespots[i]]); k++)
					{
						tempy2[county] = instr.tokens[filespots[i]][k];
						county = county + 1;
					}
					tempy3 = (char *) malloc(strlen(tempy1) + strlen(tempy2) + 1);
					strcpy(tempy3, tempy1);
					strcat(tempy3, tempy2);																												//tempy3 now holds the final answer, just need to put it in instr.tokens[filespots[i]]

					instr.tokens[filespots[i]] = NULL;
					instr.tokens[filespots[i]] = (char *) malloc(strlen(tempy3) + 1);
					strcpy(instr.tokens[filespots[i]], tempy3);

					free(tempy1);
					free(tempy2);
					free(tempy3);
					tempy1 = NULL;
					tempy2 = NULL;
					tempy3 = NULL;

				}//done with single dot from the middle "abc/./123"
				//Removing the single dot from the end of the path
				if(j == strlen(instr.tokens[filespots[i]]) - 1  && instr.tokens[filespots[i]][j] == '.')
				{
					tempy1 = (char *) malloc(strlen(instr.tokens[filespots[i]]));
					strncpy(tempy1, instr.tokens[filespots[i]], strlen(instr.tokens[filespots[i]]) - 1);
					instr.tokens[filespots[i]] = (char *) malloc(strlen(tempy1) + 1);
					strcpy(instr.tokens[filespots[i]], tempy1);
					free(tempy1);
					tempy1 = NULL;
				}//done with single dot at the very end "abc/."

			}//done with a single file
	 	}//done iterating through each file
		//Done converting file paths



		//SECTION: checking if file paths are all valid
		//use the stat() function on each file in filespots[] to see if the file paths are valid or not. Through an error if it isn't and exit(1)
		struct stat stats;																																			//weird struct that the stat function uses to display different file properties. It is built into #include <sys/stat.h>
		truth = 1;																																							//true false on whether or not all the files are valid. Assume they are true until you encounter a bad one
		for(i = 0; i < numfiles; i++)
		{
			if(stat(instr.tokens[filespots[i]], &stats) != 0)																			//stat() returns 0 if a valid file path was given and a nonzero number if it was invalid
			{
				//It's not an invalid path if it comes right after a ">" because the file will be created during the output redirection process
				if (i != 0 && strcmp(instr.tokens[filespots[i] - 1], ">") != 0)
				{
					printf("Invalid file path: %s\n", instr.tokens[filespots[i]]);
					truth = 0;
					addNull(&instr);																																		//have to do this clearing stuff before asking the user for another command because otherwise commands from previous prompts get jumbled in
					clearInstruction(&instr);
					break;
				}
			}
		}
		if (truth == 0)																																					//If you found an invalid file path then continue on to the next loop of the big while loop, prompting the user to type in another command
			continue;


		//SECTION: checking for some errors *** need to add more error checking for some of the other weird combinations of symbols ***
		//Lat token can't be a input or output redirection
		if(!strcmp(instr.tokens[instr.numTokens - 1], "<") || !strcmp(instr.tokens[instr.numTokens - 1], ">"))
		{
			printf("bash: syntax error near unexpected token newline\n");																																		//error message and clearing instructions since invalid stuff was inputted
			clearInstruction(&instr);
			continue;
		}
		//if you called .. on the root directory
		if (truth2 == 1)
		{
			printf("Error: can't use \"..\" path on the root directory\n");
			clearInstruction(&instr);
			continue;
		}
		truth2 = 0;
		//if "<" is the first token
		if (strcmp(instr.tokens[0], "<") == 0)
		{
			printf("bash: syntax error\n");
			clearInstruction(&instr);
			continue;
		}

		if (!strcmp(instr.tokens[0], "|"))
		{
			printf("bash: syntax error near unexpected token '|'\n");
			pipeError = 1;
		}



		//SECTION: Built in commands (part 6 stuff)
		//checking to make sure the arguement isn't any of the built in commands that we have to build ourselves for part 10
		if(strcmp(instr.tokens[0], "exit") != 0 && strcmp(instr.tokens[0], "cd") != 0 && strcmp(instr.tokens[0], "echo") != 0 && strcmp(instr.tokens[0], "alias") != 0 && strcmp(instr.tokens[0], "unalias"))
		{
			temp = (char *) malloc(strlen(instr.tokens[0]) + 6);																				//+6 because of the normal + 1 + the characters in "/bin/"
			strcpy(temp, "/bin/");																																			//***** part 5 stuff ****************
			strcat(temp, instr.tokens[0]);																															//concatenating the beginning of the file path and the executable's name to be a path to pass to the execv func
			//counting how many parameters there are for the command
			county = 0;
			for(i = 1; i < instr.numTokens; i++)
			{
				if (i == instr.numTokens)																																	//case to handle if only a command was entered with no parameters (like "ls")
					break;
				else if ((strcmp(instr.tokens[i], "<") == 0) || (strcmp(instr.tokens[i], ">") == 0) || (strcmp(instr.tokens[i], "|") == 0) || (strcmp(instr.tokens[i], "&") == 0))
					break;
				else
					county++;
			}

			//creating parameter list to be passed to execv() for the built in commands
			int parmlistlen = county + 2;
			char* parmlist[county + 2]; 																																//size of parm list is county + 2. county for each parameter, 1 for the command name, and one for the NULL at the end of the list
			parmlist[0] = temp;
			for(i = 1; i < county + 1; i++) 																														//adding all the parameters to the parmlist
				parmlist[i] = instr.tokens[i];
			parmlist[county + 1] = NULL;


			//Checking what sort of Redirections there are to do
			for(i = 1; i < instr.numTokens; i++)
			{
				if( i == instr.numTokens)
					break;
				if(strcmp(instr.tokens[i], "|") == 0)
				{
					pipe = 1;
					pipeLocation = i;

					if( i == instr.numTokens-1) {
						pipeError = 1;
						printf("bash: syntax error near unexpected token '|'\n");
						break;
					}
					tempy1 = (char *) malloc(strlen(instr.tokens[i+1]) + 6);
					strcpy(tempy1, "/bin/");	
					strcat(tempy1, instr.tokens[i+1]);

					county = 0;
					for(j = i+2; j < instr.numTokens; j++) {
						if( j == instr.numTokens)
							break;
						else if(strcmp(instr.tokens[j], "|") == 0) {
							pipe = 2;
							pipeLocation2 = j;

							if( j == instr.numTokens-1) {
								pipeError = 1;
								printf("bash: syntax error near unexpected token '|'\n");
								break;
							}
							tempy2 = (char *) malloc(strlen(instr.tokens[j+1]) + 6);
							strcpy(tempy2, "/bin/");	
							strcat(tempy2, instr.tokens[j+1]);

							county2 = 0;
							for(k = j+2; k < instr.numTokens; k++) {
								if(k == instr.numTokens)
									break;
								else
									county2++;
							}
							break;	
						}
						else
							county++;
					}
					break;
				}
				else if(strcmp(instr.tokens[i], "<") == 0)
					inred = 1;
				else if(strcmp(instr.tokens[i], ">") == 0)
					outred = 1;
			}
			int parmlist2len = county + 2;
			char* parmlist2[county + 2];

			int parmlist3len = county2 + 2;
			char* parmlist3[county2 + 2];

			if(pipe == 1 || pipe == 2 && pipeError == 0) {
				parmlist2[0] = tempy1;
				for(i = 1; i < county + 1; i++) {	
					if(instr.tokens[i+pipeLocation+1] == "|")
						break;
					parmlist2[i] = instr.tokens[i+pipeLocation+1];
				}
				parmlist2[county + 1] = NULL;
				if(pipe == 2) {
					parmlist3[0] = tempy2;
					for(i = 1; i < county2 + 1; i++) 																														//adding all the parameters to the parmlist
							parmlist3[i] = instr.tokens[i+pipeLocation2+1];
					parmlist3[county2 + 1] = NULL;
				}
			}


			//This part works without any I/O Redirection or Piping so I'm not touching it
			if (outred == 0 && inred == 0 && pipe == 0 && pipeError == 0)
			{
				int status;																																									//this is for the waitpid function. status is 0 if there is no error.
				pid_t pid = fork();
				if (pid == -1)																																		//this code was making the prompt print out twice for some reason so i just commented it out for now
								printf("Forking Error");
				else if(pid == 0) 																																					//pid of 0 means the child process was successfully created
				{
					execv(temp, parmlist);																																		//this is what executes all the prebuilt commands. temp is the command name (at the end of the path) and parmlist is all the parameters for that command
					printf("execv failed\n");																															//if an invalid command was inputted then execv returns and hits this line to display an error code
					exit(1);
				}
				else
					waitpid(pid, &status, 0);																															//this else is for the parent to hit. The parent hits this while the child hits the else if. Parent waits for child to finish
			}

			//Just Output Redirection
			else if(outred == 1 && inred == 0 && pipe == 0 && pipeError == 0)
			{
				int status;
				pid_t pid = fork();
				//need to get tempy to equal the file that is being outputted to
				for(i = 0; i < instr.numTokens; i++)
				{
						if(strcmp(instr.tokens[i], ">") == 0)
						{
							tempy1 = (char *) malloc(strlen(instr.tokens[i + 1]) + 1);
							strcpy(tempy1, instr.tokens[i + 1]);
							break;
						}
				}
				if (pid == -1)																																		//this code was making the prompt print out twice for some reason so i just commented it out for now
			          printf("Forking Error");
			  else if(pid == 0) 																																					//pid of 0 means the child process was successfully created
			  {
			    open(tempy1, O_RDWR | O_CREAT | O_TRUNC);
			    close(1);
			    dup(3);
			    close(3);
			    execv(temp, parmlist);
			    printf("exec failed\n");
			  }
			  else
			  	waitpid(pid, &status, 0);

				free(tempy1);
				tempy1 = NULL;
			}

			//Just Input Redirection
			else if(outred == 0 && inred == 1 && pipe == 0 && pipeError == 0)
			{
				printf("TEST: Just Input redirection\n");
				int status;
				pid_t pid = fork();
				//need to get tempy to equal the file that the input is coming from
				for(i = 0; i < instr.numTokens; i++)
				{
						if(strcmp(instr.tokens[i], "<") == 0)
						{
							tempy1 = (char *) malloc(strlen(instr.tokens[i + 1]) + 1);
							strcpy(tempy1, instr.tokens[i + 1]);
							break;
						}
				}
				//tempy1 is the input file now


				if (pid == -1)																																		//this code was making the prompt print out twice for some reason so i just commented it out for now
								printf("Forking Error");
				else if(pid == 0) 																																					//pid of 0 means the child process was successfully created
				{
					open(tempy1, O_RDONLY);
					close(0);
					dup(3);
					close(3);
					execv(temp, parmlist);
					printf("exec failed\n");
				}
				else
					waitpid(pid, &status, 0);

				free(tempy1);
				tempy1 = NULL;
			}//end of input redirection

//WORKING HERE-------------------------------------------------
			//Both input and output redirection
			else if(outred == 1 && inred == 1 && pipe == 0 && pipeError == 0)
			{
				printf("TEST: Godzirraaa says there is both input and output redirection mofo\n");
				//tempy1 will hold the input file and tempy2 will hold the output file
				//finding the input file
				for(i = 0; i < instr.numTokens; i++)
				{
					if(strcmp(instr.tokens[i], "<") == 0)
					{
						county = i + 1;
						break;
					}
				}
				tempy1 = (char *) malloc(strlen(instr.tokens[county]));
				strcpy(tempy1, instr.tokens[county]);
				//finding the output file
				for(i = 0; i < instr.numTokens; i++)
				{
					if(strcmp(instr.tokens[i], ">") == 0)
					{
						county = i + 1;
						break;
					}
				}
				tempy2 = (char *) malloc(strlen(instr.tokens[county]));
				strcpy(tempy2, instr.tokens[county]);

				//input and output file now both identified
				int status;
				pid_t pid = fork();
				//setting input and output redirections using the same process that was used for them each individually
				if (pid == -1)
								printf("Forking Error");
				else if(pid == 0)
				{
					//setting input
					open(tempy1, O_RDONLY);
					close(0);
					dup(3);
					close(3);
					//setting output
					open(tempy1, O_RDWR | O_CREAT | O_TRUNC);
					close(1);
					dup(3);
					close(3);
					//executing
					execv(temp, parmlist);
					printf("exec failed\n");
				}
				else
					waitpid(pid, &status, 0);

				free(tempy1);
				tempy1 = NULL;
				free(tempy2);
				tempy2 = NULL;
			}//done with both input and output redirection

//WORKING ABOVE------------------------------------------------------

			free(temp);
			temp = NULL;
			outred = 0;
			inred = 0;
			pipe = 0;
			pipeError = 0;
		}//End of Built in Functions



		//**** Should probably turn these into functions that can be called in other places to make it easier for I/O redirections and piping ***
		//SECTION: Own commands (part 10)
		for (i = 0; i < instr.numTokens; i++)
		{
			//"echo" command
			//*******echo is one of the built in commands in part 10**** this works but later we should just use the built in since we know that works completely correctly for every single test case
			if(!strcmp(instr.tokens[i], "echo")) 													//***the strcmp function returns 0 if the two strings are equal and a nonzero number if they aren't equal
			{
				int j;
				for (j = i+1; j < instr.numTokens; j++)
				{
					if (!strcmp(instr.tokens[j], "|") || !strcmp(instr.tokens[j], "<") || !strcmp(instr.tokens[j], ">") || !strcmp(instr.tokens[j], "&"))					//echo command shouldn't print these 4 special characters
						break;
					else if (instr.tokens[j] != NULL)														//if you don't hit a special character keep printing instructions until the end
						printf("%s ", instr.tokens[j]);
				}
				printf("\n");
				break;
			}
		}


		//SECTION: ending stuff from parserhelp.c
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
