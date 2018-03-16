extern "C" {
    #include "lex.yy.h"
    extern int line_num;
    extern int comments;
    extern int hash_parens;
}
#include "Reader.hpp"
#include "Symbol.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include "Assembler.hpp"
#include <cstdio>
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
    } else if (expr->isHashParen) {
        return unique_ptr<Stmt>(new StmtHashParen(line, expr->name));
    } else if (expr->isZeroDispatch) {
        const char* name = expr->name;
        if (name[0] == '0')
            return unique_ptr<Stmt>(new StmtZeroDispatch(line, name[1], '\0', name + 2));
        else
            return unique_ptr<Stmt>(new StmtZeroDispatch(line, name[2], name[0], name + 3));
    } else if (expr->isMethod) {
        auto contents0 = translateList(expr->args);
        list< shared_ptr<Stmt> > contents1( contents0.size() );
        transform(contents0.begin(), contents0.end(), contents1.begin(),
                  [](auto& cc) { return move(cc); });
        return unique_ptr<Stmt>(new StmtMethod(line, contents1));
    } else if (expr->isSpecialMethod) {
        auto contents0 = translateList(expr->args);
        list< shared_ptr<Stmt> > contents1( contents0.size() );
        transform(contents0.begin(), contents0.end(), contents1.begin(),
                  [](auto& cc) { return move(cc); });
        return unique_ptr<Stmt>(new StmtSpecialMethod(line, contents1));
    } else if (expr->isComplex) {
        return unique_ptr<Stmt>(new StmtComplex(line, expr->number, expr->number1));
    } else if (expr->equals) {
        auto rhs = translateStmt(expr->rhs);
        auto func = expr->name;
        auto lhs = expr->lhs ? translateStmt(expr->lhs) : unique_ptr<Stmt>();
        return unique_ptr<Stmt>(new StmtEqual(line, lhs, func, rhs));
    } else if (expr->equals2) {
        auto rhs = translateStmt(expr->rhs);
        auto func = expr->name;
        auto lhs = expr->lhs ? translateStmt(expr->lhs) : unique_ptr<Stmt>();
        return unique_ptr<Stmt>(new StmtDoubleEqual(line, lhs, func, rhs));
    } else if (expr->isHashQuote) {
        auto lhs = unique_ptr<Stmt>();
        return unique_ptr<Stmt>(new StmtHeld(line, lhs, expr->name));
    } else {
        auto args = expr->args ? translateList(expr->args) : list< unique_ptr<Stmt> >();
        string func;
        if (expr->isBind) {
            func = "<-";
            args.push_front(translateStmt(expr->rhs));
            args.push_front(unique_ptr<Stmt>(new StmtSymbol(line, expr->name)));
        } else {
            func = expr->name;
            if (expr->isEquality) {
                args.push_back(translateStmt(expr->rhs));
                func = func + "=";
            }
        }
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

void clearCurrentLine() noexcept {
    currentLine.reset();
}

std::list< std::unique_ptr<Stmt> > parse(std::string str) {
    const char* buffer = str.c_str();
    auto curr = yy_scan_string(buffer);
    line_num = 1;
    comments = 0;
    hash_parens = 0;
    yyparse();
    yy_delete_buffer(curr);
    auto result = translateCurrentLine();
    clearCurrentLine();
    return result;
}

void eval(IntState& state, const ReadOnlyState& reader, string str) {
    try {
        auto result = parse(str);
        if (!result.empty()) {
            TranslationUnitPtr unit = make_shared<TranslationUnit>();
            InstrSeq toplevel;
            for (auto& stmt : result) {
                stmt->translate(*unit, toplevel);
            }
            (makeAssemblerLine(Instr::UNTR)).appendOnto(toplevel);
            state.stack = pushNode(state.stack, state.cont);
            unit->instructions() = toplevel;
            state.cont = MethodSeek(Method(unit, { 0 }));
            state.trns.push(unit);
        } else {
            state.ret = garnishObject(reader, boost::blank());
        }
    } catch (std::string str) {
        throwError(state, reader, "ParseError", str);
    }
}

void readFileSource(string fname, Scope defScope, IntState& state, const ReadOnlyState& reader) {
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
            auto stmts = parse(str.str());
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
            auto lex = state.lex.top(); // TODO Possible empty stack error?
            state.lex.push(defScope.lex);
            if (!state.dyn.empty()) {
                state.dyn.push( clone(state.dyn.top()) );
                state.dyn.top()->put(Symbols::get()["$dynamic"], state.dyn.top());
            }
            state.lex.top()->put(Symbols::get()["self"], state.lex.top());
            state.lex.top()->put(Symbols::get()["again"], state.lex.top()); // TODO Does this make sense?
            state.lex.top()->put(Symbols::get()["lexical"], state.lex.top());
            state.lex.top()->put(Symbols::get()["caller"], lex);
            state.stack = pushNode(state.stack, state.cont);
            unit->instructions() = toplevel;
            state.cont = MethodSeek(Method(unit, { 0 }));
            state.trns.push(unit);
        } catch (std::string parseException) {
#ifdef DEBUG_LOADS
            std::cout << parseException << std::endl;
#endif
            throwError(state, reader, "ParseError", parseException);
        }
    } catch (ios_base::failure err) {
        throwError(state, reader, "IOError", err.what());
    }
}

