//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

extern "C" {
    #include "lex.yy.h"
    extern int line_num;
    extern char* filename;
    extern int comments;
    extern int hash_parens;
}
#include "Reader.hpp"
#include "Symbol.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include "Assembler.hpp"
#include "Optimizer.hpp"
#include "Pathname.hpp"
#include "Precedence.hpp"
#include "Serialize.hpp"
#include <cstdio>
#include <cctype>
#include <list>
#include <memory>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <boost/scope_exit.hpp>
#include <boost/blank.hpp>

//#define DEBUG_LOADS

using namespace std;

PtrToList currentLine;

void ExprDeleter::operator()(Expr* x) const {
    cleanupE(x);
}

void ExprDeleter::operator()(List* x) const {
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

unique_ptr<Stmt> translateStmt(const OperatorTable& table, Expr*& expr, bool held);
list< unique_ptr<Stmt> > translateList(const OperatorTable& table, List* list, bool held);

unique_ptr<Stmt> translateStmt(const OperatorTable& table, Expr*& expr, bool held) {
    int line = expr->line;
    assert(!expr->isDummy);
    if (expr->isOperator) {
        expr = reorganizePrecedence(table, expr);
    }
    if (expr->isWrapper) {
        // Has no actual effect; blocks parsing of operator table in parenthesized cases
        return translateStmt(table, expr->lhs, held);
    } else if (expr->isSymbol) {
        assert(expr->namelen >= 0);
        return unique_ptr<Stmt>(new StmtSymbol(line, { expr->name, (unsigned long)expr->namelen }));
    } else if (expr->isString) {
        assert(expr->namelen >= 0);
        return unique_ptr<Stmt>(new StmtString(line, { expr->name, (unsigned long)expr->namelen }));
    } else if (expr->isNumber) {
        return unique_ptr<Stmt>(new StmtNumber(line, expr->number));
    } else if (expr->isInt) {
        return unique_ptr<Stmt>(new StmtInteger(line, expr->integer));
    } else if (expr->isBigInt) {
        return unique_ptr<Stmt>(new StmtBigInteger(line, expr->name));
    } else if (expr->isList) {
        auto args = expr->args ? translateList(table, expr->args, held) : list< unique_ptr<Stmt> >();
        return unique_ptr<Stmt>(new StmtList(line, args));
    } else if (expr->isSigil) {
        // Recall that sigil names are verified to start with a ~ in the parser
        assert(expr->name[0] == '~');
        std::string name = expr->name + 1;
        return unique_ptr<Stmt>(new StmtSigil(line, name, translateStmt(table, expr->rhs, held)));
    } else if (expr->isZeroDispatch) {
        const char* name = expr->name;
        if (name[0] == '0')
            return unique_ptr<Stmt>(new StmtZeroDispatch(line, name[1], '\0', name + 2));
        else
            return unique_ptr<Stmt>(new StmtZeroDispatch(line, name[2], name[0], name + 3));
    } else if (expr->isMethod) {
        auto contents0 = translateList(table, expr->args, held);
        list< shared_ptr<Stmt> > contents1( contents0.size() );
        transform(contents0.begin(), contents0.end(), contents1.begin(),
                  [](auto& cc) { return move(cc); });
        return unique_ptr<Stmt>(new StmtMethod(line, contents1));
    } else if (expr->isComplex) {
        return unique_ptr<Stmt>(new StmtComplex(line, expr->number, expr->number1));
    } else if (expr->equals) {
        auto rhs = translateStmt(table, expr->rhs, held);
        auto func = expr->name;
        auto lhs = expr->lhs ? translateStmt(table, expr->lhs, held) : unique_ptr<Stmt>();
        return unique_ptr<Stmt>(new StmtEqual(line, lhs, func, rhs));
    } else if (expr->equals2) {
        auto rhs = translateStmt(table, expr->rhs, held);
        auto func = expr->name;
        auto lhs = expr->lhs ? translateStmt(table, expr->lhs, held) : unique_ptr<Stmt>();
        return unique_ptr<Stmt>(new StmtDoubleEqual(line, lhs, func, rhs));
    } else if (expr->isHashQuote) {
        return translateStmt(table, expr->lhs, true);
    } else if (expr->isDict) {
        auto list = expr->args;
        StmtDict::KVList res;
        while ((list->car != nullptr) && (list->cdr != nullptr)) {
            res.emplace_back(std::move(translateStmt(table, list->car->lhs, held)),
                             std::move(translateStmt(table, list->car->rhs, held)));
            list = list->cdr;
        }
        return unique_ptr<Stmt>(new StmtDict(line, res));
    } else {
        auto args = expr->args ? translateList(table, expr->args, held) : list< unique_ptr<Stmt> >();
        string func;
        if (expr->isBind) {
            func = "<-";
            args.push_front(translateStmt(table, expr->rhs, held));
            args.push_front(unique_ptr<Stmt>(new StmtSymbol(line, expr->name)));
        } else {
            func = expr->name;
            if (expr->isEquality) {
                args.push_back(translateStmt(table, expr->rhs, held));
                func = func + "=";
            }
        }
        auto lhs = expr->lhs ? translateStmt(table, expr->lhs, held) : unique_ptr<Stmt>();
        return unique_ptr<Stmt>(new StmtCall(line, lhs, func, args, expr->argsProvided || !held));
    }
}

list< unique_ptr<Stmt> > translateList(const OperatorTable& table, List* lst, bool held) {
    if ((lst->car == nullptr) || (lst->cdr == nullptr)) {
        return list< unique_ptr<Stmt> >();
    } else {
        auto head = translateStmt(table, lst->car, held);
        auto tail = translateList(table, lst->cdr, held);
        tail.push_front(move(head));
        return tail;
    }
}

list< unique_ptr<Stmt> > translateCurrentLine(const OperatorTable& table) {
    if (currentLine)
        return translateList(table, currentLine.get(), false);
    else
        return list< unique_ptr<Stmt> >();
}

void clearCurrentLine() noexcept {
    currentLine.reset();
}

std::list< std::unique_ptr<Stmt> > parse(const OperatorTable& table,
                                         std::string filename,
                                         std::string str) {
    const char* buffer = str.c_str();
    auto curr = yy_scan_string(buffer);
    auto& g_filename = ::filename;
    line_num = 1;
    if (g_filename != nullptr)
        delete[] g_filename;
    g_filename = new char[filename.length() + 1];
    g_filename[filename.length()] = 0;
    strcpy(g_filename, filename.c_str());
    comments = 0;
    hash_parens = 0;
    yyparse();
    yy_delete_buffer(curr);
    auto result = translateCurrentLine(table);
    clearCurrentLine();
    return result;
}

bool eval(VMState& vm,
          const OperatorTable& table,
          string str) {
    try {
        auto result = parse(table, "(eval)", str);
        if (!result.empty()) {
            TranslationUnitPtr unit = make_shared<TranslationUnit>();
            InstrSeq toplevel;
            for (auto& stmt : result) {
                stmt->translate(*unit, toplevel);
            }
            (makeAssemblerLine(Instr::UNTR)).appendOnto(toplevel);
            vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
            unit->instructions() = toplevel;
            vm.state.cont = MethodSeek(Method(unit, { 0 }));
            vm.state.trns.push(unit);
        } else {
            vm.trans.ret = garnishObject(vm.reader, boost::blank());
        }
    } catch (ParseError& e) {
        throwError(vm, "ParseError", e.getMessage());
        return false;
    }
    return true;
}

bool readFileSource(string fname,
                    Scope defScope,
                    VMState& vm,
                    const OperatorTable& table) {
#ifdef DEBUG_LOADS
    cout << "Loading " << fname << "..." << endl;
#endif
    ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    try {
        file.open(fname);
        BOOST_SCOPE_EXIT(&file) {
            file.close();
        } BOOST_SCOPE_EXIT_END;
        stringstream str;
        while ((file >> str.rdbuf()).good());
        try {
            auto stmts = parse(table, fname, str.str());
            for (auto& stmt : stmts)
                stmt->propogateFileName(fname);
            list< shared_ptr<Stmt> > stmts1;
            for (unique_ptr<Stmt>& stmt : stmts)
                stmts1.push_back(shared_ptr<Stmt>(move(stmt)));
            TranslationUnitPtr unit = make_shared<TranslationUnit>();
            InstrSeq toplevel;
            (makeAssemblerLine(Instr::LOCFN, fname)).appendOnto(toplevel);
            for (auto& stmt : stmts1) {
                stmt->translate(*unit, toplevel);
            }
            (makeAssemblerLine(Instr::RET)).appendOnto(toplevel);
            auto lex = vm.state.lex.top();
            vm.state.lex.push(defScope.lex);
            if (!vm.state.dyn.empty()) {
                vm.state.dyn.push( clone(vm.state.dyn.top()) );
            }
            ObjectPtr mthd = clone(vm.reader.lit.at(Lit::METHOD));
            mthd->prim(Method(unit, { 0 }));
            vm.state.lex.top()->put(Symbols::get()["self"], vm.state.lex.top());
            vm.state.lex.top()->put(Symbols::get()["again"], mthd);
            vm.state.lex.top()->put(Symbols::get()["caller"], lex);
            unit->instructions() = toplevel;
            optimize::lookupSymbols(unit);
            vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
            vm.state.cont = MethodSeek(Method(unit, { 0 }));
            pushTrace(vm.state);
            vm.state.trns.push(unit);
        } catch (ParseError& e) {
#ifdef DEBUG_LOADS
            std::cout << e.getMessage() << std::endl;
#endif
            throwError(vm, "ParseError", e.getMessage());
            return false;
        }
    } catch (ios_base::failure& err) {
        throwError(vm, "IOError", err.what());
        return false;
    }
    return true;
}

void saveInstrs(ofstream& file, const InstrSeq& seq) {
    SerialInstrSeq compiled;
    {
        auto iter = std::back_inserter(compiled);
        for (const auto& instr : seq) {
            serialize<AssemblerLine>(instr, iter);
        }
    }
    unsigned long length = compiled.size();
    for (int i = 0; i < 8; i++) {
        file << (unsigned char)(length % 256);
        length >>= 8;
    }
    for (const unsigned char& ch : compiled) {
        file << ch;
    }
}

void saveToFile(ofstream& file, const Header& header, TranslationUnitPtr unit) {
    saveFileHeader(file, header);
    saveInstrs(file, unit->instructions());
    for (int i = 1; i < unit->methodCount(); i++) {
        saveInstrs(file, unit->method(i));
    }
}

unsigned long loadAsNumber(ifstream& file) {
    unsigned long result = 0UL;
    unsigned char curr[8];
    for (int i = 0; i < 8; i++) {
        if (file.eof())
            return 0;
        curr[i] = file.get();
    }
    for (int i = 7; i >= 0; i--) {
        result <<= 8;
        result += (unsigned long)curr[i];
    }
    return result;
}

SerialInstrSeq loadSeq(ifstream& file, unsigned long length) {
    SerialInstrSeq result;
    for (unsigned long i = 0; i < length; i++) {
        unsigned char ch;
        ch = file.get();
        result.push_back(ch);
    }
    return result;
}

template <typename InputIterator, typename OutputIterator>
void parseSeq(InputIterator begin, InputIterator end, OutputIterator& seqOut) {
    while (begin != end) {
        AssemblerLine instruction = deserialize<AssemblerLine>(begin);
        *seqOut++ = instruction;
    }
}

// Throws HeaderError
TranslationUnitPtr loadFromFile(ifstream& file) {
    TranslationUnitPtr result = make_shared<TranslationUnit>();
    getFileHeaderComp(file); // Ignore it; we don't need it right now
    {
        unsigned long length = loadAsNumber(file);
        SerialInstrSeq seq = loadSeq(file, length);
        auto outputIter = std::back_inserter(result->instructions());
        parseSeq(seq.begin(), seq.end(), outputIter);
    }
    while (true) {
        unsigned long mlength = loadAsNumber(file);
        if (file.eof())
            break;
        SerialInstrSeq mseq = loadSeq(file, mlength);
        auto curr = result->pushMethod(InstrSeq());
        auto outputIter = std::back_inserter(result->method(curr.index));
        parseSeq(mseq.begin(), mseq.end(), outputIter);
    }
    return result;
}

bool compileFile(string fname,
                 string fname1,
                 VMState& vm,
                 const OperatorTable& table) {
#ifdef DEBUG_LOADS
    cout << "Compiling " << fname << " into " << fname1 << "..." << endl;
#endif
    Header header;
    {
        ifstream file0 { fname };
        BOOST_SCOPE_EXIT(&file0) {
            file0.close();
        } BOOST_SCOPE_EXIT_END;
        header = getFileHeaderSource(file0);
    }
    ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    try {
        file.open(fname);
        BOOST_SCOPE_EXIT(&file) {
            file.close();
        } BOOST_SCOPE_EXIT_END;
        stringstream str;
        while ((file >> str.rdbuf()).good());
        try {
            auto stmts = parse(table, fname, str.str());
            for (auto& stmt : stmts)
                stmt->propogateFileName(fname);
            list< shared_ptr<Stmt> > stmts1;
            for (unique_ptr<Stmt>& stmt : stmts)
                stmts1.push_back(shared_ptr<Stmt>(move(stmt)));
            TranslationUnitPtr unit = make_shared<TranslationUnit>();
            InstrSeq toplevel;
            (makeAssemblerLine(Instr::LOCFN, fname)).appendOnto(toplevel);
            for (auto& stmt : stmts1) {
                stmt->translate(*unit, toplevel);
            }
            (makeAssemblerLine(Instr::RET)).appendOnto(toplevel);
            ofstream file1;
            file1.open(fname1, std::ofstream::out | std::ofstream::binary);
            BOOST_SCOPE_EXIT(&file1) {
                file1.close();
            } BOOST_SCOPE_EXIT_END;
            unit->instructions() = toplevel;
            saveToFile(file1, header, unit);
        } catch (ParseError& e) {
            throwError(vm, "ParseError", e.getMessage());
            return false;
        }
    } catch (ios_base::failure& err) {
        throwError(vm, "IOError", err.what());
        return false;
    }
    return true;
}

bool readFileComp(string fname,
                  Scope defScope,
                  VMState& vm,
                  const OperatorTable& table) {
#ifdef DEBUG_LOADS
    cout << "Loading (compiled) " << fname << "..." << endl;
#endif
    ifstream file;
    file.exceptions(ifstream::badbit);
    try {
        file.open(fname, std::ifstream::in | std::ifstream::binary);
        BOOST_SCOPE_EXIT(&file) {
            file.close();
        } BOOST_SCOPE_EXIT_END;
        try {
            TranslationUnitPtr unit = loadFromFile(file);
            auto lex = vm.state.lex.top();
            vm.state.lex.push(defScope.lex);
            if (!vm.state.dyn.empty()) {
                vm.state.dyn.push( clone(vm.state.dyn.top()) );
            }
            ObjectPtr mthd = clone(vm.reader.lit.at(Lit::METHOD));
            mthd->prim(Method(unit, { 0 }));
            vm.state.lex.top()->put(Symbols::get()["self"], vm.state.lex.top());
            vm.state.lex.top()->put(Symbols::get()["again"], mthd);
            vm.state.lex.top()->put(Symbols::get()["caller"], lex);
            optimize::lookupSymbols(unit);
            vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
            vm.state.cont = MethodSeek(Method(unit, { 0 }));
            pushTrace(vm.state);
            vm.state.trns.push(unit);
        } catch (ParseError& e) {
            throwError(vm, "ParseError", e.getMessage());
            return false;
        } catch (HeaderError& e) {
            throwError(vm, "ParseError", e.getMessage());
            return false;
        }
    } catch (ios_base::failure& err) {
        throwError(vm, "IOError", err.what());
        return false;
    }
    return true;
}

bool needsRecompile(string fname, string cname) {

    // TODO Detect whether or not we can write to the target directory and never recompile if we can't.

    // If the source doesn't exist, don't recompile.
    if (!fileExists(fname))
        return false;

    // Otherwise, recompile if any of the conditions are met.
    bool recomp = false;
    // (1) The compiled file doesn't exist.
    recomp = recomp || !fileExists(cname);
    // (2) The source file was modified after the compiled file.
    recomp = recomp || (std::difftime(modificationTime(cname), modificationTime(fname)) <= 0);

    return recomp;

}

bool readFile(string fname,
              Scope defScope,
              VMState& vm,
              const OperatorTable& table) {
    bool okay = true;
    string cname = fname + "c";
    if (needsRecompile(fname, cname)) {
        okay &= compileFile(fname, cname, vm, table);
    }
    // readFileSource(fname, defScope, vm);
    if (okay)
        okay &= readFileComp(cname, defScope, vm, table);
    return okay;
}

Header getFileHeader(std::string filename) {
    std::string cname = filename + "c";
    if (needsRecompile(filename, cname)) {
        std::ifstream file { filename };
        BOOST_SCOPE_EXIT(&file) {
            file.close();
        } BOOST_SCOPE_EXIT_END;
        return getFileHeaderSource(file);
    } else {
        std::ifstream file { cname };
        BOOST_SCOPE_EXIT(&file) {
            file.close();
        } BOOST_SCOPE_EXIT_END;
        return getFileHeaderComp(file);
    }
}

Stmt::Stmt(int line_no)
    : file_name("(eval)"), line_no(line_no), location(true) {}

int Stmt::line() {
    return line_no;
}

void Stmt::disableLocationInformation() {
    location = false;
}

void Stmt::stateLine(InstrSeq& seq) const {
    if (!location)
        return;
    InstrSeq seq0 = asmCode(makeAssemblerLine(Instr::LOCLN, line_no));
    seq.insert(seq.end(), seq0.begin(), seq0.end());
}

void Stmt::stateFile(InstrSeq& seq) const {
    if (!location)
        return;
    InstrSeq seq0 = asmCode(makeAssemblerLine(Instr::LOCFN, file_name));
    seq.insert(seq.end(), seq0.begin(), seq0.end());
}

void Stmt::propogateFileName(std::string name) {
    file_name = name;
}

StmtCall::StmtCall(int line_no, unique_ptr<Stmt>& cls, const string& func, ArgList& arg, bool hasArgs)
    : Stmt(line_no), className(move(cls)), functionName(func), args(move(arg)), performCall(hasArgs) {}

void StmtCall::translate(TranslationUnit& unit, InstrSeq& seq) {

    if (!className)
        stateLine(seq);

    // Evaluate the class name
    if (className) {
        className->translate(unit, seq);
    } else if ((functionName != "") && (functionName[0] == '$')) {
        (makeAssemblerLine(Instr::GETD, Reg::RET)).appendOnto(seq);
    } else {
        (makeAssemblerLine(Instr::GETL, Reg::RET)).appendOnto(seq);
    }

    if (performCall) {
        // Store the class name object
        (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);
    }

    // "Evaluate" the name to lookup (may incur `missing`)
    (makeAssemblerLine(Instr::SYM, functionName)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);

    if (performCall) {

        // Store the method/slot object
        if (!args.empty())
            (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);
        else
            (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);

        // Evaluate each of the arguments, in order
        for (auto& arg : args) {
            arg->translate(unit, seq);
            (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);
        }

        // Make the call
        if (!args.empty())
            (makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO)).appendOnto(seq);
        (makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO)).appendOnto(seq);
        (makeAssemblerLine(Instr::CALL, (long)args.size())).appendOnto(seq);

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

void StmtEqual::translate(TranslationUnit& unit, InstrSeq& seq) {

    if (!className)
        stateLine(seq);

    // Evaluate the class name
    if (className) {
        className->translate(unit, seq);
    } else if ((functionName != "") && (functionName[0] == '$')) {
        (makeAssemblerLine(Instr::GETD, Reg::RET)).appendOnto(seq);
    } else {
        (makeAssemblerLine(Instr::GETL, Reg::RET)).appendOnto(seq);
    }

    // Store the class name
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);

    // Evaluate the right-hand-side
    rhs->translate(unit, seq);

    // Load the appropriate values into the registers they need to be in
    (makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);

    // Put the name in the symbol field
    (makeAssemblerLine(Instr::SYM, functionName)).appendOnto(seq);

    // Put the value
    (makeAssemblerLine(Instr::SETF)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET)).appendOnto(seq);

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

