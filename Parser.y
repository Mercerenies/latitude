
%code top {
    #include <cstdio>
    #include <cstdlib>
    #include <iostream>
    #include <memory>
    #include <string>
    #include "Reader.hpp"
    #include "Standard.hpp"
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
        int line;
        struct Expr* lhs;
        char* name;
        struct List* args;
        bool equals;
        struct Expr* rhs;
        bool isMethod; // Check args
        bool isNumber; // Check number
        double number;
        double number1;
        bool isInt; // Check integer
        long integer;
        bool isBigInt; // Check name
        bool isString; // Check name
        bool isSymbol; // Check name
        bool isList; // Check args
        bool isSigil; // Check name and rhs
        bool isEquality; // Treated like call with a rhs
        bool isHashParen; // Check name
        bool isZeroDispatch; // Check name
        bool isSpecialMethod; // Just like isMethod
        bool isBind; // Treated like call with a rhs
        bool isComplex; // Check number and number1
        bool equals2; // ::= (`a b ::= c.` desugars to `a b := c. a b :: 'b.`)
        bool isHashQuote; // Check name
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
        struct Expr* makeExpr();
        struct List* makeList();
    #ifdef __cplusplus
    }
    #endif

}

%union {
    long ival;
    double dval;
    char* sval;
    struct List* argval;
    struct Expr* exprval;
}

%error-verbose

%type <argval> lines
%type <exprval> line
%type <exprval> stmt
%type <exprval> rhs
%type <argval> shortarglist
%type <argval> arglist
%type <argval> arglist1
%type <exprval> arg
%type <exprval> chain
%type <exprval> nonemptychain
%type <exprval> literalish
%type <exprval> literal
%type <argval> linelist
%type <argval> literallist
%type <argval> literallist1
%type <exprval> listlit
%type <dval> doublish

%token <sval> NAME
%token <dval> NUMBER
%token <ival> INTEGER
%token <sval> BIGINT
%token <sval> STRING
%token <sval> SYMBOL
%token <sval> HASHPAREN
%token <sval> ZERODISPATCH
%token <sval> HASHQUOTE
%token LISTLIT
%token CEQUALS
%token DCEQUALS
%token ATBRACE
%token ATPAREN
%token BIND

%%

toplevel:
    lines { setCurrentLine($1); }
    ;
lines:
    line lines { $$ = makeList(); $$->car = $1; $$->cdr = $2; } |
    /* empty */ { $$ = makeList(); }
line:
    stmt '.'
    ;
stmt:
    chain NAME rhs {
        $$ = $3;
        if ($$ == NULL)
            $$ = makeExpr();
        $$->name = $2;
        $$->lhs = $1;
    } |
    literalish
    ;
rhs:
    /* empty */ { $$ = NULL; } |
    shortarglist { $$ = makeExpr(); $$->args = $1; } |
    CEQUALS stmt { $$ = makeExpr(); $$->equals = true; $$->rhs = $2; } |
    DCEQUALS stmt { $$ = makeExpr(); $$->equals2 = true; $$->rhs = $2; } |
    ':' arglist { $$ = makeExpr(); $$->args = $2; } |
    '=' stmt { $$ = makeExpr(); $$->rhs = $2; $$->isEquality = true; } |
    shortarglist '=' stmt { $$ = makeExpr(); $$->rhs = $3; $$->args = $1; $$->isEquality = true; } |
    BIND stmt { $$ = makeExpr(); $$->rhs = $2; $$->isBind = true; }
    ;
arglist:
    /* empty */ { $$ = makeList(); } |
    arg arglist1 { $$ = makeList(); $$->car = $1; $$->cdr = $2; }
    ;
arglist1:
    /* empty */ { $$ = makeList(); } |
    ',' arg arglist1 { $$ = makeList(); $$->car = $2; $$->cdr = $3; }
    ;
arg:
    nonemptychain
    ;
nonemptychain:
    chain NAME { $$ = makeExpr(); $$->lhs = $1; $$->name = $2; } |
    chain NAME shortarglist { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                              $$->args = $3; } |
    literalish
    ;
