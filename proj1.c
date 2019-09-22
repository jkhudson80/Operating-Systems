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

typedef struct
{
	char* command;
	char* alias;
	int commandwords;
} com_alias;


void echo(char** parmlist, int len, char* output);
void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);


int main() {
	char* token = NULL;
	char* temp = NULL;
	int county = 0;
	int county2 = 0;
	int numofExecCommands = 0;

	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;

	com_alias aliaslist[10];
	int numalias = 0;
	int i;
	for(i = 0; i < 10; i++)
	{
			aliaslist[i].command = NULL;
			aliaslist[i].alias = NULL;
			aliaslist[i].commandwords = 0;
	}

	while (1)
	{
		printf("%s@%s:%s>", getenv("USER"), getenv("MACHINE"), getenv("PWD"));

		clearInstruction(&instr);
		// loop reads character sequences separated by whitespace
		do
		{
			//scans for next token and allocates token var to size of scanned token
			scanf("%ms", &token);

			temp = (char*)malloc((strlen(token) + 1) * sizeof(char));

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
		int j, k;
		int pipeLocation, pipeLocation2 = 0;																											//for for loops
		int truth, truth2 = 0;																											//for true false stuff
		char *tempy1 = NULL;																								//temporary vars for strings to hold intermitent calculations
		char *tempy2 = NULL;
		char *tempy3 = NULL;
		int outred, inred, pipey = 0;																				//truth values for if there is output redirection, input redirection, and piping for a given command
		int pipeError = 0;


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
			else if (i != 0 && ((strcmp(instr.tokens[i-1], "|") == 0) || (strcmp(instr.tokens[i-1], "&") == 0)))
			{
				comspots[numofcom] = i;
				numofcom++;
			}
		}
		//Done identifying where all the commands are


		//converting any alias to there commands
		//the hard part about this is if you're turning a one word thing like "my_ls" to something like "ls -l"
		//because then there are more tokens and you'd have to increment instr.numTokens and shift everything so that
		//parmlist doesn't get affected and you can still iterate through the tokens correctly.
		//1 word to 1 word alias conversions should be easy though.
		for(i = 0; i < numofcom; i++)
		{
			if(strcmp(instr.tokens[comspots[i]], "alias") == 0 || strcmp(instr.tokens[comspots[i]], "unalias") == 0)
				break;
			for(j = 0; j < numalias; j++)
			{
				if(strcmp(instr.tokens[comspots[i]], aliaslist[j].alias) == 0)
				{
					county2 = comspots[i];
					//if the alias to command conversion is 1 word to 1 word
					if(aliaslist[j].commandwords == 1)
					{
						instr.tokens[comspots[i]] = (char *) malloc(strlen(aliaslist[j].command) + 1);
						strcpy(instr.tokens[comspots[i]], aliaslist[j].command);
					}

					//if the command that the alias refers to is more than one word
/*					else
					{
						char ** tempychar;
						tempychar = (char **) malloc(aliaslist[j].commandwords + 1);
						int index = 0;

						county = 0;
						for(k = index; k < strlen(aliaslist[j].command); k++)
						{
							if(aliaslist[j].command[k] != ' ') {
								county++;
							}
							else {
								tempy1 = (char *) malloc(county);
								memcpy(tempy1, &aliaslist[j].command[index], county);
								printf("TEST: tempy1 is |%s|\n", tempy1);
								index = index + county;
								county = 0;
								continue;
							}
						}


						//separating aliaslist[j].command into individual strings
						county = 0;
						tempy1 = NULL;
						for(k = 0; k < strlen(aliaslist[j].command); k++)
						{
							if(aliaslist[j].command[k] == ' ')
							{
								tempy1 = (char *) malloc(county + 1);
								break;
							}
							else
								county++;
						}
						printf("county is %d\n", county);
						for(k = 0; k < county; k++)
						{
							tempy1[k] = aliaslist[j].command[k];
						}
						printf("temyp1 190 is |%s|\n", tempy1);

						instr.tokens[county2] = (char *) malloc(strlen(tempy1));
						strcpy(instr.tokens[county2], tempy1);
						printf("thingy is %s\n", instr.tokens[county2]);

						free(tempy1);
						tempy1 = NULL;
					}*/
				}
			}
		}



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

		int backgroundFlag = 0;
		// Ignore if & is first token
		if( !(strcmp(instr.tokens[0], "&")) ) {
			for(i = 0; i < instr.numTokens; i++) {
				instr.tokens[i] = (char *) malloc(strlen(instr.tokens[i+1]) + 1);
				strcpy(instr.tokens[i], instr.tokens[i+1]);
				instr.numTokens--;
			}
		}
		// checks to make sure & is last
		int amberNotLast = 0;
		for(i = 0; i < instr.numTokens; i++) {
			if( !(strcmp(instr.tokens[i], "&")) ) {
				if( i != instr.numTokens-1) {
					amberNotLast = 1;
				} else {
					backgroundFlag = 1;
					instr.numTokens--;
				}
			}
		}
		if (amberNotLast == 1) {
			printf("Invalid null command.\n");
			clearInstruction(&instr);
			continue;
		}

		// cd PATH
		int cdFlag = 0;
		int singleCD = 0;
		if(!strcmp(instr.tokens[0], "cd")) {
			if(instr.numTokens == 1) {
				cdFlag = 1;
				singleCD = 1;
			} else {
				if(instr.tokens[1][0] != '/') {
					tempy1 = (char *) malloc(strlen(getenv("PWD")) + strlen(instr.tokens[1]) + 2);
					strcpy(tempy1, getenv("PWD"));
					strcat(tempy1, "/");
					strcat(tempy1, instr.tokens[1]);

					instr.tokens[1] = (char *) malloc(strlen(getenv("PWD")) + strlen(instr.tokens[1]) + 2);
					strcpy(instr.tokens[1], tempy1);
				}


				struct stat stats;
			    stat(instr.tokens[1], &stats);

			    if (S_ISDIR(stats.st_mode)) {
			    	cdFlag = 1;
			    }
		    	else {
			    	printf("bash: cd: %s: No such file or directory\n", instr.tokens[1]);
			    	clearInstruction(&instr);
					continue;
			    }
			}
		}	//Done with cd





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
		numofExecCommands++;
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

		if (!strcmp(instr.tokens[0], "|") || !strcmp(instr.tokens[instr.numTokens -1], "|"))
		{
			printf("bash: syntax error near unexpected token '|'\n");
			clearInstruction(&instr);
			pipeError = 1;
			continue;
		}


		//Part 10 alias
		//we want to do alias stuff now so that we can execute whatever commands the alias turns into afterwards
		//add alias
		//just making sure that they're free, might not be necessary but just a precaution
		free(tempy1);
		free(tempy2);
		tempy1 = NULL;
		tempy2 = NULL;
		int county3 = 0;
		for(i = 0; i < numofcom; i++)
		{
			if(strcmp(instr.tokens[comspots[i]], "alias") == 0)
			{
				county = 0;
				county2 = 0;
				for(j = i + 1; j < instr.numTokens; j++)
				{
					if (strcmp(instr.tokens[j], "=") == 0)
						break;
					else
						county++;
				}
				for(j = i  + 1; j < county + i  + 1; j++)
				{
					county2 = county2 + strlen(instr.tokens[j]);
					if(j != county  + i - 1)
						county2 = county2 + 1;
				}
				tempy1 = (char *) malloc(county2 + 1);;
				for(j = i + 1; j < county + i + 1; j++)
				{
					if (j == i + 1)
						strcpy(tempy1, instr.tokens[j]);
					else
						strcat(tempy1, instr.tokens[j]);
					if (j != county + i)
						strcat(tempy1, " ");
				}
				//tempy1 is now storing the "alias" part of the alias
				//"=" sign is at i + county  + 1
				county2 = 0;
				for(j = county + i + 2; j < instr.numTokens; j++)
				{
					//removing the ' from the first token after the = sign
					if(j == county + i + 2)
					{
						memcpy(instr.tokens[j], &instr.tokens[j][1], strlen(instr.tokens[j]));
					}
					//checking for the last ' at the end of tokens
					if(instr.tokens[j][strlen(instr.tokens[j]) - 1] == '\'')
					{
						instr.tokens[j][strlen(instr.tokens[j]) - 1] = '\0';
						county2 = county2 + 1;
						break;
					}
					county2 = county2 + 1;
				}
				for(j = 0; j < county2; j++)
				{
					county3 = county3 + strlen(instr.tokens[i + county + 2 + j]);
					if(j != county + i + 1 + county2)
						county3++;
				}
				tempy2 = (char *) malloc(county3);


				for(j = 0; j < county2; j++)
				{
					if(j == 0)
						strcpy(tempy2, instr.tokens[i + county + 2 + j]);
					else
						strcat(tempy2, instr.tokens[i + county + 2 + j]);
					if (j != county2 - 1)
						strcat(tempy2, " ");
				}
				//tempy2 is now holding the right side of the "=" sign which is the command

				//checking if the command or the alias is already used
				county = -1;																														//-1 to say there wasn't a match thus far
				for(j = 0; j < numalias; j++)
				{
					if(strcmp(tempy2, aliaslist[j].command) == 0 || strcmp(tempy1, aliaslist[i].alias) == 0)
					{
						county = j;
						break;
					}
				}
				if(county != -1)
				{
					strcpy(aliaslist[county].command, tempy2);
					strcpy(aliaslist[county].alias, tempy1);
					aliaslist[county].commandwords = county2;
				}
				else if (numalias < 10)																							//max 10 allias
				{
					aliaslist[numalias].command = (char *) malloc(strlen(tempy2) + 1);
					strcpy(aliaslist[numalias].command, tempy2);
					aliaslist[numalias].alias = (char *) malloc(strlen(tempy1) + 1);
					strcpy(aliaslist[numalias].alias, tempy1);
					aliaslist[numalias].commandwords = county2;
					numalias++;
				}
				else
					printf("Error: the alias list is already maxed out at 10\n");
				free(tempy1);
				free(tempy2);
				tempy1 = NULL;
				tempy2 = NULL;
			}
		}//done with alias


		//unalias
		for(i = 0; i < numofcom; i++)
		{
			if(strcmp(instr.tokens[comspots[i]], "unalias") == 0)
			{
				county = 0;
				tempy1 = NULL;
				//grabbing what the alias you're trying to remove is
				for(j = i + 1; j < instr.numTokens; j++)
				{
					county = county + strlen(instr.tokens[j]);
					//condition to make room for a space as long as it isn't the last token
					if(j != instr.numTokens - 1)
						county = county + 1;
				}
				tempy1 = (char *) malloc(county + 1);
				for(j = i + 1; j < instr.numTokens; j++)
				{
					if(j == i + 1)
						strcpy(tempy1, instr.tokens[j]);
					else
						strcat(tempy1, instr.tokens[j]);
					if(j != instr.numTokens - 1)
						strcat(tempy1, " ");
				}
				//tempy1 is now storing the alias that you're trying to remove

				//if there are no alias at all then the alias you're trying to remove obviously doesn't exist
				if(numalias == 0)
					printf("Error: Invalid alias trying to be removed\n");
				//going through aliaslist to see if the alias that you're trying to delete exists
				for(j = 0; j < numalias; j++)
				{
					if(strcmp(aliaslist[j].alias, tempy1) == 0)
					{
						numalias--;
						break;
					}
					//if you went through whole aliaslist and didn't find the alias
					if(j == numalias - 1)
						printf("Error: Invalid alias trying to be removed\n");
				}
			}
		}//end of unalias

		if(!strcmp(instr.tokens[0], "exit") && instr.numTokens == 1)
		{
				printf("Exiting...\n");
				printf("Commands Executed: %d\n", numofExecCommands);
				exit(0);
		}

		//SECTION: Built in commands (part 6 stuff)
		//checking to make sure the arguement isn't any of the built in commands that we have to build ourselves for part 10
		if(strcmp(instr.tokens[0], "exit") != 0  && strcmp(instr.tokens[0], "alias") != 0 && strcmp(instr.tokens[0], "unalias"))
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
					pipey = 1;
					pipeLocation = i;

					tempy1 = (char *) malloc(strlen(instr.tokens[i+1]) + 6);
					strcpy(tempy1, "/bin/");
					strcat(tempy1, instr.tokens[i+1]);

					county = 0;
					for(j = i+2; j < instr.numTokens; j++) {
						if( j == instr.numTokens)
							break;
						else if(strcmp(instr.tokens[j], "|") == 0) {
							pipey = 2;
							pipeLocation2 = j;

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

			if(pipey == 1 || pipey == 2 && pipeError == 0) {
				parmlist2[0] = tempy1;
				for(i = 1; i < county + 1; i++) {
					if(instr.tokens[i+pipeLocation+1] == "|")
						break;
					parmlist2[i] = instr.tokens[i+pipeLocation+1];
				}
				parmlist2[county + 1] = NULL;
				if(pipey == 2) {
					parmlist3[0] = tempy2;
					for(i = 1; i < county2 + 1; i++) 																														//adding all the parameters to the parmlist
							parmlist3[i] = instr.tokens[i+pipeLocation2+1];
					parmlist3[county2 + 1] = NULL;
				}
			}


			//STARTING TO EXECUTE COMMANDS
			//----------------------------
			//This part works without any I/O Redirection or Piping so I'm not touching it
			if (outred == 0 && inred == 0 && pipey == 0 && pipeError == 0)
			{
				int status;																																									//this is for the waitpid function. status is 0 if there is no error.
				pid_t pid = fork();
				if (pid == -1) {																																	//this code was making the prompt print out twice for some reason so i just commented it out for now
								printf("Forking Error");
								numofExecCommands--;
				}
				else if(pid == 0) 																																					//pid of 0 means the child process was successfully created
				{
					//Part 10 echo
					if (strcmp(temp, "/bin/echo") == 0)
					{
						char* output = NULL;
						echo(parmlist, parmlistlen, output);
						exit(1);
					}
					else if(cdFlag == 1 && singleCD == 0) {
						int chDirTest = chdir(instr.tokens[1]);
						if(chDirTest == -1) {
							printf("chdir failed\n");
							numofExecCommands--;
							exit(1);
						} else if (chDirTest == 0) {
							setenv("PWD", instr.tokens[1], 1);
						}
					}
					else if(cdFlag == 1 && singleCD == 1) {
						int chDirTest = chdir(getenv("HOME"));
						if(chDirTest == -1) {
							printf("chdir failed\n");
							numofExecCommands--;
							exit(1);
						} else if (chDirTest == 0) {
							setenv("PWD", getenv("HOME"), 1);
						}
					}
					else
					{
						execv(temp, parmlist);																																		//this is what executes all the prebuilt commands. temp is the command name (at the end of the path) and parmlist is all the parameters for that command
						printf("execv failed\n");
						numofExecCommands--;																														//if an invalid command was inputted then execv returns and hits this line to display an error code
						exit(1);
						}
				}
				else
					waitpid(pid, &status, 0);																															//this else is for the parent to hit. The parent hits this while the child hits the else if. Parent waits for child to finish
			}

			//Just Output Redirection
			else if(outred == 1 && inred == 0 && pipey == 0 && pipeError == 0 && cdFlag == 0)
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
				if (pid == -1) {																																		//this code was making the prompt print out twice for some reason so i just commented it out for now
			          printf("Forking Error");
								numofExecCommands--;
				}
			  else if(pid == 0) 																																					//pid of 0 means the child process was successfully created
			  {
			    open(tempy1, O_RDWR | O_CREAT | O_TRUNC);
			    close(1);
			    dup(3);
			    close(3);
			    execv(temp, parmlist);
			    printf("exec failed\n");
					numofExecCommands--;
			  }
			  else
			  	waitpid(pid, &status, 0);

				free(tempy1);
				tempy1 = NULL;
			}
			//Just Input Redirection
			else if(outred == 0 && inred == 1 && pipey == 0 && pipeError == 0 && cdFlag == 0)
			{
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


				if (pid == -1) {																																		//this code was making the prompt print out twice for some reason so i just commented it out for now
					printf("Forking Error");
					numofExecCommands--;
				}
				else if(pid == 0) 																																					//pid of 0 means the child process was successfully created
				{
					open(tempy1, O_RDONLY);
					close(0);
					dup(3);
					close(3);
					execv(temp, parmlist);
					printf("exec failed\n");
					numofExecCommands--;
				}
				else
					waitpid(pid, &status, 0);

				free(tempy1);
				tempy1 = NULL;
			}//end of input redirection

			//Both input and output redirection
			else if(outred == 1 && inred == 1 && pipey == 0 && pipeError == 0 && cdFlag == 0)
			{
				if (strcmp(temp, "/bin/echo") == 0)
				{
					clearInstruction(&instr);
					outred = 0;
					inred = 0;
					pipey = 0;
					pipeError = 0;
					continue;
				}

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
				if (pid == -1) {
					printf("Forking Error");
					numofExecCommands--;
				}
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
					numofExecCommands--;
				}
				else
					waitpid(pid, &status, 0);

				free(tempy1);
				tempy1 = NULL;
				free(tempy2);
				tempy2 = NULL;
			}//done with both input and output redirection

			// Single Pipe
			else if(outred == 0 && inred == 0 && pipey == 1 && pipeError == 0 && cdFlag == 0)
			{
				// 0 is read end, 1 is write end
				int status;
			    int fd[2];

			    pid_t pid1 = fork();
			    if(pid1 == -1) {
			    	printf("Forking Error");
						numofExecCommands--;
					}
			    else if(pid1 == 0) {
			    	int piper = pipe(fd);
			    	if( piper == -1) {
			    		printf("Piping Error\n");
							numofExecCommands--;
						}
			    	else if(piper == 0) {
			    		pid_t pid2 = fork();
			    		if(pid2 == -1) {
			    			printf("Forking Error");
								numofExecCommands--;
							}
			    		else if(pid2 == 0) {
								close(1);
			    			dup(fd[1]);
			    			close(fd[0]);
			    			close(fd[1]);
			    			execv(parmlist[0], parmlist);
								printf("exec failed\n");
								numofExecCommands--;
			    		}
			    		else {
			    			close(0);
			    			dup(fd[0]);
			    			close(fd[0]);
			    			close(fd[1]);
			    			execv(parmlist2[0], parmlist2);
								printf("exec failed\n");
								numofExecCommands--;
			    		}
			    	}
			    }
			    else {
			    	waitpid(pid1, &status, 0);
			    }
			}
/*
	//DOUBLE PIPES-------------------------
			else if(outred == 0 && inred == 0 && pipey == 2 && pipeError == 0)
			{
				int status1, status2;
				pid_t pid1, pid2, pid3;
				int fd1[2], fd2[2];
				int piper1, piper2;

				pid1 = fork();
				if(pid1 == -1)
					printf("Forking Error\n");
				else if (pid1 == 0)
				{
					piper1 = pipe(fd1);
					if(piper1 == -1)
						printf("Piping Error\n");
					else if(piper1 == 0)
					{
						pid2 = fork();
						if(pid2 == -1)
							printf("Forking Error\n");
						else if(pid2 == 0)
						{
							piper2 = pipe(fd2);
							if(piper2 == -1)
								printf("Piping Error\n");
							else if(piper2 == 0)
							{
								pid3 = fork();
								if(pid3 == -1)
									printf("Forking Error\n");
								else if(pid3 == 0)
								{
									//command 3
									printf("command 3\n");
									close(0);
									dup(fd2[0]);
									close(fd2[0]);
									close(fd2[1]);
									execv(parmlist2[0], parmlist2);
									printf("exec failed\n");
								}
								else
								{
									//command 2
									//changing where cmd2 is getting input
									printf("command 2\n");
									close(0);
				    			dup(fd1[0]);
				    			close(fd1[0]);
				    			close(fd1[1]);
									//changing where cmd2 is outputting to
									close(1);
									dup(fd2[1]);
									close(fd2[0]);
									close(fd2[1]);
				    			execv(parmlist2[0], parmlist2);
									printf("exec failed\n");
								}
							}
						}
						else
						{
							waitpid(pid2, &status2, 0);
						}
					}
				}
				else
					waitpid(pid1, &status1, 0);
			}//end of double pipe
*/
			free(temp);
			temp = NULL;
			free(tempy1);
			tempy1 = NULL;
			free(tempy2);
			tempy2 = NULL;
			outred = 0;
			inred = 0;
			pipey = 0;
			pipeError = 0;
		}//End of Built in Functions

		//SECTION: ending stuff from parserhelp.c
		addNull(&instr);
		//printTokens(&instr);
		clearInstruction(&instr);
	}

	return 0;
}


//echo command for part 10
void echo(char** parmlist, int len, char* output)
{
	int i;
	if(output == NULL)
	{
		for(i = 1; i < len - 1; i++)
			printf("%s ", parmlist[i]);
		printf("\n");
	}
	else
	{
		struct stat stats;
		if(stat(output, &stats) != 0)
		{
			printf("Invalid file path: %s\n", output);
			exit(1);
		}
		else
		{
			FILE *f = fopen(output, "rw");
			if (f == NULL)
		  {
		    printf("Error opening the file\n");
		    exit(1);
		  }
			for(i = 1; i < len - 1; i++)
				fprintf(f, "%s ", parmlist[i]);
		}
	}
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