void StmtMethod::translate(TranslationUnit& unit, InstrSeq& seq) {

    //stateLine(seq);

    // Find the literal object to use
    (makeAssemblerLine(Instr::YLDC, Lit::METHOD, Reg::RET)).appendOnto(seq);

    // Translate the method sequence
    InstrSeq mthd;
    stateLine(mthd);
    stateFile(mthd);
    if (contents.empty()) {
        // If the method is empty, it defaults to `meta Nil.`
        (makeAssemblerLine(Instr::YLD, Lit::NIL, Reg::RET)).appendOnto(mthd);
    } else {
        for (auto& val : contents) {
            val->translate(unit, mthd);
        }
    }
    (makeAssemblerLine(Instr::RET)).appendOnto(mthd);

    FunctionIndex index = unit.pushMethod(mthd);

    // Clone and put a prim() onto it
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
    (makeAssemblerLine(Instr::MTHD, index)).appendOnto(seq);
    (makeAssemblerLine(Instr::LOAD, Reg::MTHD)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(seq);

    // Give the object an appropriate closure
    (makeAssemblerLine(Instr::GETL, Reg::PTR)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "closure")).appendOnto(seq);
    (makeAssemblerLine(Instr::SETF)).appendOnto(seq);
    (makeAssemblerLine(Instr::INT, 2)).appendOnto(seq);
    (makeAssemblerLine(Instr::CPP, Table::CPP_PROT_VAR)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET)).appendOnto(seq);

}

