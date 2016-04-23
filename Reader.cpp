extern "C" {
    #include "lex.yy.h"
    extern int line_num;
}
#include "Reader.hpp"
#include "Symbol.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include <cstdio>
#include <list>
#include <memory>
#include <algorithm>
#include <sstream>

//#define PRINT_BEFORE_EXEC

using namespace std;

PtrToList currentLine;

void ExprDeleter::operator()(Expr* x) {
    cleanupE(x);
}

void ExprDeleter::operator()(List* x) {
    cleanupL(x);
}

extern "C" {
    void setCurrentLine(List* stmt) {
        currentLine.reset(stmt);
    }
}

PtrToList getCurrentLine() {
    return std::move(currentLine);
}

unique_ptr<Stmt> translateStmt(Expr* expr);
list< unique_ptr<Stmt> > translateList(List* list);

unique_ptr<Stmt> translateStmt(Expr* expr) {
    if (expr->isSymbol) {
        return unique_ptr<Stmt>(new StmtSymbol(expr->name));
    } else if (expr->isString) {
        return unique_ptr<Stmt>(new StmtString(expr->name));
    } else if (expr->isNumber) {
        return unique_ptr<Stmt>(new StmtNumber(expr->number));
    } else if (expr->isInt) {
        return unique_ptr<Stmt>(new StmtInteger(expr->integer));
    } else if (expr->isBigInt) {
        return unique_ptr<Stmt>(new StmtBigInteger(expr->name));
    } else if (expr->isList) {
        auto args = expr->args ? translateList(expr->args) : list< unique_ptr<Stmt> >();
        return unique_ptr<Stmt>(new StmtList(args));
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

list< unique_ptr<Stmt> > translateCurrentLine() {
    if (currentLine)
        return translateList( currentLine.get() );
    else
        return list< unique_ptr<Stmt> >();
}

void clearCurrentLine() {
    currentLine.reset();
}

ObjectPtr _callMethod(ObjectPtr self, ObjectPtr mthd, ObjectPtr lex, ObjectPtr dyn) {
    auto impl = boost::get<Method>(&mthd.lock()->prim());
    if (!impl)
        return mthd;
    ObjectPtr lex1 = clone(lex);
    Scope scope = { lex1, dyn };
    ObjectPtr result = getInheritedSlot(scope, meta(scope, mthd), Symbols::get()["Nil"]);
    // TODO Remove this if-statement and make self-binding mandatory
    //      once we're confident we've removed anywhere that it's not passed in.
    if (!self.expired())
        lex1.lock()->put(Symbols::get()["self"], self);
    lex1.lock()->put(Symbols::get()["again"], mthd);
    lex1.lock()->put(Symbols::get()["lexical"], lex1);
    lex1.lock()->put(Symbols::get()["dynamic"], dyn);
    dyn.lock()->put(Symbols::get()["$lexical"], lex1);
    dyn.lock()->put(Symbols::get()["$dynamic"], dyn);
    for (auto stmt : *impl)
        result = stmt->execute(scope);
    return result;
}

ObjectPtr doCall(Scope scope,
                 ObjectPtr self, ObjectPtr mthd,
                 list<ObjectPtr> args, function<void(ObjectPtr&)> dynCall) {
    if (!hasInheritedSlot(scope, mthd, Symbols::get()["closure"]))
        return mthd;
    ObjectPtr lex = getInheritedSlot(scope, mthd, Symbols::get()["closure"]);
    ObjectPtr dyn1 = clone(scope.dyn);
    // Arguments :D
    int nth = 0;
    for (ObjectPtr arg : args) {
        nth++;
        ostringstream oss;
        oss << "$" << nth;
        dyn1.lock()->put(Symbols::get()[oss.str()], arg);
    }
    dynCall(dyn1); // TODO Pass in the whole scope to this
    return _callMethod(self, mthd, lex, dyn1);
}

ObjectPtr determineScope(const unique_ptr<Stmt>& className, const string& functionName, const Scope& scope) {
    if (className) {
        // If a class was provided, use that as the "scope"
        return className->execute(scope);
    } else {
        // Otherwise, use lexical scope by default, or dynamic
        // scope if the name starts with $
        if ((functionName.length() > 0) && (functionName[0] == '$'))
            return scope.dyn;
        else
            return scope.lex;
    }
}

std::list< std::unique_ptr<Stmt> > parse(std::string str) {
    const char* buffer = str.c_str();
    auto curr = yy_scan_string(buffer);
    line_num = 1;
    yyparse();
    yy_delete_buffer(curr);
    auto result = translateCurrentLine();
    clearCurrentLine();
    return result;
}

ObjectPtr eval(string str, Scope scope) {
    try {
        auto result = parse(str);
        if (!result.empty()) {
            ObjectPtr val;
            for (auto& expr : result)
                val = expr->execute(scope);
            return val;
        } else {
            throw doParseError(scope.lex, "Empty statement encountered; ending parse");
        }
    } catch (std::string parseException) {
        throw doParseError(scope.lex, parseException);
    }
}

// TODO Throw a ProtoException if an IO error occurs
ObjectPtr eval(istream& file, string fname, Scope defScope, Scope scope) {
    stringstream str;
    while (file >> str.rdbuf());
    try {
        auto stmts = parse(str.str());
        list< shared_ptr<Stmt> > stmts1;
        for (unique_ptr<Stmt>& stmt : stmts)
            stmts1.push_back(shared_ptr<Stmt>(move(stmt)));
        auto mthdStmt = StmtMethod(stmts1);
        auto mthd = mthdStmt.execute(defScope);
        auto callback = [defScope, fname](ObjectPtr& dyn1) { hereIAm(dyn1, garnish(defScope, fname)); };
        return doCall(scope, defScope.lex, mthd, {}, callback);
    } catch (std::string parseException) {
        throw doParseError(scope.lex, parseException);
    }
}

StmtCall::StmtCall(unique_ptr<Stmt>& cls, const string& func, ArgList& arg)
    : className(move(cls)), functionName(func), args(move(arg)) {}

ObjectPtr StmtCall::execute(Scope scope) {
    ObjectPtr scope_ = determineScope(className, functionName, scope);
    list<ObjectPtr> parms( args.size() );
    transform(args.begin(), args.end(), parms.begin(),
              [&scope](std::unique_ptr<Stmt>& arg) {
                  return arg->execute(scope);
              });
    ObjectPtr target = getInheritedSlot(scope, scope_, Symbols::get()[functionName]);
    auto prim = (!target.expired()) ? target.lock()->prim() : boost::blank();
    if (target.expired()) {
        // Could not find slot
        // TODO Better error handling
#ifdef PRINT_BEFORE_EXEC
        cout << "No slot " << functionName << endl;
#endif
        return target;
    } else if (auto sys = boost::get<SystemCall>(&prim)) {
        // System call slot
#ifdef PRINT_BEFORE_EXEC
        cout << "Sys " << functionName << endl;
#endif
        return (*sys)(scope, parms);
    } else if (boost::get<Method>(&prim)) {
        // Standard method call
#ifdef PRINT_BEFORE_EXEC
        cout << "Func " << functionName << endl;
#endif
        // Call the function
        return doCall(scope, scope_, target, parms);
    } else {
        // Normal object
#ifdef PRINT_BEFORE_EXEC
        cout << "Normal " << functionName << endl;
#endif
        return target;
    }
}

StmtEqual::StmtEqual(unique_ptr<Stmt>& cls, const string& func, unique_ptr<Stmt>& asn)
    : className(move(cls)), functionName(func), rhs(move(asn)) {}

ObjectPtr StmtEqual::execute(Scope scope) {
    ObjectPtr scope_ = determineScope(className, functionName, scope);
    ObjectPtr result = rhs->execute(scope);
    scope_.lock()->put(Symbols::get()[functionName], result);
    return result;
}

StmtMethod::StmtMethod(std::list< std::shared_ptr<Stmt> >& contents)
    : contents(move(contents)) {}

ObjectPtr StmtMethod::execute(Scope scope) {
    ObjectPtr mthd = clone(getInheritedSlot(scope, meta(scope, scope.lex), Symbols::get()["Method"]));
    mthd.lock()->put(Symbols::get()["closure"], scope.lex);
    mthd.lock()->prim(contents);
    return mthd;
}

StmtNumber::StmtNumber(double value)
    : value(value) {}

ObjectPtr StmtNumber::execute(Scope scope) {
    return garnish(scope, Number(value));
}

StmtInteger::StmtInteger(long value)
    : value(value) {}

ObjectPtr StmtInteger::execute(Scope scope) {
    return garnish(scope, Number(value));
}

StmtBigInteger::StmtBigInteger(const char* value)
    : value(value) {}

ObjectPtr StmtBigInteger::execute(Scope scope) {
    auto value1 = static_cast<Number::bigint>(value);
    return garnish(scope, Number(value1));
}

StmtString::StmtString(const char* contents)
    : value(contents) {}

ObjectPtr StmtString::execute(Scope scope) {
    return garnish(scope, value);
}

StmtSymbol::StmtSymbol(const char* contents)
    : value(contents) {}

ObjectPtr StmtSymbol::execute(Scope scope) {
    return garnish(scope, Symbols::get()[value]);
}

// TODO Make the builtins (like Symbol, Method, etc.) call the clone method rather than forcing
//      a system clone operation

StmtList::StmtList(ArgList& arg)
    : args(move(arg)) {}

ObjectPtr StmtList::execute(Scope scope) {
    ObjectPtr meta_ = meta(scope, scope.lex);
    list<ObjectPtr> parms( args.size() );
    transform(args.begin(), args.end(), parms.begin(),
              [&scope](std::unique_ptr<Stmt>& arg) {
                  return arg->execute(scope);
              });
    ObjectPtr builder = getInheritedSlot(scope, meta_, Symbols::get()["brackets"]);
    ObjectPtr builder0 = doCallWithArgs(scope, meta_, builder);
    for (ObjectPtr elem : parms) {
        ObjectPtr mthd = getInheritedSlot(scope, builder0, Symbols::get()["next"]);
        doCallWithArgs(scope, builder0, mthd, elem);
    }
    ObjectPtr mthd1 = getInheritedSlot(scope, builder0, Symbols::get()["finish"]);
    return doCallWithArgs(scope, builder0, mthd1);
}
