/*
 * os345argparse.c
 *
 *  Created on: Oct 3, 2016
 *      Author: jbarzee
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "os345.h"
#include "os345argparse.h"

/* This library will parse a argument command line.
 *		LANGUAGE:
 *
 *		LINE=> (ARG WS)* &
 *		ARG	=> S | QS
 *		QS	=> 'QDS' | "QDS"
 *		QDS	=> a-z | A-Z | . | :			NOTE: case sensitive and quote delimited
 *		S	=> a-z | A-Z | . | :			NOTE: case in-sensitive and space delimited
 *		WS	=> \s | \n | \r | \t | \v | \f
 */

//uncomment to debug parser
void parserDebugPrint(char type, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
	if (type == 'f') {
		//vprintf(fmt, args);		//print function names
	} else if (type == 'p') {
		//vprintf(fmt, args);	//print push/pop
	} else if (type == 'm') {
		//vprintf(fmt, args);	//print mallocs
	} else {
		//vprintf(fmt, args);	//print all else
	}
    va_end(args);
}

ParsedLine parseArgs(char* buffer) {
	parserDebugPrint('f', "parseArgs()\n");
	bufferContents = buffer;
	nextCharPos = 0;


	ParsedLine line;
	line.argc = 0;
	line.argv = malloc(MAX_ARGS * sizeof(char*));
	for (int i=1; i<MAX_ARGS; i++) line.argv[i] = NULL;
	line.runInBackground = FALSE;

	int errors = parseLine(&line.argc, line.argv, &line.runInBackground);
	//printf( "finished parsing. errors:%d\n", errors );


	return line;
}

int parseLine(int* newArgc, char** newArgv, bool* background) {
	parserDebugPrint('f', "parseLine()\n");
	int errors = 0;
	while(peekChar() && peekChar() != '&') {
		errors += parseArg(newArgc, newArgv);
		errors += parseWhiteSpace(newArgc, newArgv);
	}
	errors += parseAmpersand(background);
	errors += parseWhiteSpace(newArgc, newArgv);
	if (peekChar() != '\0')
		errors++;
	return errors;
}

int parseArg(int* newArgc, char** newArgv) {
	parserDebugPrint('f', "parseArg()\n");
	if (isQuote(peekChar())) {
		return parseQuotedString(newArgc, newArgv);
	} else {
		return parseString(newArgc, newArgv);
	}
}

int parseQuotedString(int* newArgc, char** newArgv) {
	parserDebugPrint('f', "parseQuotedString()\n");
	if (!isQuote(peekChar())) {
		return 1;
	} else {
		char openQuote = popChar();
		int errors = 0;
		errors = parseQuoteDelimitedString(newArgc, newArgv);
		if (!errors && openQuote == peekChar()) {
			popChar();
			return 0;
		} else {
			return ++errors;
		}
	}
}

int parseQuoteDelimitedString(int* newArgc, char** newArgv) {
	parserDebugPrint('f', "parseQuoteDelimitedString()\n");
	char arg[MAX_ARG_LEN] = "";
	char nextChar;
	while (peekChar() != '\0') {
		nextChar = peekChar();
		if (isStringChar(nextChar) || isspace(nextChar)) {
			nextChar = popChar();
			strncat(arg, &nextChar, 1);   //append the new char
		} else if (isQuote(nextChar)) {
			newArgv[(*newArgc)] = malloc((strlen(arg)+1) * sizeof(char));
			strcpy(newArgv[(*newArgc)], arg);
			parserDebugPrint('m', "malloced Arg %s\n", newArgv[(*newArgc)]);
			(*newArgc)++;
			return 0;
		} else {
			return 1;
		}
	}
}

int parseString(int* newArgc, char** newArgv) {
	parserDebugPrint('f', "parseString()\n");
	char arg[MAX_ARG_LEN] = "";
	char nextChar;
	while (1) {
		nextChar = peekChar();
		if (isStringChar(nextChar)) {
			nextChar = tolower(popChar());
			strncat(arg, &nextChar, 1);   //append the new char
		} else if (isspace(nextChar) || nextChar == '\0') {
			newArgv[(*newArgc)] = malloc((strlen(arg)+1) * sizeof(char));
			strcpy(newArgv[(*newArgc)], arg);
			parserDebugPrint('m', "malloced Arg %s\n", newArgv[(*newArgc)]);
			(*newArgc)++;
			return 0;
		} else {
			return 1;
		}
	}
}

int parseWhiteSpace(int* newArgc, char** newArgv) {
	parserDebugPrint('f', "parsewhiteSpace()\n");
	while(isspace(peekChar())) {
		popChar();
	}
}

int parseAmpersand(bool* background) {
	parserDebugPrint('f', "parseAmpersand()\n");
	if (peekChar() != '&') {
		return 0;
	} else {
		popChar();
		*background = TRUE;
		return 0;
	}
}


char peekChar() {
	parserDebugPrint('p', "peeked %c\n", bufferContents[nextCharPos]);
	return bufferContents[nextCharPos];
}

char popChar() {
	parserDebugPrint('p', "popped %c\n", bufferContents[nextCharPos]);
	if (bufferContents[nextCharPos])
		nextCharPos++;
	return bufferContents[nextCharPos-1];
}

bool isStringChar(char c) {
	if (isalpha(c)) {
		return 1;
	} else if (c == '.') {
		return 1;
	} else if (c == ':') {
		return 1;
	}
	return 0;
}

bool isQuote(char c) {
	if (c == '\'') {
		return 1;
	} else if (c == '"') {
		return 1;
	}
	return 0;
}