void StmtMethod::propogateFileName(std::string name) {
    for (auto& ptr : contents)
        ptr->propogateFileName(name);
    Stmt::propogateFileName(name);
}

StmtNumber::StmtNumber(int line_no, double value)
    : Stmt(line_no), value(value) {}

void StmtNumber::translate(TranslationUnit& unit, InstrSeq& seq) {

    //stateLine(seq);

    // Find the literal object to use
    (makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::PTR)).appendOnto(seq);

    // Clone and put a prim() onto it
    (makeAssemblerLine(Instr::FLOAT, to_string(value))).appendOnto(seq);
    (makeAssemblerLine(Instr::LOAD, Reg::NUM0)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET)).appendOnto(seq);

}

StmtInteger::StmtInteger(int line_no, long value)
    : Stmt(line_no), value(value) {}

void StmtInteger::translate(TranslationUnit& unit, InstrSeq& seq) {

    //stateLine(seq);

    // Find the literal object to use
    (makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::PTR)).appendOnto(seq);

    // Clone and put a prim() onto it
    (makeAssemblerLine(Instr::INT, value)).appendOnto(seq);
    (makeAssemblerLine(Instr::LOAD, Reg::NUM0)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET)).appendOnto(seq);

}

StmtBigInteger::StmtBigInteger(int line_no, const char* value)
    : Stmt(line_no), value(value) {}