void saveInstrs(ofstream& file, const InstrSeq& seq) {
    SerialInstrSeq compiled;
    for (const auto& instr : seq) {
        appendInstruction(instr.getCommand(), compiled);
        for (const auto& arg : instr.arguments()) {
            appendRegisterArg(arg, compiled);
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

void saveToFile(ofstream& file, TranslationUnitPtr unit) {
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

void parseSeq(InstrSeq& seqOut, SerialInstrSeq& seqIn) {
    // Note: Destroys the sequence as it reads it
    while (!seqIn.empty()) {
        Instr instr = popInstr(seqIn);
        AssemblerLine instruction { instr };
        for (const auto& arg : getAsmArguments(instr)) {
            switch (arg) {
            case AsmType::LONG:
                instruction.addRegisterArg(popLong(seqIn));
                break;
            case AsmType::STRING:
                instruction.addRegisterArg(popString(seqIn));
                break;
            case AsmType::REG:
                instruction.addRegisterArg(popReg(seqIn));
                break;
            case AsmType::ASM:
                instruction.addRegisterArg(popFunction(seqIn));
                break;
            }
        }
        seqOut.push_back(instruction);
    }
}

TranslationUnitPtr loadFromFile(ifstream& file) {
    TranslationUnitPtr result = make_shared<TranslationUnit>();
    unsigned long length = loadAsNumber(file);
    SerialInstrSeq seq = loadSeq(file, length);
    parseSeq(result->instructions(), seq);
    while (true) {
        unsigned long mlength = loadAsNumber(file);
        if (file.eof())
            break;
        SerialInstrSeq mseq = loadSeq(file, mlength);
        auto curr = result->pushMethod(InstrSeq());
        parseSeq(result->method(curr.index), mseq);
    }
    return result;
}

void compileFile(string fname, string fname1, IntState& state, const ReadOnlyState& reader) {
#ifdef DEBUG_LOADS
    cout << "Compiling " << fname << " into " << fname1 << "..." << endl;
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
            auto stmts = parse(str.str());
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
            file1.open(fname1);
            BOOST_SCOPE_EXIT(&file1) {
                file1.close();
            } BOOST_SCOPE_EXIT_END;
            unit->instructions() = toplevel;
            saveToFile(file1, unit);
        } catch (std::string parseException) {
            throwError(state, reader, "ParseError", parseException);
        }
    } catch (ios_base::failure err) {
        throwError(state, reader, "IOError", err.what());
    }
}

void readFileComp(string fname, Scope defScope, IntState& state, const ReadOnlyState& reader) {
#ifdef DEBUG_LOADS
    cout << "Loading (compiled) " << fname << "..." << endl;
#endif
    ifstream file;
    file.exceptions(ifstream::badbit);
    try {
        file.open(fname);
        BOOST_SCOPE_EXIT(&file) {
            file.close();
        } BOOST_SCOPE_EXIT_END;
        try {
            TranslationUnitPtr unit = loadFromFile(file);
            auto lex = state.lex.top(); // TODO Possible empty stack error?
            state.lex.push(defScope.lex);
            if (!state.dyn.empty()) {
                state.dyn.push( clone(state.dyn.top()) );
                state.dyn.top()->put(Symbols::get()["$dynamic"], state.dyn.top());
            }
            state.lex.top()->put(Symbols::get()["self"], state.lex.top());
            state.lex.top()->put(Symbols::get()["again"], state.lex.top()); // TODO Does this make sense?
            state.lex.top()->put(Symbols::get()["lexical"], state.lex.top());
            state.lex.top()->put(Symbols::get()["caller"], lex);
            state.stack = pushNode(state.stack, state.cont);
            state.cont = MethodSeek(Method(unit, { 0 }));
            state.trns.push(unit);
        } catch (std::string parseException) {
            throwError(state, reader, "ParseError", parseException);
        }
    } catch (ios_base::failure err) {
        throwError(state, reader, "IOError", err.what());
    }
}

void readFile(string fname, Scope defScope, IntState& state, const ReadOnlyState& reader) {
    // Soon, this will attempt to read a compiled file first.
    // compileFile(fname, fname + "c", state, reader);
    readFileSource(fname, defScope, state, reader);
    // readFileComp(fname + "c", defScope, state, reader);
}

Stmt::Stmt(int line_no)
    : file_name("(eval)"), line_no(line_no), location(true) {}

int Stmt::line() {
    return line_no;
}

void Stmt::disableLocationInformation() {
    location = false;
}

void Stmt::stateLine(InstrSeq& seq) {
    if (!location)
        return;
    InstrSeq seq0 = asmCode(makeAssemblerLine(Instr::LOCLN, line_no));
    seq.insert(seq.end(), seq0.begin(), seq0.end());
}

void Stmt::stateFile(InstrSeq& seq) {
    if (!location)
        return;
    InstrSeq seq0 = asmCode(makeAssemblerLine(Instr::LOCFN, file_name));
    seq.insert(seq.end(), seq0.begin(), seq0.end());
}

void Stmt::propogateFileName(std::string name) {
    file_name = name;
}

StmtCall::StmtCall(int line_no, unique_ptr<Stmt>& cls, const string& func, ArgList& arg)
    : Stmt(line_no), className(move(cls)), functionName(func), args(move(arg)) {}

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

    // Store the class name object
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);

    // "Evaluate" the name to lookup (may incur `missing`)
    (makeAssemblerLine(Instr::SYM, functionName)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);

    // Store the method/slot object
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);

    // Evaluate each of the arguments, in order
    for (auto& arg : args) {
        arg->translate(unit, seq);
        (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);
    }

    // Make the call
    (makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::CALL, (long)args.size())).appendOnto(seq);

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
        mthd = asmCode(makeAssemblerLine(Instr::YLD, Lit::NIL, Reg::RET));
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
    (makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET)).appendOnto(seq);

    // TODO Find a way to protect the `closure` field here.

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

    //stateLine(seq);

    // Find the literal object to use
    (makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::PTR)).appendOnto(seq);

    // Clone and put a prim() onto it
    (makeAssemblerLine(Instr::NUM, value)).appendOnto(seq);
    (makeAssemblerLine(Instr::LOAD, Reg::NUM0)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET)).appendOnto(seq);

}

