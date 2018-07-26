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

/// \file
///
/// \brief Data structures and algorithms for translating and compiling the AST.

/// A deleter for pointers to Expr or List values. This deleter will
/// recursively free all of the memory associated with the entire tree
/// pointed to by an Expr or List node.
class ExprDeleter {
public:

    /// Frees the expression and all of its components recursively.
    ///
    /// \param x the expression
    void operator()(Expr* x) const;

    /// Frees the list and all of its elements recursively.
    ///
    /// \param x the list
    void operator()(List* x) const;

};

/// A unique pointer to an Expr, with correct deletion semantics.
using PtrToExpr = std::unique_ptr< Expr, ExprDeleter >;

/// A unique pointer to a List, with correct deletion semantics.
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

/// Sets the global variable associated with the current snippet of
/// code. The global current line variable is a unique pointer, and it
/// will <em>take ownership</em> of the argument.
///
/// This function (and, by extension, getCurrentLine()) is designed
/// entirely to communicate with the C parser API.
///
/// \param stmt a list of statements to be placed in the variable
extern "C" void setCurrentLine(List* stmt);

/// Gets the value of the global variable associated with the current
/// snippet. This function returns a unique pointer which takes
/// ownership of the data, so the global variable will be left
/// pointing at nothing after a call to this function.
///
/// \return the global snippet
PtrToList getCurrentLine();

/// Translates the global current line into a list of Stmt objects. If
/// there is no global current line, returns an empty list. This
/// function, unlike getCurrentLine(), does not take ownership of the
/// global line and only borrows it, so the current value of the line
/// is unchanged.
///
/// \param table the operator table
/// \return the translated statements, or the empty list
std::list< std::unique_ptr<Stmt> > translateCurrentLine(const OperatorTable& table);

/// Clears the global current line, freeing any memory associated with
/// it and pointing it an nullptr.
void clearCurrentLine() noexcept;

/// Parses the given text as a sequence of Latitude expressions and
/// compiles it into an AST made up of Stmt objects. If a parse error
/// occurs, a ParseError exception will be raised.
///
/// \param table the operator precedence table
/// \param filename the name of the file, used for error reporting
/// \param str the Latitude code to parse
/// \return the AST representing the code
/// \throw ParseError if any part of the parsing process fails
std::list< std::unique_ptr<Stmt> > parse(const OperatorTable& table,
                                         std::string filename,
                                         std::string str);

/// Parses and compiles a string of Latitude code. The resulting
/// instructions are placed in the VM's execution context to be
/// executed next. If an error occurs during parsing, instructions to
/// raise the same exception within the Latitude VM will be placed in
/// the execution context.
///
/// The evaluated code will not directly change the lexical or dynamic
/// scope. That is, the resulting code will be run in whatever scope
/// was currently in use before the eval() call was made.
///
/// Note that eval() adds additional instructions to the resulting
/// compiled bytecode, for handling the translation unit. Unlike
/// readFileSource(), eval() does \a not append a return instruction.
///
/// \param vm the VM state
/// \param table the operator precedence table
/// \param str the Latitude code to parse and run
/// \return False if a Latitude parse exception is raised, true if successful
bool eval(VMState& vm,
          const OperatorTable& table,
          std::string str);

/// Parses and compiles a file of Latitude code. The resulting
/// instructions will be placed in the VM's execution context to be
/// executed next. Any errors during parsing will be raised as
/// exception within the Latitude VM.
///
/// The evaluated code will run in the lexical scope given by
/// `defScope` and a clone of the current dynamic scope.
///
/// Note that readFileSource() adds additional instructions for
/// handling the translation unit and stack traces, as well as
/// appending a return instruction to the end. This is done to make
/// files containing Latitude code behave somewhat like Latitude
/// methods.
///
/// \param fname the name of the file
/// \param defScope the scope in which to run the Latitude code
/// \param vm the VM state
/// \param table the operator precedence table
/// \return False if a Latitude exception is raised, true if successful
bool readFileSource(std::string fname,
                    Scope defScope,
                    VMState& vm,
                    const OperatorTable& table);

/// Parses and compiles a file of Latitude code. The resulting
/// instructions will be stored in the file with the given name as
/// binary data.
///
/// Note that compileFile() adds additional instructions for handling
/// the translation unit and stack traces, as well as appending a
/// return instruction to the end. This is done to make files
/// containing Latitude code behave somewhat like Latitude methods.
///
/// \param fname the name of the source file
/// \param fname1 the name of the binary file to output to
/// \param vm the VM state
/// \param table the operator precedence table
/// \return False if a Latitude exception is raised, true if successful
bool compileFile(std::string fname,
                 std::string fname1,
                 VMState& vm,
                 const OperatorTable& table);

