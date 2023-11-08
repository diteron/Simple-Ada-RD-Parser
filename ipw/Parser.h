#pragma once
#define SUCCESS_PARSE 1
#define FAILED_PARSE 0

struct {
    int tokenNo;
    int tokenLineNo;
    char* tokenText;
} typedef Token;

void freeTokenText(char* tokenText);
Token* tokensRealloc(int newCapacity, int oldCapacity);
int getAllTokens(int size);
void printParseError();

int keyWord(int keyWord);
int skipToNextStmt();

int function_declaration();
int opt_parameters();
int parameters_list();
int next_parameters();
int parameter();
int func_return();
int function_body();
int opt_variables();
int end_func(int identifierToken);
int statements_list();
int statement();
int assign_statement();
int expression();
int next_expression();
int relational();
int simple_expression();
int term();
int added();
int adding();
int factor();
int number();
int multiplier();
int multiplying();
int compound_statement();
int if_statement();
int if_cond_part();
int opt_elsif();
int opt_else();
int endif();
int while_statement();
int while_cond_part();
int end_loop();
int return_statement();