void StmtBigInteger::translate(TranslationUnit& unit, InstrSeq& seq) {

    std::string value0 = "D" + value;

    //stateLine(seq);

    // Find the literal object to use
    (makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::PTR)).appendOnto(seq);

    // Clone and put a prim() onto it
    (makeAssemblerLine(Instr::NUM, value0)).appendOnto(seq);
    (makeAssemblerLine(Instr::LOAD, Reg::NUM0)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET)).appendOnto(seq);

}

StmtString::StmtString(int line_no, std::string contents)
    : Stmt(line_no), value(contents) {}

void StmtString::translate(TranslationUnit& unit, InstrSeq& seq) {

    //stateLine(seq);

    // Find the literal object to use
    (makeAssemblerLine(Instr::YLDC, Lit::STRING, Reg::PTR)).appendOnto(seq);

    // Clone and put a prim() onto it
    (makeAssemblerLine(Instr::STR, value)).appendOnto(seq);
    (makeAssemblerLine(Instr::LOAD, Reg::STR0)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET)).appendOnto(seq);

}

StmtSymbol::StmtSymbol(int line_no, std::string contents)
    : Stmt(line_no), value(contents) {}

void StmtSymbol::translate(TranslationUnit& unit, InstrSeq& seq) {

    //stateLine(seq);

    // Find the literal object to use
    (makeAssemblerLine(Instr::YLDC, Lit::SYMBOL, Reg::PTR)).appendOnto(seq);

    // Clone and put a prim() onto it
    (makeAssemblerLine(Instr::SYM, value)).appendOnto(seq);
    (makeAssemblerLine(Instr::LOAD, Reg::SYM)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET)).appendOnto(seq);

}