/// Parses and compiles (if necessary) a file of Latitude code. The
/// resulting instructions will be placed in the VM's execution
/// context to be executed next. Any errors during parsing will be
/// raised as exception within the Latitude VM. If the file has
/// already been compiled and has not been modified recently, the
/// compiled code will be loaded. Otherwise, the source file will be
/// read and compiled, and its bytecode will be placed in the VM's
/// execution context and also outputted to a bytecode file with the
/// same path as the source file but with a 'c' appended to the end of
/// the filename.
///
/// The evaluated code will run in the lexical scope given by
/// `defScope` and a clone of the current dynamic scope.
///
/// Note that readFile() adds additional instructions for handling the
/// translation unit and stack traces, as well as appending a return
/// instruction to the end. This is done to make files containing
/// Latitude code behave somewhat like Latitude methods.
///
/// \param fname the name of the file
/// \param defScope the scope in which to run the Latitude code
/// \param vm the VM state
/// \param table the operator precedence table
/// \return False if a Latitude exception is raised, true if successful
bool readFile(std::string fname,
              Scope defScope,
              VMState& vm,
              const OperatorTable& table);

/// Reads the Latitude header from either the compiled bytecode file
/// or the source file, as appropriate.
///
/// \param filename the source file name
/// \return the header structure
/// \throw HeaderError if the header is malformed
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

/// A slot access statement, which may or may not be a method call.
class StmtCall : public Stmt {
public:
    /// The type of the argument list for a StmtCall object.
    typedef std::list< std::unique_ptr<Stmt> > ArgList;
private:
    std::unique_ptr<Stmt> className;
    std::string functionName;
    ArgList args;
    bool performCall;
public:
    /// Constructs StmtCall.
    ///
    /// Note that the `cls` argument defaults to the current scope
    /// (lexical or dynamic, depending on the name of the variable) if
    /// it is not provided.
    ///
    /// \param line_no the line number
    /// \param cls the caller object, or nullptr if implied
    /// \param func the name of the slot
    /// \param arg the argument list
    /// \param hasArgs whether or not methods should be invoked
    StmtCall(int line_no,
             std::unique_ptr<Stmt>& cls,
             const std::string& func,
             ArgList& arg,
             bool hasArgs);
    virtual void translate(TranslationUnit&, InstrSeq&);
    virtual void propogateFileName(std::string name);
};

/// An assignment statement.
class StmtEqual : public Stmt {
private:
    std::unique_ptr<Stmt> className;
    std::string functionName;
    std::unique_ptr<Stmt> rhs;
public:
    /// Constructs a StmtEqual.
    ///
    /// Note that the `cls` argument defaults to the current scope
    /// (lexical or dynamic, depending on the name of the variable) if
    /// it is not provided.
    ///
    /// \param line_no the line number
    /// \param cls the caller object, or nullptr if implied
    /// \param func the name of the slot
    /// \param asn the right-hand-side
    StmtEqual(int line_no,
              std::unique_ptr<Stmt>& cls,
              const std::string& func,
              std::unique_ptr<Stmt>& asn);
    virtual void translate(TranslationUnit&, InstrSeq&);
    virtual void propogateFileName(std::string name);
};

/// A method literal.
class StmtMethod : public Stmt {
private:
    std::list< std::shared_ptr<Stmt> > contents;
public:
    /// Constructs a StmtMethod.
    ///
    /// \param line_no the line number
    /// \param contents the body of the method
    StmtMethod(int line_no, std::list< std::shared_ptr<Stmt> >& contents);
    virtual void translate(TranslationUnit&, InstrSeq&);
    virtual void propogateFileName(std::string name);
};

