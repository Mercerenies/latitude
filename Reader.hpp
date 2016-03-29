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

ObjectPtr callMethod(ObjectPtr result, Method& mthd, ObjectPtr dyn);

std::unique_ptr<Stmt> parse(std::string str);

ObjectPtr eval(std::string str, ObjectPtr lex, ObjectPtr dyn);

class Stmt {
public:
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn) = 0;
};

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

class StmtMethod : public Stmt {
private:
    std::list< std::shared_ptr<Stmt> > contents;
public:
    StmtMethod(std::list< std::shared_ptr<Stmt> >& contents);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

class StmtNumber : public Stmt {
private:
    double value;
public:
    StmtNumber(double value);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

class StmtString : public Stmt {
private:
    std::string value;
public:
    StmtString(const char* contents);
    virtual ObjectPtr execute(ObjectPtr lex, ObjectPtr dyn);
};

#endif // _READER_HPP_