StmtList::StmtList(int line_no, ArgList& arg)
    : Stmt(line_no), args(move(arg)) {}

void StmtList::translate(TranslationUnit& unit, InstrSeq& seq) {

    stateLine(seq);

    // Evaluate each of the arguments, in order
    for (auto& arg : args) {
        arg->translate(unit, seq);
        (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);
    }

    // Make the call
    (makeAssemblerLine(Instr::ARR, (long)args.size())).appendOnto(seq);

}

void StmtList::propogateFileName(std::string name) {
    for (auto& ptr : args)
        ptr->propogateFileName(name);
    Stmt::propogateFileName(name);
}

StmtSigil::StmtSigil(int line_no, string name, unique_ptr<Stmt> rhs)
    : Stmt(line_no), name(name), rhs(move(rhs)) {}

void StmtSigil::translate(TranslationUnit& unit, InstrSeq& seq) {

    //stateLine(seq);

    // Find the `sigil` object
    (makeAssemblerLine(Instr::GETL, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "meta")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "sigil")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq);

    // Store the sigil object
    (makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO)).appendOnto(seq);

    // Evaluate the argument
    rhs->translate(unit, seq);
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);

    // Get the correct sigil out
    (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, name)).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);

    // Call the sigil with the single argument
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
    (makeAssemblerLine(Instr::CALL, 1L)).appendOnto(seq);

    // Leave %sto the way we found it
    (makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO)).appendOnto(seq);

}

