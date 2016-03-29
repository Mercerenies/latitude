extern "C" {
#include "lex.yy.h"
}
#include "Reader.hpp"
#include <list>
#include <memory>
#include <algorithm>
#include <sstream>

//#define PRINT_BEFORE_EXEC

using namespace std;

PtrToExpr currentLine;

void ExprDeleter::operator()(Expr* x) {
    cleanupE(x);
}

extern "C" {
    void setCurrentLine(Expr* stmt) {
        currentLine.reset(stmt);
    }
}

PtrToExpr getCurrentLine() {
    return std::move(currentLine);
}

unique_ptr<Stmt> translateStmt(Expr* expr);
list< unique_ptr<Stmt> > translateList(List* list);

unique_ptr<Stmt> translateStmt(Expr* expr) {
    if (expr->isString) {
        return unique_ptr<Stmt>(new StmtString(expr->name));
    } else if (expr->isNumber) {
        return unique_ptr<Stmt>(new StmtNumber(expr->number));
    } else if (expr->method) {
        auto contents0 = translateList(expr->args);
        list< shared_ptr<Stmt> > contents1( contents0.size() );
        transform(contents0.begin(), contents0.end(), contents1.begin(),
                  [](auto& cc) { return move(cc); });
        return unique_ptr<Stmt>(new StmtMethod(contents1));
    } else if (expr->equals) {
        auto rhs = translateStmt(expr->rhs);
        auto func = expr->name;
        auto lhs = expr->lhs ? translateStmt(expr->lhs) : unique_ptr<Stmt>();
        return unique_ptr<Stmt>(new StmtEqual(lhs, func, rhs));
    } else {
        auto args = expr->args ? translateList(expr->args) : list< unique_ptr<Stmt> >();
        auto func = expr->name;
        auto lhs = expr->lhs ? translateStmt(expr->lhs) : unique_ptr<Stmt>();
        return unique_ptr<Stmt>(new StmtCall(lhs, func, args));
    }
}

list< unique_ptr<Stmt> > translateList(List* lst) {
    if ((lst->car == nullptr) || (lst->cdr == nullptr)) {
        return list< unique_ptr<Stmt> >();
    } else {
        auto head = translateStmt(lst->car);
        auto tail = translateList(lst->cdr);
        tail.push_front(move(head));
        return tail;
    }
}

unique_ptr<Stmt> translateCurrentLine() {
    if (currentLine)
        return translateStmt(currentLine.get());
    else
        return unique_ptr<Stmt>();
}

void clearCurrentLine() {
    currentLine.reset();
}

ObjectPtr callMethod(ObjectPtr result, Method& mthd, ObjectPtr dyn) {
    for (auto stmt : mthd.code)
        result = stmt->execute(mthd.lexicalScope, dyn);
    return result;
}

std::unique_ptr<Stmt> parse(std::string str) {
    const char* buffer = str.c_str();
    auto curr = yy_scan_string(buffer);
    yyparse();
    yy_delete_buffer(curr);
    auto result = translateCurrentLine();
    clearCurrentLine();
    return result;
}

ObjectPtr eval(string str, ObjectPtr lex, ObjectPtr dyn) {
    auto result = parse(str);
    if (result)
        return result->execute(lex, dyn);
    else
        return ObjectPtr(); // TODO Better error handling
}

StmtCall::StmtCall(unique_ptr<Stmt>& cls, const string& func, ArgList& arg)
    : className(move(cls)), functionName(func), args(move(arg)) {}

ObjectPtr StmtCall::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr scope = lex;
    if (className) {
        // If a class was provided, use that as the "scope"
        scope = className->execute(lex, dyn);
    } else {
        // Otherwise, use lexical scope by default, or dynamic
        // scope if the name starts with $
        if ((functionName.length() > 0) && (functionName[0] == '$'))
            scope = dyn;
    }
    list<ObjectPtr> parms( args.size() );
    transform(args.begin(), args.end(), parms.begin(),
              [&lex, &dyn](std::unique_ptr<Stmt>& arg) {
                  return arg->execute(lex, dyn);
              });
    ObjectPtr target = getInheritedSlot(scope, functionName);
    auto prim = target ? target->prim() : boost::blank();
    if (target == NULL) {
        // Could not find slot
        // TODO Better error handling
#ifdef PRINT_BEFORE_EXEC
        cout << "No slot " << functionName << endl;
#endif
        return target;
    } else if (auto sys = boost::get<SystemCall>(&prim)) {
        // System call slot
#ifdef PRINT_BEFORE_EXEC
        cout << "Sys" << endl;
#endif
        return (*sys)(parms);
    } else if (auto function = boost::get<Method>(&prim)) {
        // Standard method call
#ifdef PRINT_BEFORE_EXEC
        cout << "Func" << endl;
#endif
        ObjectPtr result(getInheritedSlot(meta(scope), "Nil"));
        ObjectPtr dyn1 = clone(dyn);
        // Arguments :D
        int nth = 0;
        for (ObjectPtr arg : parms) {
            nth++;
            ostringstream oss;
            oss << "$" << nth;
            dyn1->put(oss.str(), arg);
        }
        // Bind a special 'self' variable
        function->lexicalScope->put("self", scope);
        // Call the function
        return callMethod(result, *function, dyn1);
    } else {
        // Normal object
#ifdef PRINT_BEFORE_EXEC
        cout << "Normal" << endl;
#endif
        return target;
    }
}

StmtEqual::StmtEqual(unique_ptr<Stmt>& cls, const string& func, unique_ptr<Stmt>& asn)
    : className(move(cls)), functionName(func), rhs(move(asn)) {}

ObjectPtr StmtEqual::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr scope = lex;
    if (className) {
        // If a class was provided, use that as the "scope"
        scope = className->execute(lex, dyn);
    } else {
        // Otherwise, use lexical scope by default, or dynamic
        // scope if the name starts with $
        if ((functionName.length() > 0) && (functionName[0] == '$'))
            scope = dyn;
    }
    ObjectPtr result = rhs->execute(lex, dyn);
    scope->put(functionName, result);
    return result;
}

StmtMethod::StmtMethod(std::list< std::shared_ptr<Stmt> >& contents)
    : contents(move(contents)) {}

ObjectPtr StmtMethod::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr lex1 = clone(lex);
    ObjectPtr mthd = clone(getInheritedSlot(meta(lex), "Method"));
    Method methodData;
    methodData.lexicalScope = lex1;
    methodData.code = contents;
    mthd->prim(methodData);
    return mthd;
}

StmtNumber::StmtNumber(double value)
    : value(value) {}

ObjectPtr StmtNumber::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr num = clone(getInheritedSlot(meta(lex), "Number"));
    num->prim(value);
    return num;
}

StmtString::StmtString(const char* contents)
    : value(contents) {}

ObjectPtr StmtString::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr str = clone(getInheritedSlot(meta(lex), "String"));
    str->prim(value);
    return str;
}
