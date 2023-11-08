#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Tokens.h"
#include "Parser.h"

//#define DEBUG

extern int yylex();
extern char* yytext;
extern FILE* yyin;
extern int yylineno;

Token* tokensStream;
int currentToken;
int lexicalError = FALSE;
int missSemicolon = FALSE;
int syntaxErr = FALSE;

int returns = 0;
int functions = 0;

int parser_main(int argc, char* argv[]) {
    if (argc > 1) {
        fopen_s(&yyin, argv[1], "r");
        if (yyin == NULL) {
            printf("No such file\n");
            return 0;
        }
    }
    else {
        printf("Enter file name to parse\n");
        return 0;
    }

    tokensStream = calloc(20, sizeof(Token));
    if (tokensStream == NULL) {
        return 0;
    }

    int tokensNum = getAllTokens(20);

    currentToken = 0;
    int parseResult = function_declaration();
    if (!parseResult) {
        printParseError();
    }

    while (tokensStream[currentToken].tokenNo != 0 && parseResult) {
        parseResult = function_declaration();
        if (!parseResult) {
            printParseError();
        }
    }

    if (parseResult && returns >= functions && !lexicalError && !missSemicolon && !syntaxErr) {
        printf("Success parse\n");
    }

#ifdef  DEBUG
    printf("\n");
    for (int i = 0; i < tokensNum; ++i) {
        printf("%d. Token No: %d, token text: %s\n", i, tokensStream[i].tokenNo, tokensStream[i].tokenText);
    }
#endif

    for (int i = 0; i < tokensNum; ++i) {
        freeTokenText(tokensStream[i].tokenText);
    }

    free(tokensStream);
    return 0;
}

void freeTokenText(char* tokenText) {        // Отдельная функция из-за предупреждения компилятора C6001
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
            for (i = i - 1; i >= 0; --i) {    // Освободить выделенную память под предыдущие строки
                freeTokenText(tokensStream[i].tokenText);
            }
            return i;
        }
        strcpy_s(tokensStream[i].tokenText, yytextLen * sizeof(char), yytext);
        
        tokensStream[i].tokenLineNo = yylineno;

#ifdef DEBUG
        printf("%d. Token No: %d, token text: %s\n", i, tokensStream[i].tokenNo, tokensStream[i].tokenText);
#endif
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

void printParseError() {
    if (tokensStream[currentToken].tokenText[0] != '\0') {
        fprintf(stderr, "Syntax error: unexpected \"%s\" in line %d\n",
            tokensStream[currentToken].tokenText, tokensStream[currentToken].tokenLineNo);
    }
    else {
        fprintf(stderr, "Syntax error: unexpected EOF in line %d\n", tokensStream[currentToken].tokenLineNo);
    }
}


//////////////////////////////////////////////////
//                                              //
//     Парсинг методом рекурсивного спуска      //
//                                              //
//////////////////////////////////////////////////


// Проверить является ли текущий токен в потоке ключевым словом
int keyWord(int keyWord) {
    int res = tokensStream[currentToken].tokenNo == keyWord;

    // Ошибка об отсутствующей ';'. Если ';' отсутствует после последнего параметра
    // при объявлении функции, то ошибка не выводится
    if (!res && keyWord == ';' && tokensStream[currentToken].tokenNo != ')') {
        fprintf(stderr, "Syntax error: missed ';' in line %d\n", tokensStream[currentToken - 1].tokenLineNo);
        ++missSemicolon;
        return SUCCESS_PARSE;        // Возврат 1 для проверки следующей инструкции 
    }

    if (res) { 
        ++currentToken;
    }

    return res;
}

// Переход к следующей корректной инструкции (режим паники)
int skipToNextStmt() {
    if (tokensStream[currentToken].tokenNo != END && tokensStream[currentToken].tokenNo != ELSE && tokensStream[currentToken].tokenNo != ELSIF) {
        syntaxErr = TRUE;
        printParseError();
        while (tokensStream[currentToken].tokenNo != 0) {
            if (tokensStream[currentToken].tokenNo == ';' || tokensStream[currentToken + 1].tokenNo == THEN) {
                ++currentToken;
                return SUCCESS_PARSE;
            }

            if (tokensStream[currentToken].tokenNo == END) {
                while (tokensStream[currentToken].tokenNo != 0) {
                    if (tokensStream[currentToken].tokenNo == ';' || tokensStream[currentToken + 1].tokenNo == THEN) {
                        ++currentToken;
                        return SUCCESS_PARSE;
                    }
                    ++currentToken;
                }
            }
            ++currentToken;
        }
    }

    return FAILED_PARSE;
}

