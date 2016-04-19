#ifndef _READER_HPP_
#define _READER_HPP_
extern "C" {
#include "Parser.tab.h"
}
#include "Proto.hpp"
#include <memory>
#include <functional>
#include <list>

class ExprDeleter {
public:
    void operator()(Expr* x);
};

using PtrToExpr = std::unique_ptr< Expr, ExprDeleter >;

class Stmt;

extern "C" void setCurrentLine(Expr* stmt);
PtrToExpr getCurrentLine();
std::unique_ptr<Stmt> translateCurrentLine();
void clearCurrentLine();

ObjectPtr callMethod(ObjectSPtr self, ObjectPtr mthd, ObjectPtr dyn);

/*
 * Parses the string. Will throw an std::string as an exception
 * if a parse error occurs. For this reason, it is often desirable
 * to use `eval`, which captures these parse errors and rethrows them
 * as `ProtoError` objects.
 */
std::unique_ptr<Stmt> parse(std::string str);

/*
 * Parses and evaluates the expression in the given context.
 */
ObjectPtr eval(std::string str, ObjectPtr lex, ObjectPtr dyn);

/*
 * A statement. Defines only one method, which executes
 * the statement in a given lexical and dynamic context.
 */
class Stmt {
public:
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn) = 0;
};

/*
 * A slot access statement, which may or may not be a method call.
 */
class StmtCall : public Stmt {
public:
    typedef std::list< std::unique_ptr<Stmt> > ArgList;
private:
    std::unique_ptr<Stmt> className;
    std::string functionName;
    ArgList args;
public:
    StmtCall(std::unique_ptr<Stmt>& cls, const std::string& func, ArgList& arg);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

/*
 * An assignment statement. Note that the `className` field defaults to the current
 * scope (lexical or dynamic, depending on the name of the variable) if it is not
 * provided.
 */
class StmtEqual : public Stmt {
private:
    std::unique_ptr<Stmt> className;
    std::string functionName;
    std::unique_ptr<Stmt> rhs;
public:
    StmtEqual(std::unique_ptr<Stmt>& cls, const std::string& func,
              std::unique_ptr<Stmt>& asn);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

/*
 * A method literal.
 */
class StmtMethod : public Stmt {
private:
    std::list< std::shared_ptr<Stmt> > contents;
public:
    StmtMethod(std::list< std::shared_ptr<Stmt> >& contents);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

/*
 * A numeric literal.
 */
class StmtNumber : public Stmt {
private:
    double value;
public:
    StmtNumber(double value);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

/*
 * A numeric integer literal.
 */
class StmtInteger : public Stmt {
private:
    long value;
public:
    StmtInteger(long value);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

/*
 * A numeric integer literal.
 */
class StmtBigInteger : public Stmt {
private:
    std::string value;
public:
    StmtBigInteger(const char* value);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

/*
 * A string literal.
 */
class StmtString : public Stmt {
private:
    std::string value;
public:
    StmtString(const char* contents);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

/*
 * A symbolic literal.
 */
class StmtSymbol : public Stmt {
private:
    std::string value;
public:
    StmtSymbol(const char* contents);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

/*
 * An inline list, which by default evaluates to an Array object.
 */
class StmtList : public Stmt {
public:
    typedef std::list< std::unique_ptr<Stmt> > ArgList;
private:
    ArgList args;
public:
    StmtList(ArgList& arg);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

#endif // _READER_HPP_
