
%option noyywrap
%option noinput
%option nounput
%option header-file="lex.yy.h"

%{
     #include "Parser.tab.h"
     #ifdef __cplusplus
     #include <cstdlib>
     #include <cstring>
     #else
     #include <stdlib.h>
     #include <string.h>
     #endif
     int line_num = 1;
     int comments = 0;
     int hash_parens = 0;
     const char* ops = "!@#%^&*-+/<=>?\\`|~";
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
     int id_classify(char* arr) {
         // TODO We would like to support Unicode in operator names at some point...
         for (int i = 0; arr[i] != 0; i++) {
             if (strchr(ops, arr[i]) == NULL)
               return STDNAME;
         }
         return OPNAME;
     }

     // TODO Should backslashes be allowed in identifiers or should they be "special" characters?

%}

NORMAL    [^.,:;()\[\]{}\"\' \t\n\r]
SNORMAL   [^.,:;()\[\]{}\"\' \t\n\r~0-9#@]
ID        {SNORMAL}{NORMAL}*

%x INNER_STRING
%x INNER_SYMBOL
%x INNER_COMMENT
%x INNER_RSTRING
%x INNER_HASHPAREN
%%

#< { yyerror("Unreadable object"); }
= { return '='; }
\<- { return BIND; }
::= { return DCEQUALS; }

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
    if ((errno == ERANGE) || (yylval.ival > 0xFFFFFFFF) || (yylval.ival < -0xFFFFFFFF)) {
        char* arr = calloc(strlen(yytext) + 1, sizeof(char));
        strcpy(arr, yytext);
        yylval.sval = arr;
        return BIGINT;
    }
    return INTEGER;
}

[-+]?0[A-Za-z][0-9A-Za-z]+ {
    char* arr = calloc(strlen(yytext) + 1, sizeof(char));
    strcpy(arr, yytext);
    yylval.sval = arr;
    return ZERODISPATCH;
}

{ID} {
    char* arr = calloc(strlen(yytext) + 1, sizeof(char));
    strcpy(arr, yytext);
    yylval.sval = arr;
    return id_classify(yylval.sval);
}

\.\.\. { // Ellipsis is a valid identifier name
    char* arr = calloc(4, sizeof(char));
    strcpy(arr, "...");
    yylval.sval = arr;
    return STDNAME;
}

:: { // Double-colon, like ellipsis, is a valid identifier name
   char* arr = calloc(3, sizeof(char));
   strcpy(arr, "::");
   yylval.sval = arr;
   return STDNAME;
}

\" { BEGIN(INNER_STRING); clear_buffer(); }
<INNER_STRING>\n { append_buffer(yytext[0]); ++line_num; }
<INNER_STRING>[^\\\"] { append_buffer(yytext[0]); }
<INNER_STRING>\\n { append_buffer((char)0x0A); }
<INNER_STRING>\\r { append_buffer((char)0x0D); }
<INNER_STRING>\\t { append_buffer((char)0x09); }
<INNER_STRING>\\a { append_buffer((char)0x07); }
<INNER_STRING>\\b { append_buffer((char)0x08); }
<INNER_STRING>\\f { append_buffer((char)0x0C); }
<INNER_STRING>\\v { append_buffer((char)0x0B); }
<INNER_STRING>\\. { append_buffer(yytext[1]); }
<INNER_STRING>\" { BEGIN(0); yylval.sval = curr_buffer; unset_buffer(); return STRING; }
<INNER_STRING><<EOF>> { yyerror("Unterminated string"); }

\'\( { BEGIN(INNER_SYMBOL); clear_buffer(); }
<INNER_SYMBOL>\n { append_buffer(yytext[1]); ++line_num; }
<INNER_SYMBOL>[^\\\)] {append_buffer(yytext[0]); }
<INNER_SYMBOL>\\. { append_buffer(yytext[1]); }
<INNER_SYMBOL>\) { BEGIN(0); yylval.sval = curr_buffer; unset_buffer(); return SYMBOL; }
<INNER_SYMBOL><<EOF>> { yyerror("Unterminated symbol"); }

#\" { BEGIN(INNER_RSTRING); clear_buffer(); }
<INNER_RSTRING>\n { append_buffer(yytext[0]); ++line_num; }
<INNER_RSTRING>\"# { BEGIN(0); yylval.sval = curr_buffer; unset_buffer(); return STRING; }
<INNER_RSTRING>. { append_buffer(yytext[0]); }
<INNER_RSTRING><<EOF>> { yyerror("Unterminated string"); }

#\( { BEGIN(INNER_HASHPAREN); clear_buffer(); hash_parens++; }
<INNER_HASHPAREN>\( { append_buffer(yytext[0]); hash_parens++; }
<INNER_HASHPAREN>\) { hash_parens--;
                      if (hash_parens == 0) {
                          BEGIN(0);
                          yylval.sval = curr_buffer;
                          unset_buffer();
                          return HASHPAREN;
                      } else {
                          append_buffer(yytext[0]);
                      } }
<INNER_HASHPAREN>\n { append_buffer(yytext[0]); ++line_num; }
<INNER_HASHPAREN>. { append_buffer(yytext[0]); } // Ignore

\'{NORMAL}+ {
    char* arr = calloc(strlen(yytext), sizeof(char));
    strcpy(arr, yytext + 1);
    yylval.sval = arr;
    return SYMBOL;
}

~{NORMAL}+ {
    char* arr = calloc(strlen(yytext) + 1, sizeof(char));
    strcpy(arr, yytext);
    yylval.sval = arr;
    return SYMBOL;
}

#\'{NORMAL}+ {
    char* arr = calloc(strlen(yytext) - 1, sizeof(char));
    strcpy(arr, yytext + 2);
    yylval.sval = arr;
    return HASHQUOTE;
}

\'\[ { return LISTLIT; }

\. { return '.'; }
\( { return '('; }
\) { return ')'; }
\{ { return '{'; }
\} { return '}'; }
: { return ':'; }
:= { return CEQUALS; }
, { return ','; }
\[ { return '['; }
\] { return ']'; }
@\{ { return ATBRACE; }
@\( { return ATPAREN; }

\;[^\n]* ; // Line comments
\{\* { BEGIN(INNER_COMMENT); comments++; }
<INNER_COMMENT>\{\* { comments++; }
<INNER_COMMENT>\*\} { comments--; if (comments == 0) BEGIN(0); }
<INNER_COMMENT>\n { ++line_num; }
<INNER_COMMENT>. ; // Ignore

[ \t\r] ; // Ignore whitespace
[\n] { ++line_num; }

. { yyerror("Invalid lexical token"); }
