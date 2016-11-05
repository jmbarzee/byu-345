/*
 * os345argparse.h
 *
 *  Created on: Oct 3, 2016
 *      Author: jbarzee
 */

#ifndef OS345ARGPARSE_H_
#define OS345ARGPARSE_H_

#define MAX_ARG_LEN		256

char* bufferContents;
int nextCharPos;

typedef struct {
	int argc;
	char** argv;
	bool runInBackground;
	int errors;
} ParsedLine;

ParsedLine parseArgs(char* buffer);
int parseLine(int* newArgc, char** newArgv, bool* background);
int parseArg(int* newArgc, char** newArgv);
int parseQuotedString(int* newArgc, char** newArgv);
int parseQuoteDelimitedString(int* newArgc, char** newArgv);
int parseString(int* newArgc, char** newArgv);
int parseWhiteSpace(int* newArgc, char** newArgv);
int parseAmpersand(bool* background);



char peekChar();
char popChar();

bool isStringChar(char c);
bool isQuote(char c);

int parseNum(char* str);

#endif /* OS345ARGPARSE_H_ */