void StmtSigil::propogateFileName(std::string name) {
    rhs->propogateFileName(name);
    Stmt::propogateFileName(name);
}

StmtZeroDispatch::StmtZeroDispatch(int line_no, char sym, char ch, string text)
    : Stmt(line_no), text(text), symbol(sym), prefix(ch) {}

void StmtZeroDispatch::translate(TranslationUnit& unit, InstrSeq& seq) {

    char start = (prefix == '\0') ? '0' : prefix;
    std::string value0 = std::string(1, (char)toupper(symbol)) + start + text;

    //stateLine(seq);

    // Find the literal object to use
    (makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::PTR)).appendOnto(seq);

    // Clone and put a prim() onto it
    (makeAssemblerLine(Instr::NUM, value0)).appendOnto(seq);
    (makeAssemblerLine(Instr::LOAD, Reg::NUM0)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET)).appendOnto(seq);

}

StmtComplex::StmtComplex(int line_no, double lhs, double rhs)
    : Stmt(line_no), rl(lhs), im(rhs) {}

void StmtComplex::translate(TranslationUnit& unit, InstrSeq& seq) {

    //stateLine(seq);

    // Find the literal object to use
    (makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::PTR)).appendOnto(seq);

    // Clone and put a prim() onto it
    (makeAssemblerLine(Instr::CMPLX, to_string(rl), to_string(im))).appendOnto(seq);
    (makeAssemblerLine(Instr::LOAD, Reg::NUM0)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET)).appendOnto(seq);

}