/// A floating-point numeric literal.
class StmtNumber : public Stmt {
private:
    double value;
public:
    /// Constructs a StmtNumber.
    ///
    /// \param line_no the line number
    /// \param value the floating-point value
    StmtNumber(int line_no, double value);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/// A (fixed) integer numeric literal.
class StmtInteger : public Stmt {
private:
    long value;
public:
    /// Constructs a StmtInteger.
    ///
    /// \param line_no the line number
    /// \param value the integer value
    StmtInteger(int line_no, long value);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/// A (variable-length) integer numeric literal.
class StmtBigInteger : public Stmt {
private:
    std::string value;
public:
    /// Constructs a StmtBigInteger.
    ///
    /// The value should be a string consisting of an optional sign
    /// followed only by digits. It will always be assumed to be radix
    /// 10, even if there are leading zeroes.
    ///
    /// \param line_no the line number
    /// \param value the integer value, as a string of digits
    StmtBigInteger(int line_no, const char* value);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/// A string literal.
class StmtString : public Stmt {
private:
    std::string value;
public:
    /// Constructs a StmtString.
    ///
    /// \param line_no the line number
    /// \param contents a valid UTF-8 string
    StmtString(int line_no, std::string contents);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/// A symbolic literal.
class StmtSymbol : public Stmt {
private:
    std::string value;
public:
    /// Constructs a StmtSymbol.
    ///
    /// \param line_no the line number
    /// \param contents the symbol's name, as a valid UTF-8 string
    StmtSymbol(int line_no, std::string contents);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/// An inline list.
///
/// Evaluates to an Array object in Latitude.
class StmtList : public Stmt {
public:
    /// The type of the constituent list for a StmtList object.
    typedef std::list< std::unique_ptr<Stmt> > ArgList;
private:
    ArgList args;
public:
    /// Constructs a StmtList.
    ///
    /// \param line_no the line number
    /// \param arg the list of elements
    StmtList(int line_no, ArgList& arg);
    virtual void translate(TranslationUnit&, InstrSeq&);
    virtual void propogateFileName(std::string name);
};

/// A sigil, syntax sugar for some function calls.
class StmtSigil : public Stmt {
private:
    std::string name;
    std::unique_ptr<Stmt> rhs;
public:
    /// Constructs a StmtSigil.
    ///
    /// \param line_no the line number
    /// \param name the sigil's name, a valid UTF-8 string
    /// \param rhs the sigil's sole argument
    StmtSigil(int line_no, std::string name, std::unique_ptr<Stmt> rhs);
    virtual void translate(TranslationUnit&, InstrSeq&);
    virtual void propogateFileName(std::string name);
};

/// A (variable-length) integer numeric literal with radix.
///
/// This type is used for numerical literals where the radix is
/// explicitly specified. The radix may or may not be 10.
class StmtZeroDispatch : public Stmt {
private:
    std::string text;
    char symbol;
    char prefix;
public:
    /// Constructs a StmtZeroDispatch.
    ///
    /// \param line_no the line number
    /// \param sym the radix, as one of D (10), O (8), X (16), or B (2), case insensitive
    /// \param ch the sign, one of '+', '-', or '\0'
    /// \param text the string of digits which make up the number
    StmtZeroDispatch(int line_no, char sym, char ch, std::string text);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/// A complex numeric literal.
class StmtComplex : public Stmt {
private:
    double rl, im;
public:
    /// Constructs a StmtComplex.
    ///
    /// \param line_no the line number
    /// \param lhs the real part
    /// \param rhs the imaginary part
    StmtComplex(int line_no, double lhs, double rhs);
    virtual void translate(TranslationUnit&, InstrSeq&);
};

/// An assignment statement with the double-colon syntax sugar.
class StmtDoubleEqual : public Stmt {
private:
    std::unique_ptr<Stmt> className;
    std::string functionName;
    std::unique_ptr<Stmt> rhs;
public:
    /// Constructs a StmtDoubleEqual.
    ///
    /// Note that the `cls` argument defaults to the current scope
    /// (lexical or dynamic, depending on the name of the variable) if
    /// it is not provided.
    ///
    /// \param line_no the line number
    /// \param cls the caller object, or nullptr if implied
    /// \param func the name of the slot
    /// \param asn the right-hand-side
    StmtDoubleEqual(int line_no,
                    std::unique_ptr<Stmt>& cls,
                    const std::string& func,
                    std::unique_ptr<Stmt>& asn);
    virtual void translate(TranslationUnit&, InstrSeq&);
    virtual void propogateFileName(std::string name);
};

/// An inline dictionary.
///
/// Evaluates to an Dict object in Latitude.
class StmtDict : public Stmt {
public:
    /// The type of the constituent key-value pair list for a StmtDict object.
    typedef std::list< std::pair< std::unique_ptr<Stmt>, std::unique_ptr<Stmt> > > KVList;
private:
    KVList args;
public:
    /// Constructs a StmtDict.
    ///
    /// \param line_no the line number
    /// \param arg the list of elements
    StmtDict(int line_no, KVList& arg);
    virtual void translate(TranslationUnit&, InstrSeq&);
    virtual void propogateFileName(std::string name);
};

#endif // READER_HPP