StmtString::StmtString(int line_no, const char* contents)
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

StmtSymbol::StmtSymbol(int line_no, const char* contents)
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

    //stateLine(seq);

    // Find the literal object to use
    (makeAssemblerLine(Instr::GETL, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "meta")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "brackets")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);

    // Call the `brackets` function properly
    (makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
    (makeAssemblerLine(Instr::CALL, 0L)).appendOnto(seq);
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);

    // Add all of the elements
    for (auto& arg : args) {

        // Evaluate the argument
        arg->translate(unit, seq);
        (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);

        // Grab `next` and call it
        (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
        (makeAssemblerLine(Instr::SYM, "next")).appendOnto(seq);
        (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
        (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
        (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
        (makeAssemblerLine(Instr::CALL, 1L)).appendOnto(seq);

    }

    // Now call finish
    (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "finish")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
    (makeAssemblerLine(Instr::CALL, 0L)).appendOnto(seq);

    // Leave %sto the way we found it
    (makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO)).appendOnto(seq);

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

StmtHashParen::StmtHashParen(int line_no, string text)
    : Stmt(line_no), text(text) {}

void StmtHashParen::translate(TranslationUnit& unit, InstrSeq& seq) {

    //stateLine(seq);

    // Find the `hashParen` object
    (makeAssemblerLine(Instr::GETL, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "meta")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "hashParen")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);

    // Evaluate the argument
    InstrSeq arg0 = garnishSeq(text);
    seq.insert(seq.end(), arg0.begin(), arg0.end());
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);

    // Get the object back out
    (makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::PTR)).appendOnto(seq);
    (makeAssemblerLine(Instr::CALL, 1L)).appendOnto(seq);

}

StmtZeroDispatch::StmtZeroDispatch(int line_no, char sym, char ch, string text)
    : Stmt(line_no), text(text), symbol(sym), prefix(ch) {}

