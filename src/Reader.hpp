//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef READER_HPP
#define READER_HPP
extern "C" {
#include "Parser.tab.h"
}
#include "Bytecode.hpp"
#include "Proto.hpp"
#include "Precedence.hpp"
#include "Header.hpp"
#include <memory>
#include <functional>
#include <list>

class ExprDeleter {
public:
    void operator()(Expr* x) const;
    void operator()(List* x) const;
};

using PtrToExpr = std::unique_ptr< Expr, ExprDeleter >;
using PtrToList = std::unique_ptr< List, ExprDeleter >;

class Stmt;

/// \brief A scope consists of a lexical scoping object and a dynamic
/// scoping object.
struct Scope {

    /// \brief The lexical scope.
    ObjectPtr lex;

    /// \brief The dynamic scope.
    ObjectPtr dyn;


};

extern "C" void setCurrentLine(List* stmt);
PtrToList getCurrentLine();
std::list< std::unique_ptr<Stmt> > translateCurrentLine(const OperatorTable& table);
void clearCurrentLine() noexcept;

/*
 * Parses the string. Will throw a ParseError as an exception if a
 * parse error occurs.
 */
std::list< std::unique_ptr<Stmt> > parse(const OperatorTable& table, std::string filename, std::string str);

bool eval(IntState& state,
          const ReadOnlyState& reader,
          const OperatorTable& table,
          std::string str);

bool readFileSource(std::string fname,
                    Scope defScope,
                    IntState& state,
                    const ReadOnlyState& reader,
                    const OperatorTable& table);

bool compileFile(std::string fname,
                 std::string fname1,
                 IntState& state,
                 const ReadOnlyState& reader,
                 const OperatorTable& table);

bool readFile(std::string fname,
              Scope defScope,
              IntState& state,
              const ReadOnlyState& reader,
              const OperatorTable& table);

// Throws HeaderError
Header getFileHeader(std::string filename);

/// This is the base class of all statement classes in Latitude.
/// Statements are expected to store basic information about the
/// location (line number and file name) of the original statement in
/// the Latitude source, and they are expected to be able to translate
/// statements into a sequence of VM instructions.
///
/// The statement class itself is abstract. Its only pure virtual
/// method is Stmt::translate.
class Stmt {
private:
    std::string file_name;
    int line_no;
    bool location;

protected:

    /// Requests that the object not store location information (line
    /// number and file name, specifically) in the compiled output.
    /// This operation is idempotent, so calling this method on an
    /// instance whose location information is already blocked will
    /// have no effect.
    void disableLocationInformation();

    /// Outputs the line number of the statement to the argument
    /// sequence. This method has no effect if location information
    /// has been disabled.
    ///
    /// \param seq the sequence to output to
    void stateLine(InstrSeq& seq) const;

    /// Outputs the file name of the statement to the argument
    /// sequence. This method has no effect if location information
    /// has been disabled.
    ///
    /// \param seq the sequence to output to
    void stateFile(InstrSeq& seq) const;

public:

    /// Constructs a statement, given the line number of the statement.
    ///
    /// \param line_no the line number
    Stmt(int line_no);

    /// Returns the line number, as passed into the constructor.
    ///
    /// \return the line number
    int line();

    /// Translates the current statement into a sequence of
    /// instructions, outputting the sequence onto the sequence
    /// argument. If necessary, the translation unit may be modified
    /// to include new behaviors.
    ///
    /// \param unit the translation unit
    /// \param seq the instruction sequence
    virtual void translate(TranslationUnit& unit, InstrSeq& seq) = 0;

    /// By default, the file name is stored only at the top-level
    /// statement object, the root of the AST. In order to propogate
    /// the file name downward into the AST, it is necessary to
    /// recursively call this method on each statement to set the file
    /// name.
    ///
    /// This method should be overridden on any statement which stores
    /// other statements and should propogate the file name downward
    /// recursively.
    ///
    /// \param name the file name
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
    bool performCall;
public:
    StmtCall(int line_no,
             std::unique_ptr<Stmt>& cls,
             const std::string& func,
             ArgList& arg,
             bool hasArgs);
    virtual void translate(TranslationUnit&, InstrSeq&);
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
    virtual void translate(TranslationUnit&, InstrSeq&);
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
    virtual void translate(TranslationUnit&, InstrSeq&);
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
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/*
 * A numeric integer literal.
 */
class StmtInteger : public Stmt {
private:
    long value;
public:
    StmtInteger(int line_no, long value);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/*
 * A numeric integer literal.
 */
class StmtBigInteger : public Stmt {
private:
    std::string value;
public:
    StmtBigInteger(int line_no, const char* value);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/*
 * A string literal.
 */
class StmtString : public Stmt {
private:
    std::string value;
public:
    StmtString(int line_no, std::string contents);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/*
 * A symbolic literal.
 */
class StmtSymbol : public Stmt {
private:
    std::string value;
public:
    StmtSymbol(int line_no, std::string contents);
    virtual void translate(TranslationUnit&, InstrSeq&);
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
    virtual void translate(TranslationUnit&, InstrSeq&);
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
    virtual void translate(TranslationUnit&, InstrSeq&);
    virtual void propogateFileName(std::string name);
};

/*
 * A zero-dispatch syntax, usually used to represent numerals
 * of different radices
 */
class StmtZeroDispatch : public Stmt {
private:
    std::string text;
    char symbol;
    char prefix;
public:
    StmtZeroDispatch(int line_no, char sym, char ch, std::string text);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/*
 * A special method literal.
 */
class StmtSpecialMethod : public Stmt {
private:
    std::list< std::shared_ptr<Stmt> > contents;
public:
    StmtSpecialMethod(int line_no, std::list< std::shared_ptr<Stmt> >& contents);
    virtual void translate(TranslationUnit&, InstrSeq&);
    virtual void propogateFileName(std::string name);
};

/*
 * A complex number literal.
 */
class StmtComplex : public Stmt {
private:
    double rl, im;
public:
    StmtComplex(int line_no, double lhs, double rhs);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/*
 * An assignment statement with the double-colon syntax sugar.
 */
class StmtDoubleEqual : public Stmt {
private:
    std::unique_ptr<Stmt> className;
    std::string functionName;
    std::unique_ptr<Stmt> rhs;
public:
    StmtDoubleEqual(int line_no,
                    std::unique_ptr<Stmt>& cls, const std::string& func,
                    std::unique_ptr<Stmt>& asn);
    virtual void translate(TranslationUnit&, InstrSeq&);
    virtual void propogateFileName(std::string name);
};

/*
 * An inline dictionary, which by default evaluates to a Dict object.
 */
class StmtDict : public Stmt {
public:
    typedef std::list< std::pair< std::unique_ptr<Stmt>, std::unique_ptr<Stmt> > > KVList;
private:
    KVList args;
public:
    StmtDict(int line_no, KVList& arg);
    virtual void translate(TranslationUnit&, InstrSeq&);
    virtual void propogateFileName(std::string name);
};

#endif // READER_HPP
