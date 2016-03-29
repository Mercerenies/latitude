
%code top {
    #include <cstdio>
    #include <cstdlib>
    #include <iostream>
    #include <memory>
    #include <string>
    #include "Reader.hpp"
    #ifdef __cplusplus
    extern "C" int yylex();
    extern "C" int yyparse();
    extern "C" FILE *yyin;
    extern "C" int line_num;
    #else
    int yylex();
    int yyparse();
    FILE *yyin;
    int line_num;
    #endif
    void yyerror(const char*);
}

%code requires {

    #ifndef __cplusplus
    typedef char bool;
    #endif
    struct Expr;
    struct List;

    struct Expr {
        struct Expr* lhs;
        char* name;
        struct List* args;
        bool equals;
        struct Expr* rhs;
        bool method;
        bool isNumber;
        double number;
        bool isString;
    };

    struct List {
        struct Expr* car;
        struct List* cdr;
    };

    #ifdef __cplusplus
    extern "C" {
    #endif
        void cleanupE(struct Expr* stmt);
        void cleanupL(struct List* stmt);
        void yyerror(const char*);
    #ifdef __cplusplus
    }
    #endif

}

%union {
    double dval;
    char* sval;
    struct List* argval;
    struct Expr* exprval;
}

%error-verbose

%type <exprval> line
%type <exprval> stmt
%type <exprval> rhs
%type <argval> arglist
%type <argval> arglist1
%type <exprval> arg
%type <exprval> chain
%type <exprval> chain0
%type <exprval> literal
%type <argval> linelist

%token <sval> NAME
%token <dval> NUMBER
%token <sval> STRING

%%

toplevel:
    line { setCurrentLine($1); }
    ;
line:
    stmt '.'
    ;
stmt:
    chain NAME rhs {
        $$ = $3;
        if ($$ == NULL)
            $$ = new Expr();
        $$->name = $2;
        $$->lhs = $1;
    } |
    literal
    ;
rhs:
    /* empty */ { $$ = NULL; } |
    literal { $$ = new Expr(); $$->args = new List();
              $$->args->car = $1; $$->args->cdr = new List(); } |
    '=' stmt { $$ = new Expr(); $$->equals = true; $$->rhs = $2; } |
    ':' arglist { $$ = new Expr(); $$->args = $2; }
    ;
arglist:
    /* empty */ { $$ = new List(); } |
    arg arglist1 { $$ = new List(); $$->car = $1; $$->cdr = $2; }
    ;
arglist1:
    /* empty */ { $$ = new List(); } |
    ',' arg arglist1 { $$ = new List(); $$->car = $2; $$->cdr = $3; }
    ;
arg:
    chain0 |
    literal
    ;
chain:
    chain NAME { $$ = new Expr(); $$->lhs = $1; $$->name = $2; } |
    '(' stmt ')' { $$ = $2; } |
    /* empty */ { $$ = NULL; }
    ;
chain0:
    chain NAME { $$ = new Expr(); $$->lhs = $1; $$->name = $2; } |
    '(' stmt ')' { $$ = $2; }
    ;
literal:
    '{' linelist '}' { $$ = new Expr(); $$->method = true; $$->args = $2; } |
    NUMBER { $$ = new Expr(); $$->isNumber = true; $$->number = $1; } |
    STRING { $$ = new Expr(); $$->isString = true; $$->name = $1; }
    ;
linelist:
    line linelist { $$ = new List(); $$->car = $1; $$->cdr = $2; } |
    /* empty */ { $$ = new List(); }
    ;

%%

extern "C" {

void yyerror(const char* str) {
    std::cout << "Error on line " << line_num << "! " << str << std::endl;
}

void cleanupE(struct Expr* stmt) {
    if (stmt->lhs != 0) {
        cleanupE(stmt->lhs);
        free(stmt->lhs);
    }
    if (stmt->rhs != 0) {
        cleanupE(stmt->rhs);
        free(stmt->rhs);
    }
    if (stmt->args != 0) {
        cleanupL(stmt->args);
        free(stmt->args);
    }
    if (stmt->name != 0)
        free(stmt->name);
}

void cleanupL(struct List* stmt) {
    if (stmt->car != 0) {
        cleanupE(stmt->car);
        free(stmt->car);
    }
    if (stmt->cdr != 0) {
        cleanupL(stmt->cdr);
        free(stmt->cdr);
    }
}

}
