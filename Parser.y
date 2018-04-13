
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

    struct vstring_t {
        char* str;
        long length;
    };

    struct Expr {
        int line;
        struct Expr* lhs;
        char* name;
        long namelen;
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
        bool isZeroDispatch; // Check name
        bool isSpecialMethod; // Just like isMethod
        bool isBind; // Treated like call with a rhs
        bool isComplex; // Check number and number1
        bool equals2; // ::= (`a b ::= c.` desugars to `a b := c. a b :: 'b.`)
        bool isHashQuote; // Check lhs
        bool argsProvided;
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
    struct vstring_t vsval;
    struct List* argval;
    struct Expr* exprval;
}

%error-verbose

%type <argval> lines
%type <exprval> line
%type <exprval> stmt
%type <exprval> rhs
%type <exprval> rhs1
%type <argval> shortarglist
%type <argval> arglist
%type <argval> arglist1
%type <exprval> arg
%type <exprval> chain
%type <exprval> simplechain
%type <exprval> verysimplechain
%type <exprval> verysimplechainl
%type <exprval> literalish
%type <exprval> literal
%type <argval> linelist
%type <argval> literallist
%type <argval> literallist1
%type <exprval> listlit
%type <dval> doublish
%type <sval> name

%token <sval> STDNAME
%token <sval> OPNAME
%token <dval> NUMBER
%token <ival> INTEGER
%token <sval> BIGINT
%token <vsval> STRING
%token <vsval> SYMBOL
%token <sval> ZERODISPATCH
%token <sval> HASHQUOTE
%token LISTLIT
%token CEQUALS
%token DCEQUALS
%token ATPAREN
%token BIND
%token ARROW

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
    simplechain STDNAME rhs {
        $$ = $3;
        if ($$ == NULL)
            $$ = makeExpr();
        $$->name = $2;
        $$->lhs = $1;
    } |
    chain OPNAME rhs1 {
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
    shortarglist { $$ = makeExpr(); $$->args = $1; $$->argsProvided = true; } |
    CEQUALS stmt { $$ = makeExpr(); $$->equals = true; $$->rhs = $2; } |
    DCEQUALS stmt { $$ = makeExpr(); $$->equals2 = true; $$->rhs = $2; } |
    ':' arglist { $$ = makeExpr(); $$->args = $2; $$->argsProvided = true; } |
    '=' stmt { $$ = makeExpr(); $$->rhs = $2; $$->isEquality = true; } |
    shortarglist '=' stmt { $$ = makeExpr(); $$->rhs = $3; $$->args = $1; $$->isEquality = true; $$->argsProvided = true; } |
    BIND stmt { $$ = makeExpr(); $$->rhs = $2; $$->isBind = true; }
    ;
rhs1:
    verysimplechainl { $$ = makeExpr(); $$->args = makeList(); $$->args->car = makeExpr();
                       $$->args->car = $1; $$->args->cdr = makeList(); $$->argsProvided = true; } |
    shortarglist { $$ = makeExpr(); $$->args = $1; $$->argsProvided = true; } |
    CEQUALS stmt { $$ = makeExpr(); $$->equals = true; $$->rhs = $2; } |
    DCEQUALS stmt { $$ = makeExpr(); $$->equals2 = true; $$->rhs = $2; } |
    ':' arglist { $$ = makeExpr(); $$->args = $2; $$->argsProvided = true; } |
    '=' stmt { $$ = makeExpr(); $$->rhs = $2; $$->isEquality = true; } |
    shortarglist '=' stmt { $$ = makeExpr(); $$->rhs = $3; $$->args = $1; $$->isEquality = true;
                            $$->argsProvided = true; } |
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
    simplechain STDNAME { $$ = makeExpr(); $$->lhs = $1; $$->name = $2; } |
    chain OPNAME verysimplechainl { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                   $$->args = makeList(); $$->args->car = $3;
                                   $$->args->cdr = makeList(); $$->argsProvided = true; } |
    simplechain STDNAME shortarglist { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                       $$->args = $3; $$->argsProvided = true; } |
    chain OPNAME shortarglist { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                $$->args = $3; $$->argsProvided = true; } |
    literalish
    ;
chain:
    chain OPNAME verysimplechainl { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                   $$->args = makeList(); $$->args->car = $3;
                                   $$->args->cdr = makeList(); $$->argsProvided = true; } |
    chain OPNAME shortarglist { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                $$->args = $3; $$->argsProvided = true; } |
    simplechain
    ;
simplechain:
    simplechain STDNAME { $$ = makeExpr(); $$->lhs = $1; $$->name = $2; } |
    simplechain STDNAME shortarglist { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                       $$->args = $3; $$->argsProvided = true; } |
    literalish |
    /* empty */ { $$ = NULL; }
verysimplechain:
    verysimplechainl STDNAME { $$ = makeExpr(); $$->lhs = $1; $$->name = $2; } |
    verysimplechainl STDNAME shortarglist { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                            $$->args = $3; $$->argsProvided = true; }
    ;
verysimplechainl:
    verysimplechain |
    /* empty */ { $$ = NULL; }
    ;
shortarglist:
    '(' arglist ')' { $$ = $2; } |
    '(' simplechain STDNAME ':' arg ')' { $$ = makeList(); $$->car = makeExpr();
                                          $$->car->args = makeList(); $$->car->args->car = $5;
                                          $$->car->args->cdr = makeList(); $$->car->name = $3;
                                          $$->car->lhs = $2; $$->cdr = makeList();
                                          $$->car->argsProvided = true; } |
    '(' chain OPNAME ':' arg ')' { $$ = makeList(); $$->car = makeExpr(); $$->car->args = makeList();
                                   $$->car->args->car = $5; $$->car->args->cdr = makeList();
                                   $$->car->name = $3; $$->car->lhs = $2; $$->cdr = makeList();
                                   $$->car->argsProvided = true; } |
     literal { $$ = makeList(); $$->car = $1; $$->cdr = makeList(); }
     ;
literalish:
    SYMBOL literalish { if ($1.str[0] != '~') { yyerror("Sigil name must be an interned symbol"); }
                        $$ = makeExpr(); $$->isSigil = true; $$->name = $1.str;
                        $$->namelen = $1.length; $$->rhs = $2; } |
    '(' stmt ')' { $$ = $2; } |
    literal
    ;
literal:
    '{' linelist '}' { $$ = makeExpr(); $$->isMethod = true; $$->args = $2;
                       $$->argsProvided = true; } |
    ATPAREN doublish ',' doublish ')' { $$ = makeExpr(); $$->number = $2;
                                        $$->number1 = $4; $$->isComplex = true;  } |
    NUMBER { $$ = makeExpr(); $$->isNumber = true; $$->number = $1; } |
    INTEGER { $$ = makeExpr(); $$->isInt = true; $$->integer = $1; } |
    BIGINT { $$ = makeExpr(); $$->isBigInt = true; $$->name = $1; } |
    STRING { $$ = makeExpr(); $$->isString = true; $$->name = $1.str; $$->namelen = $1.length; } |
    SYMBOL { $$ = makeExpr(); $$->isSymbol = true; $$->name = $1.str; $$->namelen = $1.length; } |
    '[' arglist ']' { $$ = makeExpr(); $$->isList = true; $$->args = $2;
                      $$->argsProvided = true; } |
    LISTLIT literallist ']' { $$ = makeExpr(); $$->isList = true; $$->args = $2;
                              $$->argsProvided = true; } |
    ZERODISPATCH { $$ = makeExpr(); $$->isZeroDispatch = true;
                   $$->name = $1; } |
    HASHQUOTE name { $$ = makeExpr(); $$->isHashQuote = true; $$->lhs = makeExpr();
                     $$->lhs->lhs = NULL; $$->lhs->name = $2; } |
    HASHQUOTE literalish { $$ = makeExpr(); $$->isHashQuote = true; $$->lhs = $2; }
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
    STRING { $$ = makeExpr(); $$->isString = true; $$->name = $1.str; $$->namelen = $1.length; } |
    SYMBOL { $$ = makeExpr(); $$->isSymbol = true; $$->name = $1.str; $$->namelen = $1.length; } |
    NUMBER { $$ = makeExpr(); $$->isNumber = true; $$->number = $1; } |
    INTEGER { $$ = makeExpr(); $$->isInt = true; $$->integer = $1; } |
    BIGINT { $$ = makeExpr(); $$->isBigInt = true; $$->name = $1; } |
    '[' literallist ']' { $$ = makeExpr(); $$->isList = true; $$->args = $2;
                          $$->argsProvided = true; } |
    name { $$ = makeExpr(); $$->isSymbol = true; $$->name = $1; $$->namelen = strlen($1); }
    ;
doublish:
    NUMBER |
    INTEGER { $$ = (double)$1; } |
    BIGINT { $$ = strtod($1, NULL); }
    ;
name:
    STDNAME |
    OPNAME

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
