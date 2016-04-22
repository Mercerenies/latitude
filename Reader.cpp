extern "C" {
    #include "lex.yy.h"
    extern int line_num;
}
#include "Reader.hpp"
#include "Symbol.hpp"
#include "Standard.hpp"
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

ObjectPtr callMethod(ObjectSPtr self, ObjectPtr mthd, ObjectPtr dyn) {
    ObjectPtr result = getInheritedSlot(meta(mthd), Symbols::get()["Nil"]);
    ObjectPtr lex = getInheritedSlot(mthd, Symbols::get()["closure"]);
    auto impl = boost::get<Method>(&mthd.lock()->prim());
    if (!impl)
        return mthd;
    ObjectPtr lex1 = clone(lex);
    // TODO Remove this if-statement and make self-binding mandatory
    //      once we're confident we've removed anywhere that it's not passed in.
    if (self)
        lex1.lock()->put(Symbols::get()["self"], self);
    lex1.lock()->put(Symbols::get()["again"], mthd);
    lex1.lock()->put(Symbols::get()["lexical"], lex1);
    lex1.lock()->put(Symbols::get()["dynamic"], dyn);
    dyn.lock()->put(Symbols::get()["$lexical"], lex1);
    dyn.lock()->put(Symbols::get()["$dynamic"], dyn);
    for (auto stmt : *impl)
        result = stmt->execute(lex1, dyn);
    return result;
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

ObjectPtr eval(string str, ObjectPtr lex, ObjectPtr dyn) {
    try {
        auto result = parse(str);
        if (!result.empty()) {
            ObjectPtr val;
            for (auto& expr : result)
                val = expr->execute(lex, dyn);
            return val;
        } else {
            throw doParseError(lex, "Empty statement encountered; ending parse");
        }
    } catch (std::string parseException) {
        throw doParseError(lex, parseException);
    }
}

// TODO Throw a ProtoException if an IO error occurs
ObjectPtr eval(istream& file, ObjectPtr lexDef, ObjectPtr dynDef, ObjectPtr lex, ObjectPtr dyn) {
    stringstream str;
    while (file >> str.rdbuf());
    try {
        auto stmts = parse(str.str());
        list< shared_ptr<Stmt> > stmts1;
        for (unique_ptr<Stmt>& stmt : stmts)
            stmts1.push_back(shared_ptr<Stmt>(move(stmt)));
        auto mthdStmt = StmtMethod(stmts1);
        auto mthd = mthdStmt.execute(lexDef, dynDef);
        return callMethod(lexDef.lock(), mthd, clone(dyn));
    } catch (std::string parseException) {
        throw doParseError(lex, parseException);
    }
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
    ObjectPtr target = getInheritedSlot(scope, Symbols::get()[functionName]);
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
        cout << "Sys" << endl;
#endif
        return (*sys)(parms);
    } else if (boost::get<Method>(&prim)) {
        // Standard method call
#ifdef PRINT_BEFORE_EXEC
        cout << "Func" << endl;
#endif
        ObjectPtr dyn1 = clone(dyn);
        // Arguments :D
        int nth = 0;
        for (ObjectPtr arg : parms) {
            nth++;
            ostringstream oss;
            oss << "$" << nth;
            dyn1.lock()->put(Symbols::get()[oss.str()], arg);
        }
        // Call the function
        return callMethod(scope.lock(), target, dyn1);
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
    scope.lock()->put(Symbols::get()[functionName], result);
    return result;
}

StmtMethod::StmtMethod(std::list< std::shared_ptr<Stmt> >& contents)
    : contents(move(contents)) {}

ObjectPtr StmtMethod::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr mthd = clone(getInheritedSlot(meta(lex), Symbols::get()["Method"]));
    mthd.lock()->put(Symbols::get()["closure"], lex);
    mthd.lock()->prim(contents);
    return mthd;
}

StmtNumber::StmtNumber(double value)
    : value(value) {}

ObjectPtr StmtNumber::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr num = clone(getInheritedSlot(meta(lex), Symbols::get()["Number"]));
    num.lock()->prim(Number(value));
    return num;
}

StmtInteger::StmtInteger(long value)
    : value(value) {}

ObjectPtr StmtInteger::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr num = clone(getInheritedSlot(meta(lex), Symbols::get()["Number"]));
    num.lock()->prim(Number(value));
    return num;
}

StmtBigInteger::StmtBigInteger(const char* value)
    : value(value) {}

ObjectPtr StmtBigInteger::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr num = clone(getInheritedSlot(meta(lex), Symbols::get()["Number"]));
    auto value1 = static_cast<Number::bigint>(value);
    num.lock()->prim(Number(value1));
    return num;
}

StmtString::StmtString(const char* contents)
    : value(contents) {}

ObjectPtr StmtString::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr str = clone(getInheritedSlot(meta(lex), Symbols::get()["String"]));
    str.lock()->prim(value);
    return str;
}

StmtSymbol::StmtSymbol(const char* contents)
    : value(contents) {}

ObjectPtr StmtSymbol::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr str = clone(getInheritedSlot(meta(lex), Symbols::get()["Symbol"]));
    Symbolic sym ( Symbols::get()[value] );
    str.lock()->prim(sym);
    return str;
}

// TODO Make the builtins (like Symbol, Method, etc.) call the clone method rather than forcing
//      a system clone operation

StmtList::StmtList(ArgList& arg)
    : args(move(arg)) {}

ObjectPtr StmtList::execute(ObjectPtr lex, ObjectPtr dyn) {
    ObjectPtr meta_ = meta(lex);
    list<ObjectPtr> parms( args.size() );
    transform(args.begin(), args.end(), parms.begin(),
              [&lex, &dyn](std::unique_ptr<Stmt>& arg) {
                  return arg->execute(lex, dyn);
              });
    ObjectPtr builder = getInheritedSlot(meta_, Symbols::get()["brackets"]);
    ObjectPtr builder0 = callMethod(meta_.lock(),
                                    builder,
                                    clone(dyn));
    for (ObjectPtr elem : parms) {
        ObjectPtr mthd = getInheritedSlot(builder0, Symbols::get()["next"]);
        ObjectPtr dyn1 = clone(dyn);
        dyn1.lock()->put(Symbols::get()["$1"], elem);
        callMethod(builder0.lock(), mthd, dyn1);
    }
    ObjectPtr mthd1 = getInheritedSlot(builder0, Symbols::get()["finish"]);
    return callMethod(builder0.lock(), mthd1, clone(dyn));
}