StmtDoubleEqual::StmtDoubleEqual(int line_no, unique_ptr<Stmt>& cls, const string& func, unique_ptr<Stmt>& asn)
    : Stmt(line_no), className(move(cls)), functionName(func), rhs(move(asn)) {}

void StmtDoubleEqual::translate(TranslationUnit& unit, InstrSeq& seq) {

    if (!className)
        stateLine(seq);

    // Evaluate the class name
    if (className) {
        className->translate(unit, seq);
    } else if ((functionName != "") && (functionName[0] == '$')) {
        (makeAssemblerLine(Instr::GETD, Reg::RET)).appendOnto(seq);
    } else {
        (makeAssemblerLine(Instr::GETL, Reg::RET)).appendOnto(seq);
    }

    // Store the class name
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);

    // Evaluate the right-hand-side
    rhs->translate(unit, seq);

    // Load the appropriate values into the registers they need to be in
    (makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);

    // Put the name in the symbol field
    (makeAssemblerLine(Instr::SYM, functionName)).appendOnto(seq);

    // Put the value
    (makeAssemblerLine(Instr::SETF)).appendOnto(seq);
    (makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::STO)).appendOnto(seq);

    // Get the function name as an object
    (makeAssemblerLine(Instr::YLDC, Lit::SYMBOL, Reg::PTR)).appendOnto(seq);

    // Clone and put a prim() onto it
    (makeAssemblerLine(Instr::SYM, functionName)).appendOnto(seq);
    (makeAssemblerLine(Instr::LOAD, Reg::SYM)).appendOnto(seq);
    (makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::ARG)).appendOnto(seq);

    // Call the `::` method
    (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "::")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
    (makeAssemblerLine(Instr::CALL, 1L)).appendOnto(seq);

}

void StmtDoubleEqual::propogateFileName(std::string name) {
    if (className)
        className->propogateFileName(name);
    if (rhs)
        rhs->propogateFileName(name);
    Stmt::propogateFileName(name);
}

StmtDict::StmtDict(int line_no, KVList& arg)
    : Stmt(line_no), args(move(arg)) {}

void StmtDict::translate(TranslationUnit& unit, InstrSeq& seq) {

    stateLine(seq);

    // Evaluate each of the arguments, in order
    for (auto& arg : args) {
        arg.first->translate(unit, seq);
        (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);
        arg.second->translate(unit, seq);
        (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);
    }

    // Make the call
    (makeAssemblerLine(Instr::DICT, (long)args.size())).appendOnto(seq);

}

void StmtDict::propogateFileName(std::string name) {
    for (auto& ptr : args)
        ptr.second->propogateFileName(name);
    Stmt::propogateFileName(name);
}