chain:
    nonemptychain |
    /* empty */ { $$ = NULL; }
    ;
shortarglist:
    '(' arglist ')' { $$ = $2; } |
    '(' chain NAME ':' arg ')' { $$ = makeList(); $$->car = makeExpr(); $$->car->args = makeList();
                                 $$->car->args->car = $5; $$->car->args->cdr = makeList();
                                 $$->car->name = $3; $$->car->lhs = $2; $$->cdr = makeList(); } |
     literal { $$ = makeList(); $$->car = $1; $$->cdr = makeList(); }
     ;
literalish:
    SYMBOL literalish { if (*$1 != '~') { yyerror("Sigil name must be an interned symbol"); }
                        $$ = makeExpr(); $$->isSigil = true; $$->name = $1; $$->rhs = $2; } |
    '(' stmt ')' { $$ = $2; } |
    literal
    ;
literal:
    '{' linelist '}' { $$ = makeExpr(); $$->isMethod = true; $$->args = $2; } |
    ATBRACE linelist '}' { $$ = makeExpr(); $$->isSpecialMethod = true; $$->args = $2; } |
    ATPAREN doublish ',' doublish ')' { $$ = makeExpr(); $$->number = $2;
                                        $$->number1 = $4; $$->isComplex = true;  } |
    NUMBER { $$ = makeExpr(); $$->isNumber = true; $$->number = $1; } |
    INTEGER { $$ = makeExpr(); $$->isInt = true; $$->integer = $1; } |
    BIGINT { $$ = makeExpr(); $$->isBigInt = true; $$->name = $1; } |
    STRING { $$ = makeExpr(); $$->isString = true; $$->name = $1; } |
    SYMBOL { $$ = makeExpr(); $$->isSymbol = true; $$->name = $1; } |
    '[' arglist ']' { $$ = makeExpr(); $$->isList = true; $$->args = $2; } |
    LISTLIT literallist ']' { $$ = makeExpr(); $$->isList = true; $$->args = $2; } |
    HASHPAREN { $$ = makeExpr(); $$->isHashParen = true; $$->name = $1; } |
    ZERODISPATCH { $$ = makeExpr(); $$->isZeroDispatch = true;
                   $$->name = $1; } |
    HASHQUOTE { $$ = makeExpr(); $$->isHashQuote = true; $$->name = $1; }
    ;
linelist:
    line linelist { $$ = makeList(); $$->car = $1; $$->cdr = $2; } |
    /* empty */ { $$ = makeList(); }
    ;
literallist:
    /* empty */ { $$ = makeList(); } |
    listlit literallist1 { $$ = makeList(); $$->car = $1; $$->cdr = $2; }
    ;
literallist1:
    /* empty */ { $$ = makeList(); } |
    ',' listlit literallist1 { $$ = makeList(); $$->car = $2; $$->cdr = $3; }
    ;
listlit:
    STRING { $$ = makeExpr(); $$->isString = true; $$->name = $1; } |
    SYMBOL { $$ = makeExpr(); $$->isSymbol = true; $$->name = $1; } |
    NUMBER { $$ = makeExpr(); $$->isNumber = true; $$->number = $1; } |
    INTEGER { $$ = makeExpr(); $$->isInt = true; $$->integer = $1; } |
    BIGINT { $$ = makeExpr(); $$->isBigInt = true; $$->name = $1; } |
    '[' literallist ']' { $$ = makeExpr(); $$->isList = true; $$->args = $2; } |
    NAME { $$ = makeExpr(); $$->isSymbol = true; $$->name = $1; }
    ;
doublish:
    NUMBER |
    INTEGER { $$ = (double)$1; } |
    BIGINT { $$ = strtod($1, NULL); }
    ;

%%

extern "C" {

void yyerror(const char* str) {
    std::ostringstream oss;
    oss << "Error on line " << line_num << "! " << str;
    throw oss.str();
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

struct Expr* makeExpr() {
    struct Expr* expr = new Expr();
    // TODO No idea why this '+ 1' is necessary but it makes things work right
    expr->line = line_num + 1;
    return expr;
}

struct List* makeList() {
    return new List();
}

}
