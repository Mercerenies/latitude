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
    int line = expr->line;
    if (expr->isSymbol) {
        return unique_ptr<Stmt>(new StmtSymbol(line, expr->name));
    } else if (expr->isString) {
        return unique_ptr<Stmt>(new StmtString(line, expr->name));
    } else if (expr->isNumber) {
        return unique_ptr<Stmt>(new StmtNumber(line, expr->number));
    } else if (expr->isInt) {
        return unique_ptr<Stmt>(new StmtInteger(line, expr->integer));
    } else if (expr->isBigInt) {
        return unique_ptr<Stmt>(new StmtBigInteger(line, expr->name));
    } else if (expr->isList) {
        auto args = expr->args ? translateList(expr->args) : list< unique_ptr<Stmt> >();
        return unique_ptr<Stmt>(new StmtList(line, args));
    } else if (expr->isSigil) {
        // Recall that sigil names are verified to start with a ~ in the parser
        assert(expr->name[0] == '~');
        std::string name = expr->name + 1;
        return unique_ptr<Stmt>(new StmtSigil(line, name, translateStmt(expr->rhs)));
    } else if (expr->method) {
        auto contents0 = translateList(expr->args);
        list< shared_ptr<Stmt> > contents1( contents0.size() );
        transform(contents0.begin(), contents0.end(), contents1.begin(),
                  [](auto& cc) { return move(cc); });
        return unique_ptr<Stmt>(new StmtMethod(line, contents1));
    } else if (expr->equals) {
        auto rhs = translateStmt(expr->rhs);
        auto func = expr->name;
        auto lhs = expr->lhs ? translateStmt(expr->lhs) : unique_ptr<Stmt>();
        return unique_ptr<Stmt>(new StmtEqual(line, lhs, func, rhs));
    } else {
        auto args = expr->args ? translateList(expr->args) : list< unique_ptr<Stmt> >();
        auto func = expr->name;
        auto lhs = expr->lhs ? translateStmt(expr->lhs) : unique_ptr<Stmt>();
        return unique_ptr<Stmt>(new StmtCall(line, lhs, func, args));
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

int grabLineNumber(const Scope& scope, ObjectPtr dyn) {
    ObjectPtr meta_ = meta(scope, scope.lex);
    ObjectPtr lineStorage_ = getInheritedSlot(scope, meta_, Symbols::get()["lineStorage"]);
    Symbolic* lineStorage = boost::get<Symbolic>(&lineStorage_.lock()->prim());
    if ((lineStorage) && (hasInheritedSlot(scope, dyn, *lineStorage))) {
        ObjectPtr line = getInheritedSlot(scope, dyn, *lineStorage);
        if (auto value = boost::get<Number>(&line.lock()->prim()))
            return value->asSmallInt();
    }
    return 0;
}

std::string grabFileName(const Scope& scope, ObjectPtr dyn) {
    ObjectPtr meta_ = meta(scope, scope.lex);
    ObjectPtr fileStorage_ = getInheritedSlot(scope, meta_, Symbols::get()["fileStorage"]);
    Symbolic* fileStorage = boost::get<Symbolic>(&fileStorage_.lock()->prim());
    if ((fileStorage) && (hasInheritedSlot(scope, dyn, *fileStorage))) {
        ObjectPtr file = getInheritedSlot(scope, dyn, *fileStorage);
        if (auto value = boost::get<std::string>(&file.lock()->prim()))
            return *value;
    }
    return "???";
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
            throw doParseError(scope, "Empty statement encountered; ending parse");
        }
    } catch (std::string parseException) {
        throw doParseError(scope, parseException);
    }
}

// TODO Throw a ProtoException if an IO error occurs
ObjectPtr eval(istream& file, string fname, Scope defScope, Scope scope) {
    stringstream str;
    while (file >> str.rdbuf());
    try {
        auto stmts = parse(str.str());
        for (auto& stmt : stmts)
            stmt->propogateFileName(fname);
        list< shared_ptr<Stmt> > stmts1;
        for (unique_ptr<Stmt>& stmt : stmts)
            stmts1.push_back(shared_ptr<Stmt>(move(stmt)));
        auto mthdStmt = StmtMethod(0, stmts1);
        auto mthd = mthdStmt.execute(defScope);
        auto callback = [defScope, fname](ObjectPtr& dyn1) { hereIAm(dyn1, garnish(defScope, fname)); };
        return doCall(scope, defScope.lex, mthd, {}, callback);
    } catch (std::string parseException) {
        throw doParseError(scope, parseException);
    }
}

Stmt::Stmt(int line_no)
    : file_name("(eval)"), line_no(line_no) {}

