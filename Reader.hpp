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
    void operator()(List* x);
};

struct Scope {
    ObjectPtr lex;
    ObjectPtr dyn;
};

using PtrToExpr = std::unique_ptr< Expr, ExprDeleter >;
using PtrToList = std::unique_ptr< List, ExprDeleter >;

class Stmt;

extern "C" void setCurrentLine(List* stmt);
PtrToList getCurrentLine();
std::list< std::unique_ptr<Stmt> > translateCurrentLine();
void clearCurrentLine();

ObjectPtr doCall(Scope scope,
                 ObjectPtr self, ObjectPtr mthd,
                 std::list<ObjectPtr> args,
                 std::function<void(ObjectPtr&)> dynCall = [](ObjectPtr&){});

ObjectPtr determineScope(const std::unique_ptr<Stmt>& className,
                         const std::string& functionName,
                         const Scope& scope);

int grabLineNumber(const Scope& scope, ObjectPtr dyn);

std::string grabFileName(const Scope& scope, ObjectPtr dyn);

/*
 * Parses the string. Will throw an std::string as an exception
 * if a parse error occurs. For this reason, it is often desirable
 * to use `eval`, which captures these parse errors and rethrows them
 * as `ProtoError` objects.
 */
std::list< std::unique_ptr<Stmt> > parse(std::string str);

/*
 * Parses and evaluates the expression in the given context.
 */
ObjectPtr eval(std::string str, Scope scope);

/*
 * Parses and evaluates the file, which should be a source file. Note that lexDef and dynDef should almost
 * always be the global scope. The file will be treated as a method who was defined in the lexDef / dynDef
 * scope and called from the lex / dyn scope. The latter should be the "current" scope.
 */
ObjectPtr eval(std::istream& file, std::string fname, Scope defScope, Scope scope);

/*
 * A statement. Defines only one method, which executes
 * the statement in a given context.
 */
class Stmt {
private:
    std::string file_name;
    int line_no;
public:
    Stmt(int line_no);
    void establishLocation(const Scope& scope);
    virtual ObjectPtr execute(Scope scope) = 0;
    virtual void propogateFileName(std::string name);
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
    StmtCall(int line_no, std::unique_ptr<Stmt>& cls, const std::string& func, ArgList& arg);
    virtual ObjectPtr execute(Scope scope);
    virtual void propogateFileName(std::string name);
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
    StmtEqual(int line_no,
              std::unique_ptr<Stmt>& cls, const std::string& func,
              std::unique_ptr<Stmt>& asn);
    virtual ObjectPtr execute(Scope scope);
    virtual void propogateFileName(std::string name);
};

/*
 * A method literal.
 */
class StmtMethod : public Stmt {
private:
    std::list< std::shared_ptr<Stmt> > contents;
public:
    StmtMethod(int line_no, std::list< std::shared_ptr<Stmt> >& contents);
    virtual ObjectPtr execute(Scope scope);
    virtual void propogateFileName(std::string name);
};

/*
 * A numeric literal.
 */
class StmtNumber : public Stmt {
private:
    double value;
public:
    StmtNumber(int line_no, double value);
    virtual ObjectPtr execute(Scope scope);
};

/*
 * A numeric integer literal.
 */
class StmtInteger : public Stmt {
private:
    long value;
public:
    StmtInteger(int line_no, long value);
    virtual ObjectPtr execute(Scope scope);
};

/*
 * A numeric integer literal.
 */
class StmtBigInteger : public Stmt {
private:
    std::string value;
public:
    StmtBigInteger(int line_no, const char* value);
    virtual ObjectPtr execute(Scope scope);
};

/*
 * A string literal.
 */
class StmtString : public Stmt {
private:
    std::string value;
public:
    StmtString(int line_no, const char* contents);
    virtual ObjectPtr execute(Scope scope);
};

/*
 * A symbolic literal.
 */
class StmtSymbol : public Stmt {
private:
    std::string value;
public:
    StmtSymbol(int line_no, const char* contents);
    virtual ObjectPtr execute(Scope scope);
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
    StmtList(int line_no, ArgList& arg);
    virtual ObjectPtr execute(Scope scope);
    virtual void propogateFileName(std::string name);
};

/*
 * A sigil, syntax sugar for some function calls
 */
class StmtSigil : public Stmt {
private:
    std::string name;
    std::unique_ptr<Stmt> rhs;
public:
    StmtSigil(int line_no, std::string name, std::unique_ptr<Stmt> rhs);
    virtual ObjectPtr execute(Scope scope);
    virtual void propogateFileName(std::string name);
};

#endif // _READER_HPP_
