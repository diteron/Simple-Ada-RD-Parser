#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Tokens.h"
#include "Parser.h"

extern int yylex();
extern char* yytext;
extern FILE* yyin;
extern int yylineno;

Token* tokensStream;
int currentToken;
int lexicalError = 0;

int parser_main(int argc, char* argv[]) {
	fopen_s(&yyin, "ada.txt", "r");

	tokensStream = calloc(20, sizeof(Token));
	if (tokensStream == NULL) {
		return 0;
	}
	currentToken = 0;

	int tokensNum = getAllTokens(20);

	if (tokensNum > 0) {
		int parseResult = procedure_specification();
		if (parseResult && !lexicalError) {
			printf("Success parse\n");
		}
		else if (!parseResult) {
			printParseError();
		}

		printf("\n");
		for (int i = 0; i < tokensNum; ++i) {
			printf("%d. Token No: %d, token text: %s\n", i, tokensStream[i].tokenNo, tokensStream[i].tokenText);
			freeTokenText(tokensStream[i].tokenText);
		}
	}
	free(tokensStream);
	system("pause");
	return 0;
}

void freeTokenText(char* tokenText) {		// Отдельная функция из-за предупреждения компилятора C6001
	free(tokenText);
}

// Перевыделение памяти под поток токенов
Token* tokensRealloc(int newCapacity, int oldCapacity) {
	Token* temp = realloc(tokensStream, sizeof(Token) * newCapacity);

	if (newCapacity < oldCapacity) {
		oldCapacity = newCapacity;
	}

	if (temp == NULL) {
		for (int i = oldCapacity; i >= 0; --i) {
			freeTokenText(tokensStream[i].tokenText);
		}
		free(tokensStream);
		printf("Error of memory reallocation\n");
	}

	return temp;
}

// Поместить все токены в массив tokenStream
int getAllTokens(int size) {
	int tokenNo = 1;
	int currentCapacity = size;

	int i = 0;
	while (tokenNo) {
		if (currentCapacity <= i) {
			int newCapacity = currentCapacity * 2;
			tokensStream = tokensRealloc(newCapacity, currentCapacity);
			if (tokensStream == NULL) {
				return 0;
			}
			currentCapacity = newCapacity;
		}

		tokenNo = yylex();

		tokensStream[i].tokenNo = tokenNo;

		size_t yytextLen = strlen(yytext) + 1;
		tokensStream[i].tokenText = calloc(yytextLen, sizeof(char));
		if (tokensStream[i].tokenText == NULL) {
			for (i = i - 1; i >= 0; --i) {	// Освободить выделенную память под предыдущие строки
				freeTokenText(tokensStream[i].tokenText);
			}
			return i;
		}
		strcpy_s(tokensStream[i].tokenText, yytextLen * sizeof(char), yytext);

		tokensStream[i].tokenLineNo = yylineno;

		printf("%d. Token No: %d, token text: %s\n", i, tokensStream[i].tokenNo, tokensStream[i].tokenText);
		++i;
	}

	if (i < currentCapacity) {
		tokensStream = tokensRealloc(i, currentCapacity);
		if (tokensStream == NULL) {
			return 0;
		}
	}

	return i;
}

//////////////////////////////////////////////////
//												//
//		Парсинг методом рекурсивного спуска		//
//												//
//////////////////////////////////////////////////

void printParseError() {
	fprintf(stderr, "Syntax error unexpected \"%s\" in line %d\n",
	tokensStream[currentToken].tokenText, tokensStream[currentToken].tokenLineNo);
}

// Проверить является ли текущий токен в потоке ключевым словом
int keyWord(int keyWord) {
	int res = tokensStream[currentToken].tokenNo == keyWord;
	if (res) { 
		++currentToken;
	}

	return res;
}

int procedure_specification() {
	if (keyWord(PROC)) {
		if (keyWord(IDENTIFIER)) {
			int identifierToken = currentToken - 1;
			if (opt_parameters()) {
				if (procedure_body(identifierToken)) {
					if (end_proc(identifierToken)) {
						return SUCCESS_PARSE;
					}
				}
			}
		}
	}

	return FAILED_PARSE;
}

int opt_parameters() {
	if (keyWord('(')) {
		if (parameters_list()) {
			if (keyWord(')')) {
				return SUCCESS_PARSE;
			}
		}
		return FAILED_PARSE;
	}
	
	return SUCCESS_PARSE;
}

int parameters_list() {
	if (parameter()) {
		if (next_parameters()) {
			return SUCCESS_PARSE;
		}
	}

	return FAILED_PARSE;
}

int next_parameters() {
	if (keyWord(';')) {
		if (parameter()) {
			if (next_parameters()) {
				return SUCCESS_PARSE;
			}
		}

		return FAILED_PARSE;
	}

	return SUCCESS_PARSE;
}

int parameter() {
	if (keyWord(IDENTIFIER)) {
		if (keyWord(':')) {
			if (keyWord(TYPE)) {
				return SUCCESS_PARSE;
			}
		}
	}

	return FAILED_PARSE;
}

int procedure_body(int identifierToken) {
	if (keyWord(IS)) {
		if (opt_variables()) {
			if (keyWord(BEGiN)) {
				if (statements_list()) {
					return SUCCESS_PARSE;
				}
			}
		}
	}

	return FAILED_PARSE;
}

int opt_variables() {
	int savePos = currentToken;
	if (parameter()) {
		if (keyWord(';')) {
			opt_variables();
			return SUCCESS_PARSE;
		}

		return FAILED_PARSE;
	}
	else {
		currentToken = savePos;
		return SUCCESS_PARSE;
	}
}