void Stmt::establishLocation(const Scope& scope) {
    ObjectPtr meta_ = meta(scope, scope.lex);
    ObjectPtr lineStorage_ = getInheritedSlot(scope, meta_, Symbols::get()["lineStorage"]);
    ObjectPtr fileStorage_ = getInheritedSlot(scope, meta_, Symbols::get()["fileStorage"]);
    Symbolic* lineStorage = boost::get<Symbolic>(&lineStorage_.lock()->prim());
    Symbolic* fileStorage = boost::get<Symbolic>(&fileStorage_.lock()->prim());
    if (lineStorage && fileStorage) {
        scope.dyn.lock()->put(*lineStorage, garnish(scope, line_no));
        scope.dyn.lock()->put(*fileStorage, garnish(scope, file_name));
    }
}

void Stmt::propogateFileName(std::string name) {
    file_name = name;
}

StmtCall::StmtCall(int line_no, unique_ptr<Stmt>& cls, const string& func, ArgList& arg)
    : Stmt(line_no), className(move(cls)), functionName(func), args(move(arg)) {}

ObjectPtr StmtCall::execute(Scope scope) {
    establishLocation(scope);
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

void StmtCall::propogateFileName(std::string name) {
    if (className)
        className->propogateFileName(name);
    for (auto& stmt : args)
        stmt->propogateFileName(name);
    Stmt::propogateFileName(name);
}

StmtEqual::StmtEqual(int line_no, unique_ptr<Stmt>& cls, const string& func, unique_ptr<Stmt>& asn)
    : Stmt(line_no), className(move(cls)), functionName(func), rhs(move(asn)) {}

ObjectPtr StmtEqual::execute(Scope scope) {
    establishLocation(scope);
    ObjectPtr scope_ = determineScope(className, functionName, scope);
    ObjectPtr result = rhs->execute(scope);
    scope_.lock()->put(Symbols::get()[functionName], result);
    return result;
}

void StmtEqual::propogateFileName(std::string name) {
    if (className)
        className->propogateFileName(name);
    if (rhs)
        rhs->propogateFileName(name);
    Stmt::propogateFileName(name);
}

StmtMethod::StmtMethod(int line_no, std::list< std::shared_ptr<Stmt> >& contents)
    : Stmt(line_no), contents(move(contents)) {}

ObjectPtr StmtMethod::execute(Scope scope) {
    establishLocation(scope);
    ObjectPtr mthd = clone(getInheritedSlot(scope, meta(scope, scope.lex), Symbols::get()["Method"]));
    mthd.lock()->put(Symbols::get()["closure"], scope.lex);
    mthd.lock()->prim(contents);
    return mthd;
}

void StmtMethod::propogateFileName(std::string name) {
    for (auto& ptr : contents)
        ptr->propogateFileName(name);
    Stmt::propogateFileName(name);
}

StmtNumber::StmtNumber(int line_no, double value)
    : Stmt(line_no), value(value) {}

ObjectPtr StmtNumber::execute(Scope scope) {
    establishLocation(scope);
    return garnish(scope, Number(value));
}

StmtInteger::StmtInteger(int line_no, long value)
    : Stmt(line_no), value(value) {}

ObjectPtr StmtInteger::execute(Scope scope) {
    establishLocation(scope);
    return garnish(scope, Number(value));
}

StmtBigInteger::StmtBigInteger(int line_no, const char* value)
    : Stmt(line_no), value(value) {}

ObjectPtr StmtBigInteger::execute(Scope scope) {
    establishLocation(scope);
    auto value1 = static_cast<Number::bigint>(value);
    return garnish(scope, Number(value1));
}

StmtString::StmtString(int line_no, const char* contents)
    : Stmt(line_no), value(contents) {}

ObjectPtr StmtString::execute(Scope scope) {
    establishLocation(scope);
    return garnish(scope, value);
}

StmtSymbol::StmtSymbol(int line_no, const char* contents)
    : Stmt(line_no), value(contents) {}

ObjectPtr StmtSymbol::execute(Scope scope) {
    establishLocation(scope);
    return garnish(scope, Symbols::get()[value]);
}

// TODO Make the builtins (like Symbol, Method, etc.) call the clone method rather than forcing
//      a system clone operation

StmtList::StmtList(int line_no, ArgList& arg)
    : Stmt(line_no), args(move(arg)) {}

ObjectPtr StmtList::execute(Scope scope) {
    establishLocation(scope);
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

void StmtList::propogateFileName(std::string name) {
    for (auto& ptr : args)
        ptr->propogateFileName(name);
    Stmt::propogateFileName(name);
}

StmtSigil::StmtSigil(int line_no, string name, unique_ptr<Stmt> rhs)
    : Stmt(line_no), name(name), rhs(move(rhs)) {}

ObjectPtr StmtSigil::execute(Scope scope) {
    establishLocation(scope);
    ObjectPtr rhs_ = rhs->execute(scope);
    ObjectPtr meta_ = meta(scope, scope.lex);
    ObjectPtr sigil = getInheritedSlot(scope, meta_, Symbols::get()["sigil"]);
    ObjectPtr mySigil = getInheritedSlot(scope, sigil, Symbols::get()[name]);
    return doCallWithArgs(scope, sigil, mySigil, rhs_);
}

void StmtSigil::propogateFileName(std::string name) {
    rhs->propogateFileName(name);
    Stmt::propogateFileName(name);
}
