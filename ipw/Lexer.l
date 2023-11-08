%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Tokens.h"

#define fileno _fileno
extern int lexicalError;
%}

%option noyywrap
%option yylineno

%%

"function" { return FUNC; }
"Integer"|"Float"|"Boolean"|"Character" { return TYPE; }
":="        { return ASSIGN; }
"is"        { return IS; }
"begin"     { return BEGiN; }
"end"       { return END; }
"<="        { return LE; }
">="        { return GE; }
"/="        { return NE; }
"if"        { return IF; }
"then"      { return THEN; }
"elsif"     { return ELSIF; }
"else"      { return ELSE; }
"while"     { return WHILE; }
"loop"      { return LOOP; }
"return"    { return RETURN; }
"("         { return '('; }
")"         { return ')'; }
"<"         { return '<'; }
">"         { return '>'; }
"="         { return '='; }
"+"         { return '+'; }
"-"         { return '-'; }
"*"         { return '*'; }
"/"         { return '/'; }
":"         { return ':'; }
";"         { return ';'; }

[a-zA-Z](_?[a-zA-Z0-9])*    { return IDENTIFIER; }
[0-9]+                      { return INTEGER; }
[0-9]+(\.?[0-9]*)           { return DECIMAL; }

[ \n\t\f]    ;

. {
    lexicalError = TRUE;
    fprintf(stderr, "Lexical error: unexpexted character \"%s\" in line %d\n", yytext, yylineno);
}
%%