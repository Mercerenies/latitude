//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details


%code top {
    #include <cstdio>
    #include <cstdlib>
    #include <iostream>
    #include <memory>
    #include <string>
    #include "Reader.hpp"
    #include "Standard.hpp"
    #ifdef __cplusplus
    extern "C" {
        int yylex();
        int yyparse();
        extern FILE *yyin;
        extern int line_num;
        char* filename = NULL;
    }
    #else
    int yylex();
    int yyparse();
    extern FILE *yyin;
    char* filename = NULL;
    extern int line_num;
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

    struct complex_t {
        double real;
        double imag;
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
        bool isDict; // Check args (in expressions, check lhs and rhs)
        bool isWrapper; // Check lhs
        bool isOperator; // Like a call, but will use the precedence table
        bool isDummy; // Check nothing (used in operator resolution; should not hit the parser)
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
    struct complex_t cval;
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
%type <argval> shortarglist1
%type <argval> arglist
%type <argval> arglist1
%type <exprval> postarglist
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
%type <exprval> postlitlist
%type <sval> name

%token <sval> STDNAME
%token <sval> OPNAME
%token <dval> NUMBER
%token <ival> INTEGER
%token <sval> BIGINT
%token <cval> COMPLEX
%token <vsval> STRING
%token <vsval> SYMBOL
%token <sval> ZERODISPATCH
%token <sval> HASHQUOTE
%token LISTLIT
%token CEQUALS
%token DCEQUALS
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
                       $$->args->car = $1; $$->args->cdr = makeList(); $$->argsProvided = true;
                       $$->isOperator = true; } |
    shortarglist1 { $$ = makeExpr(); $$->args = $1; $$->argsProvided = true;
                       $$->isOperator = true; } |
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
postarglist:
    arg ARROW arg { $$ = makeExpr(); $$->isDict = true; $$->args = makeList();
                        $$->args->car = makeExpr(); $$->args->car->lhs = $1; $$->args->car->rhs = $3;
                        $$->args->cdr = makeList(); } |
    arg ARROW arg ',' postarglist { $$ = $5; List* temp = $$->args; $$->args = makeList();
                                      $$->args->cdr = temp; $$->args->car = makeExpr();
                                      $$->args->car->lhs = $1; $$->args->car->rhs = $3; } |
    ARROW { $$ = makeExpr(); $$->isDict = true; $$->args = makeList(); }
arg:
    simplechain STDNAME { $$ = makeExpr(); $$->lhs = $1; $$->name = $2; } |
    chain OPNAME verysimplechainl { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                   $$->args = makeList(); $$->args->car = $3;
                                   $$->args->cdr = makeList(); $$->argsProvided = true;
                                   $$->isOperator = true; } |
    simplechain STDNAME shortarglist { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                       $$->args = $3; $$->argsProvided = true; } |
    chain OPNAME shortarglist1 { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                 $$->args = $3; $$->argsProvided = true;
                                 $$->isOperator = true; } |
    literalish
    ;
chain:
    chain OPNAME verysimplechainl { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                   $$->args = makeList(); $$->args->car = $3;
                                   $$->args->cdr = makeList(); $$->argsProvided = true;
                                   $$->isOperator = true; } |
    chain OPNAME shortarglist1 { $$ = makeExpr(); $$->lhs = $1; $$->name = $2;
                                 $$->args = $3; $$->argsProvided = true;
                                 $$->isOperator = true; } |
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
    literal |
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
shortarglist1:
    '(' arglist ')' { $$ = $2; } |
    '(' simplechain STDNAME ':' arg ')' { $$ = makeList(); $$->car = makeExpr();
                                          $$->car->args = makeList(); $$->car->args->car = $5;
                                          $$->car->args->cdr = makeList(); $$->car->name = $3;
                                          $$->car->lhs = $2; $$->cdr = makeList();
                                          $$->car->argsProvided = true; } |
    '(' chain OPNAME ':' arg ')' { $$ = makeList(); $$->car = makeExpr(); $$->car->args = makeList();
                                   $$->car->args->car = $5; $$->car->args->cdr = makeList();
                                   $$->car->name = $3; $$->car->lhs = $2; $$->cdr = makeList();
                                   $$->car->argsProvided = true; }
literalish:
    SYMBOL literalish { if ($1.str[0] != '~') { yyerror("Sigil name must be an interned symbol"); }
                        $$ = makeExpr(); $$->isSigil = true; $$->name = $1.str;
                        $$->namelen = $1.length; $$->rhs = $2; } |
    '(' stmt ')' { $$ = makeExpr(); $$->lhs = $2; $$->isWrapper = true; } |
    literal
    ;
literal:
    '{' linelist '}' { $$ = makeExpr(); $$->isMethod = true; $$->args = $2;
                       $$->argsProvided = true; } |
    COMPLEX { $$ = makeExpr(); $$->number = $1.real; $$->number1 = $1.imag; $$->isComplex = true; } |
    NUMBER { $$ = makeExpr(); $$->isNumber = true; $$->number = $1; } |
    INTEGER { $$ = makeExpr(); $$->isInt = true; $$->integer = $1; } |
    BIGINT { $$ = makeExpr(); $$->isBigInt = true; $$->name = $1; } |
    STRING { $$ = makeExpr(); $$->isString = true; $$->name = $1.str; $$->namelen = $1.length; } |
    SYMBOL { $$ = makeExpr(); $$->isSymbol = true; $$->name = $1.str; $$->namelen = $1.length; } |
    '[' arglist ']' { $$ = makeExpr(); $$->isList = true; $$->args = $2;
                      $$->argsProvided = true; } |
    LISTLIT literallist ']' { $$ = makeExpr(); $$->isList = true; $$->args = $2;
                              $$->argsProvided = true; } |
    LISTLIT postlitlist ']' { $$ = $2; } |
    ZERODISPATCH { $$ = makeExpr(); $$->isZeroDispatch = true;
                   $$->name = $1; } |
    HASHQUOTE name { $$ = makeExpr(); $$->isHashQuote = true; $$->lhs = makeExpr();
                     $$->lhs->lhs = NULL; $$->lhs->name = $2; } |
    HASHQUOTE literalish { $$ = makeExpr(); $$->isHashQuote = true; $$->lhs = $2; } |
    '[' postarglist ']' { $$ = $2; }
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
    '[' postlitlist ']' { $$ = $2; } |
    name { $$ = makeExpr(); $$->isSymbol = true; $$->name = $1; $$->namelen = strlen($1); }
    ;
postlitlist:
    listlit ARROW listlit { $$ = makeExpr(); $$->isDict = true; $$->args = makeList();
                        $$->args->car = makeExpr(); $$->args->car->lhs = $1; $$->args->car->rhs = $3;
                        $$->args->cdr = makeList(); } |
    listlit ARROW listlit ',' postlitlist { $$ = $5; List* temp = $$->args; $$->args = makeList();
                                      $$->args->cdr = temp; $$->args->car = makeExpr();
                                      $$->args->car->lhs = $1; $$->args->car->rhs = $3; } |
    ARROW { $$ = makeExpr(); $$->isDict = true; $$->args = makeList(); }
name:
    STDNAME |
    OPNAME

%%

extern "C" {

void yyerror(const char* str) {
    std::ostringstream oss;
    oss << "Error in file " << filename << " on line " << line_num << "! " << str;
    throw ParseError(oss.str());
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
    // TODO This +1 correction seems to be only necessary in specifically repl.lats... :/
    //expr->line = line_num + 1;
    expr->line = line_num;
    return expr;
}

struct List* makeList() {
    return new List();
}

}