int function_declaration() {
    if (keyWord(FUNC)) {
        if (keyWord(IDENTIFIER)) {
            int identifierToken = currentToken - 1;
            if (opt_parameters()) {
                if (func_return()) {
                    int currReturns = returns;
                    if (function_body()) {
                        if (end_func(identifierToken)) {
                            ++functions;
                            if (currReturns == returns) {    // Если число return'ов не увеличилось, значит в этой функции нет return'а
                                printf("Missed return statement in function %s\n", tokensStream[identifierToken].tokenText);
                            }
                        }
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

int func_return() {
    if (keyWord(RETURN)) {
        if (keyWord(TYPE)) {
            return SUCCESS_PARSE;
        }
    }

    return FAILED_PARSE;
}

int function_body() {
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
    //int savePos = currentToken;
    if (parameter()) {
        if (keyWord(';')) {
            opt_variables();
            return SUCCESS_PARSE;
        }

        return FAILED_PARSE;
    }

    //currentToken = savePos;
    return SUCCESS_PARSE;
}

int end_func(int identifierToken) {
    if (keyWord(END)) {
        if (keyWord(IDENTIFIER)) {
            // Проверка совпадает ли идентификатор с идентификатором функции при ее объвлении
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

    return FAILED_PARSE;
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
    int errToken = currentToken;

    
    //currentToken = savePos;
    if (compound_statement()) {
        return SUCCESS_PARSE;
    }

    if (errToken < currentToken) {
        errToken = currentToken;
    }

    currentToken = savePos;
    if (return_statement()) {
        return SUCCESS_PARSE;
    }

    if (errToken < currentToken) {
        errToken = currentToken;
    }

    currentToken = errToken;
    if (skipToNextStmt()) {
        return SUCCESS_PARSE;
    }

    return FAILED_PARSE;
}

int assign_statement() {
    if (keyWord(IDENTIFIER)) {
        if (keyWord(ASSIGN)) {
            if (expression()) {
                if (keyWord(';')) {
                    return SUCCESS_PARSE;
                }
            }
        }
        --currentToken;        // Возврат назад если идентификатор корректный, но дальше нет присваивания
    }

    return FAILED_PARSE;
}

int expression() {
    if (simple_expression()) {
        if (next_expression()) {
            return SUCCESS_PARSE;
        }
    }

    if (skipToNextStmt()) {
        return SUCCESS_PARSE;
    }

    return FAILED_PARSE;
}

int next_expression() {
    if (relational()) {
        if (expression()) {
            if (next_expression()) {
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
    }

    return FAILED_PARSE;
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

    int posForErr = currentToken;
    currentToken = savePos;
    if (while_statement()) {
        return SUCCESS_PARSE;
    }

    if (posForErr > currentToken) {
        currentToken = posForErr;
    }

    return FAILED_PARSE;
}

int if_statement() {
    if (keyWord(IF)) {
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
    }

    return FAILED_PARSE;
}

int if_cond_part() {
    if (expression()) {
        if (keyWord(THEN)) {
            return SUCCESS_PARSE;
        }
    }

    return FAILED_PARSE;
}

int opt_elsif() {
    if (keyWord(ELSIF)) {
        if (if_cond_part()) {
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
    if (while_cond_part()) {
        if (statements_list()) {
            if (end_loop()) {
                return SUCCESS_PARSE;
            }
        }
    }

    return FAILED_PARSE;
}

int while_cond_part() {
    if (keyWord(WHILE)) {
        if (expression()) {
            if (keyWord(LOOP)) {
                return SUCCESS_PARSE;
            }
        }
    }

    return FAILED_PARSE;
}

int end_loop() {
        if (keyWord(END)) {
            if (keyWord(LOOP)) {
                if (keyWord(';')) {
                    return SUCCESS_PARSE;
                }
            }
        }

    return FAILED_PARSE;
}

int return_statement() {
    if (keyWord(RETURN)) {
        if (expression()) {
            if (keyWord(';')) {
                ++returns;
                return SUCCESS_PARSE;
            }
        }
    }

    return FAILED_PARSE;
}