void StmtZeroDispatch::translate(TranslationUnit& unit, InstrSeq& seq) {

    //stateLine(seq);

    // Find the `radix` object
    (makeAssemblerLine(Instr::GETL, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "meta")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "radix")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);

    string contents = text;
    Symbolic prefix = Symbols::get()[ this->prefix == '\0' ? "" : string(1, this->prefix) ];

    InstrSeq seq0 = garnishSeq(contents);
    seq.insert(seq.end(), seq0.begin(), seq0.end());
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);

    InstrSeq seq1 = garnishSeq(prefix);
    seq.insert(seq.end(), seq1.begin(), seq1.end());
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);

    (makeAssemblerLine(Instr::SYM, string(1, this->symbol))).appendOnto(seq);
    (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);

    (makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
    (makeAssemblerLine(Instr::CALL, 2L)).appendOnto(seq);

}

StmtSpecialMethod::StmtSpecialMethod(int line_no, std::list< std::shared_ptr<Stmt> >& contents)
    : Stmt(line_no), contents(move(contents)) {}

void StmtSpecialMethod::translate(TranslationUnit& unit, InstrSeq& seq) {

    //stateLine(seq);

    // PIZZA

    auto prefix = [this, &unit](const InstrSeq& arg) {
        InstrSeq result;
        // Find the literal object to use
        (makeAssemblerLine(Instr::YLDC, Lit::METHOD, Reg::PTR)).appendOnto(result);
        // Translate the method sequence
        InstrSeq mthd;
        stateLine(mthd); // TODO These are overwritten if arg is empty (same in StmtMethod)
        stateFile(mthd);
        if (arg.empty()) {
            // If the method is empty, it defaults to `meta Nil.`
            mthd = asmCode(makeAssemblerLine(Instr::YLD, Lit::NIL, Reg::RET));
        } else {
            mthd.insert(mthd.end(), arg.begin(), arg.end());
        }
        (makeAssemblerLine(Instr::RET)).appendOnto(mthd);
        // Register with the translation unit
        FunctionIndex index = unit.pushMethod(mthd);
        // Clone and put a prim() onto it
        (makeAssemblerLine(Instr::MTHD, index)).appendOnto(result);
        (makeAssemblerLine(Instr::LOAD, Reg::MTHD)).appendOnto(result);
        (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(result);
        // Give the object an appropriate closure
        (makeAssemblerLine(Instr::GETL, Reg::PTR)).appendOnto(result);
        (makeAssemblerLine(Instr::SYM, "closure")).appendOnto(result);
        (makeAssemblerLine(Instr::SETF)).appendOnto(result);
        (makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET)).appendOnto(result);
        return result;
    };

    // Find the literal object to use
    (makeAssemblerLine(Instr::GETL, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "meta")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "statements")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);

    // Call the `statements` function properly
    (makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
    (makeAssemblerLine(Instr::CALL, 0L)).appendOnto(seq);
    (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq);

    // Add all of the elements
    for (auto& arg : contents) {

        // Evaluate the argument
        InstrSeq arg0;
        arg->translate(unit, arg0);

        InstrSeq temp = prefix(arg0);
        seq.insert(seq.end(), temp.begin(), temp.end());
        (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);

        // Grab `next` and call it
        (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
        (makeAssemblerLine(Instr::SYM, "next")).appendOnto(seq);
        (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
        (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
        (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
        (makeAssemblerLine(Instr::CALL, 1L)).appendOnto(seq);

    }

    // Now call finish
    (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::SYM, "finish")).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
    (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
    (makeAssemblerLine(Instr::CALL, 0L)).appendOnto(seq);

    // Leave %sto the way we found it
    (makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO)).appendOnto(seq);

}

void StmtSpecialMethod::propogateFileName(std::string name) {
    for (auto& ptr : contents)
        ptr->propogateFileName(name);
    Stmt::propogateFileName(name);
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

StmtHeld::StmtHeld(int line_no, unique_ptr<Stmt>& cls, const string& func)
    : Stmt(line_no), className(move(cls)), functionName(func) {}

void StmtHeld::translate(TranslationUnit& unit, InstrSeq& seq) {

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

    // "Evaluate" the name to lookup (may incur `missing`)
    (makeAssemblerLine(Instr::SYM, functionName)).appendOnto(seq);
    (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq);
    (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);

}

void StmtHeld::propogateFileName(std::string name) {
    if (className)
        className->propogateFileName(name);
    Stmt::propogateFileName(name);
}