int end_proc(int identifierToken) {
	if (keyWord(END)) {
		if (keyWord(IDENTIFIER)) {
			if (!strcmp(tokensStream[currentToken - 1].tokenText, tokensStream[identifierToken].tokenText)) {
				if (keyWord(';')) {
					return SUCCESS_PARSE;
				}
			}
			else {
				--currentToken;
			}
		}
		else {
			if (keyWord(';')) {
				return SUCCESS_PARSE;
			}
		}
	}

	return FAILED_PARSE;;
}

int statements_list() {
	if (statement()) {
		statements_list();
		return SUCCESS_PARSE;
	}

	return FAILED_PARSE;
}

int statement() {
	int savePos = currentToken;

	if (assign_statement()) {
		return SUCCESS_PARSE;
	}

	int posForErr = currentToken;
	currentToken = savePos;
	if (compound_statement()) {
		return SUCCESS_PARSE;
	}

	currentToken = posForErr;
	return FAILED_PARSE;
}

int assign_statement() {
	if (keyWord(IDENTIFIER)) {
		if (keyWord(ASSIGN)) {
			if (simple_expression()) {
				if (keyWord(';')) {
					return SUCCESS_PARSE;
				}
			}
		}
	}

	return FAILED_PARSE;
}

int simple_expression() {
	int savePos = currentToken;
	if (term()) {
		added();
		return SUCCESS_PARSE;
	}

	currentToken = savePos;
	if (adding()) {
		if (term()) {
			if (added()) {
				return SUCCESS_PARSE;
			}
		}

		return FAILED_PARSE;
	}

	return FAILED_PARSE;
}

int term() {
	if (factor()) {
		if (multiplier()) {
			return SUCCESS_PARSE;
		}
	}

	return FAILED_PARSE;
}

int added() {
	if (adding()) {
		if (term()) {
			if (added()) {
				return SUCCESS_PARSE;
			}
		}

		return FAILED_PARSE;
	}

	return SUCCESS_PARSE;
}

int adding() {
	int sign = tokensStream[currentToken].tokenNo;
	int res = (sign == '+' || sign == '-');
	if (res) {
		++currentToken;
	}
	return res;
}

int factor() {
	if (number()) {
		return SUCCESS_PARSE;
	}
	else if (keyWord(IDENTIFIER)) {
		return SUCCESS_PARSE;
	}
	else {
		if (keyWord('(')) {
			if (simple_expression()) {
				if (keyWord(')')) {
					return SUCCESS_PARSE;
				}
			}
		}

		return FAILED_PARSE;
	}
}

int number() {
	int num = tokensStream[currentToken].tokenNo;
	int res = (num == INTEGER || num == DECIMAL);
	if (res) {
		++currentToken;
	}
	return res;
}

int multiplier() {
	if (multiplying()) {
		if (factor()) {
			if (multiplier()) {
				return SUCCESS_PARSE;
			}
		}

		return FAILED_PARSE;
	}

	return SUCCESS_PARSE;
}

int multiplying() {
	int sign = tokensStream[currentToken].tokenNo;
	int res = (sign == '*' || sign == '/');
	if (res) {
		++currentToken;
	}
	return res;
}

int compound_statement() {
	int savePos = currentToken;
	if (if_statement()) {
		return SUCCESS_PARSE;
	}

	currentToken = savePos;
	if (while_statement()) {
		return SUCCESS_PARSE;
	}

	return FAILED_PARSE;
}

int if_statement() {
	if (if_cond_part()) {
		if (statements_list()) {
			if (opt_elsif()) {
				if (opt_else()) {
					if (endif()) {
						return SUCCESS_PARSE;
					}
				}
			}
		}
	}

	return FAILED_PARSE;
}

int if_cond_part() {
	if (keyWord(IF)) {
		if (condition()) {
			if (keyWord(THEN)) {
				return SUCCESS_PARSE;
			}
		}
	}

	return FAILED_PARSE;
}

int condition() {
	if (simple_expression()) {
		if (next_condition()) {
			return SUCCESS_PARSE;
		}
	}

	return FAILED_PARSE;
}

int next_condition() {
	if (relational()) {
		if (condition()) {
			if (next_condition()) {
				return SUCCESS_PARSE;
			}
		}

		return FAILED_PARSE;
	}

	return SUCCESS_PARSE;
}

int relational() {
	int sign = tokensStream[currentToken].tokenNo;
	int res = (sign == '<' || sign == LE || sign == '=' || sign == '>' || sign == GE || sign == NE);
	if (res) {
		++currentToken;
	}
	return res;
}

int opt_elsif() {
	if (keyWord(ELSIF)) {
		if (elsif_cond_part()) {
			if (statements_list()) {
				if (opt_elsif()) {
					return SUCCESS_PARSE;
				}
			}
		}
		
		return FAILED_PARSE;
	}

	return SUCCESS_PARSE;
}

int elsif_cond_part() {
	if (condition()) {
		if (keyWord(THEN)) {
			return SUCCESS_PARSE;
		}
	}

	return FAILED_PARSE;
}

int opt_else() {
	if (keyWord(ELSE)) {
		if (statements_list()) {
			return SUCCESS_PARSE;
		}

		return FAILED_PARSE;
	}

	return SUCCESS_PARSE;
}

int endif() {
	if (keyWord(END)) {
		if (keyWord(IF)) {
			if (keyWord(';')) {
				return SUCCESS_PARSE;
			}
		}
	}

	return FAILED_PARSE;
}

int while_statement() {
	return FAILED_PARSE;
}