
%option noyywrap
%option header-file="lex.yy.h"

%{
     #include "Parser.tab.h"
     #ifdef __cplusplus
     #include <cstdlib>
     #else
     #include <stdlib.h>
     #endif
     int line_num = 1;
     int comments = 0;
     char* curr_buffer = NULL;
     int curr_buffer_size = 0;
     int curr_buffer_pos = 0;
     void clear_buffer() {
         if (curr_buffer != NULL)
             free(curr_buffer);
         curr_buffer = calloc(40, sizeof(char));
         memset(curr_buffer, 0, 40 * sizeof(char));
         curr_buffer_pos = 0;
         curr_buffer_size = 40;
     }
     void append_buffer(char ch) {
         if (curr_buffer_pos >= curr_buffer_size - 1) {
             curr_buffer_size *= 2;
             curr_buffer = realloc(curr_buffer, curr_buffer_size * sizeof(char));
             memset(curr_buffer + curr_buffer_pos, 0,
                    (curr_buffer_size - curr_buffer_pos) / 2);
         }
         curr_buffer[curr_buffer_pos++] = ch;
     }
     void unset_buffer() { // Does NOT free curr_buffer
         curr_buffer = NULL;
         curr_buffer_pos = 0;
         curr_buffer_size = 0;
     }
%}

NORMAL    [^.,:()\[\]{}\"\' \t\n]
SNORMAL   [^.,:()\[\]{}\"\' \t\n~0-9]
ID        {SNORMAL}{NORMAL}*

%x INNER_STRING
%x INNER_SYMBOL
%x INNER_COMMENT
%%

[-+]?[0-9]+(\.[0-9]+)([eE][-+]?[0-9]+)? {
    yylval.dval = strtod(yytext, NULL);
    return NUMBER;
}

[-+]?[0-9]+([eE][-+]?[0-9]+) {
    yylval.dval = strtod(yytext, NULL);
    return NUMBER;
}

[-+]?[0-9]+ {
    errno = 0;
    yylval.ival = strtol(yytext, NULL, 10);
    if (errno == ERANGE) {
        char* arr = calloc(strlen(yytext) + 1, sizeof(char));
        strcpy(arr, yytext);
        yylval.sval = arr;
        return BIGINT;
    }
    return INTEGER;
}

{ID} {
    char* arr = calloc(strlen(yytext) + 1, sizeof(char));
    strcpy(arr, yytext);
    yylval.sval = arr;
    return NAME;
}

\" { BEGIN(INNER_STRING); clear_buffer(); }
<INNER_STRING>\n { append_buffer(yytext[1]); ++line_num; }
<INNER_STRING>[^\\\"] { append_buffer(yytext[0]); }
<INNER_STRING>\\. { append_buffer(yytext[1]); }
<INNER_STRING>\" { BEGIN(0); yylval.sval = curr_buffer; unset_buffer(); return STRING; }
<INNER_STRING><<EOF>> { yyerror("Unterminated string"); }

\'\( { BEGIN(INNER_SYMBOL); clear_buffer(); }
<INNER_SYMBOL>\n { append_buffer(yytext[1]); ++line_num; }
<INNER_SYMBOL>[^\\\)] {append_buffer(yytext[0]); }
<INNER_SYMBOL>\\. { append_buffer(yytext[1]); }
<INNER_SYMBOL>\) { BEGIN(0); yylval.sval = curr_buffer; unset_buffer(); return SYMBOL; }
<INNER_SYMBOL><<EOF>> { yyerror("Unterminated symbol"); }

\'{NORMAL}+ {
    char* arr = calloc(strlen(yytext), sizeof(char));
    strcpy(arr, yytext + 1);
    yylval.sval = arr;
    return SYMBOL;
}

~{NORMAL}* {
    char* arr = calloc(strlen(yytext) + 1, sizeof(char));
    strcpy(arr, yytext);
    yylval.sval = arr;
    return SYMBOL;
}

\'\[ { return LISTLIT; }

\. { return '.'; }
\( { return '('; }
\) { return ')'; }
\{ { return '{'; }
\} { return '}'; }
: { return ':'; }
:= { return '='; }
, { return ','; }
\[ { return '['; }
\] { return ']'; }

\;[^\n]* ; // Line comments
\{\* { BEGIN(INNER_COMMENT); comments++; }
<INNER_COMMENT>\{\* { comments++; }
<INNER_COMMENT>\*\} { comments--; if (comments == 0) BEGIN(0); }
<INNER_COMMENT>\n { ++line_num; }
<INNER_COMMENT>. ; // Ignore

[ \t] ; // Ignore whitespace
[\n] { ++line_num; }

. { yyerror("Invalid lexical token"); }
