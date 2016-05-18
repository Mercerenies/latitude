#include "Standard.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include "GC.hpp"
#include "Cont.hpp"
#include <list>
#include <sstream>
#include <fstream>
#include <boost/scope_exit.hpp>

using namespace std;

///// Check hashes.txt; it's a lengthy to-do list for you.

// TODO More objects should have toString so they don't all default to showing "Object"

// TODO Make primitive objects like String and Number clone properly (prim() fields don't clone)

// TODO Some syntax sugar for pattern matching key-value pairs
//    (So we can do a `capture3` which returns three values)
//    (Or maybe a "variable bomb" method which introduces variables into the local scope)

void spawnSystemCalls(ObjectPtr& global, ObjectPtr& systemCall, ObjectPtr& sys);

ProtoError doSystemArgError(Scope scope, string name, int expected, int got);
ProtoError doSlotError(Scope scope, ObjectPtr problem, Symbolic slotName);
ProtoError doParseError(Scope scope);
ProtoError doParseError(Scope scope, string message);
ProtoError doEtcError(Scope scope, string errorName, string msg);

ObjectPtr spawnObjects() {

    ObjectPtr object(GC::get().allocate());
    ObjectPtr meta(clone(object));
    ObjectPtr global(clone(object));

    ObjectPtr proc(clone(object));
    ObjectPtr method(clone(proc));
    ObjectPtr systemCall(clone(method));
    ObjectPtr number(clone(object));
    ObjectPtr string(clone(object));
    ObjectPtr symbol(clone(object));

    ObjectPtr cont(clone(proc));
    ObjectPtr contValidator(clone(object));

    ObjectPtr exception(clone(object));
    ObjectPtr systemError(clone(exception));

    ObjectPtr process(clone(object));
    ObjectPtr stream(clone(object));
    ObjectPtr stdout_(clone(stream));
    ObjectPtr stdin_(clone(stream));
    ObjectPtr stderr_(clone(stream));

    ObjectPtr array_(clone(object));

    ObjectPtr sys(clone(object));
    ObjectPtr sigil(clone(object));
    ObjectPtr kernel(clone(object));

    ObjectPtr nil(clone(object));
    ObjectPtr boolean(clone(object));
    ObjectPtr true_(clone(boolean));
    ObjectPtr false_(clone(boolean));

    // Meta calls for basic types
    meta.lock()->put(Symbols::get()["Object"], object);
    meta.lock()->put(Symbols::get()["Proc"], proc);
    meta.lock()->put(Symbols::get()["Method"], method);
    meta.lock()->put(Symbols::get()["Number"], number);
    meta.lock()->put(Symbols::get()["String"], string);
    meta.lock()->put(Symbols::get()["Symbol"], symbol);
    meta.lock()->put(Symbols::get()["Stream"], stream);
    meta.lock()->put(Symbols::get()["Process"], process);
    meta.lock()->put(Symbols::get()["SystemCall"], systemCall);
    meta.lock()->put(Symbols::get()["True"], true_);
    meta.lock()->put(Symbols::get()["False"], false_);
    meta.lock()->put(Symbols::get()["Nil"], nil);
    meta.lock()->put(Symbols::get()["Boolean"], boolean);
    meta.lock()->put(Symbols::get()["Cont"], cont);
    meta.lock()->put(Symbols::get()["ContValidator"], contValidator);
    meta.lock()->put(Symbols::get()["sys"], sys);
    meta.lock()->put(Symbols::get()["Exception"], exception);
    meta.lock()->put(Symbols::get()["SystemError"], systemError);
    meta.lock()->put(Symbols::get()["Array"], array_);
    meta.lock()->put(Symbols::get()["Kernel"], kernel);

    // Object is its own parent
    object.lock()->put(Symbols::get()["parent"], object);

    // Meta linkage
    meta.lock()->put(Symbols::get()["meta"], meta);
    object.lock()->put(Symbols::get()["meta"], meta);
    meta.lock()->put(Symbols::get()["sys"], sys);
    meta.lock()->put(Symbols::get()["sigil"], sigil);

    // This ensures that problems in the core.lat file (before exceptions are well-defined)
    // do not cause the entire program to crash
    meta.lock()->put(Symbols::get()["exceptions?"], false_);

    // Global variables not accessible in meta
    global.lock()->put(Symbols::get()["stdin"], stdin_);
    global.lock()->put(Symbols::get()["stderr"], stderr_);
    global.lock()->put(Symbols::get()["stdout"], stdout_);
    global.lock()->put(Symbols::get()["global"], global);

    // Method and system call properties
    spawnSystemCalls(global, systemCall, sys);

    // Prim Fields
    method.lock()->prim(Method());
    number.lock()->prim(0.0);
    string.lock()->prim("");
    symbol.lock()->prim(Symbols::get()[""]);
    contValidator.lock()->prim(weak_ptr<SignalValidator>());
    stdout_.lock()->prim(outStream());
    stdin_.lock()->prim(inStream());
    stderr_.lock()->prim(errStream());

    // Location and line number (necessary for error printing; nice to have anyway)
    meta.lock()->put(Symbols::get()["lineStorage"], garnish({global, global}, Symbols::gensym("STORE")));
    meta.lock()->put(Symbols::get()["fileStorage"], garnish({global, global}, Symbols::gensym("STORE")));

    // The core libraries
    evalFile("std/latitude.lat", { global, global }, { global, global });

    return global;
}

// TODO Stack protection (a variable which sets a finite limit to the stack depth to avoid overflow)

void spawnSystemCalls(ObjectPtr& global, ObjectPtr& systemCall, ObjectPtr& sys) {
    ObjectPtr callStreamIn(clone(systemCall));
    ObjectPtr callStreamOut(clone(systemCall));
    ObjectPtr callStreamPuts(clone(systemCall));
    ObjectPtr callStreamPutln(clone(systemCall));
    ObjectPtr callPrimToString(clone(systemCall));
    ObjectPtr callIfStatement(clone(systemCall));
    ObjectPtr callStreamDump(clone(systemCall));
    ObjectPtr callClone(clone(systemCall));
    ObjectPtr callGetSlot(clone(systemCall));
    ObjectPtr callHasSlot(clone(systemCall));
    ObjectPtr callPutSlot(clone(systemCall));
    ObjectPtr callGensym(clone(systemCall));
    ObjectPtr callGensymOf(clone(systemCall));
    ObjectPtr callCallCC(clone(systemCall));
    ObjectPtr callExitCC(clone(systemCall));
    ObjectPtr callInstanceof(clone(systemCall));
    ObjectPtr callTryStmt(clone(systemCall));
    ObjectPtr callThrowStmt(clone(systemCall));
    ObjectPtr callNumAdd(clone(systemCall));
    ObjectPtr callNumSub(clone(systemCall));
    ObjectPtr callNumMul(clone(systemCall));
    ObjectPtr callNumDiv(clone(systemCall));
    ObjectPtr callNumMod(clone(systemCall));
    ObjectPtr callNumPow(clone(systemCall));
    ObjectPtr callTripleEquals(clone(systemCall));
    ObjectPtr callPrimEquals(clone(systemCall));
    ObjectPtr callStringConcat(clone(systemCall));
    ObjectPtr callStreamRead(clone(systemCall));
    ObjectPtr callStreamReadChar(clone(systemCall));
    ObjectPtr callStreamEof(clone(systemCall));
    ObjectPtr callStreamClose(clone(systemCall));
    ObjectPtr callStreamFileOpen(clone(systemCall));
    ObjectPtr callScopeProtect(clone(systemCall));
    ObjectPtr callInvoke(clone(systemCall));
    ObjectPtr callOrigin(clone(systemCall));
    ObjectPtr callIntern(clone(systemCall));
    ObjectPtr callSymbolic(clone(systemCall));
    ObjectPtr callNumLevel(clone(systemCall));
    ObjectPtr callPrimLT(clone(systemCall));
    ObjectPtr callNatSym(clone(systemCall));
    ObjectPtr callLoop(clone(systemCall));
    ObjectPtr callLoadFile(clone(systemCall));
    ObjectPtr callStackDump(clone(systemCall));
    ObjectPtr callProcessCreate(clone(systemCall));
    ObjectPtr callProcessInStream(clone(systemCall));
    ObjectPtr callProcessOutStream(clone(systemCall));
    ObjectPtr callProcessErrStream(clone(systemCall));
    ObjectPtr callProcessExec(clone(systemCall));
    ObjectPtr callProcessFinished(clone(systemCall));
    ObjectPtr callProcessRunning(clone(systemCall));
    ObjectPtr callProcessExitCode(clone(systemCall));
    ObjectPtr callDoWithCallback(clone(systemCall));
    ObjectPtr callStringReplace(clone(systemCall));
    ObjectPtr callStringSubstring(clone(systemCall));
    ObjectPtr callStringLength(clone(systemCall));

    systemCall.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            return eval("meta Nil.", scope);
        });
    callStreamIn.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr stream;
            if (bindArguments(lst, stream)) {
                auto prim = stream->prim();
                if (auto call = boost::get<StreamPtr>(&prim))
                    return garnish(scope, (*call)->hasIn());
                else
                    return eval("meta False.", scope);
            } else {
                throw doSystemArgError(scope, "streamIn#", 1, lst.size());
            }
        });
    callStreamOut.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr stream;
            if (bindArguments(lst, stream)) {
                auto prim = stream->prim();
                if (auto call = boost::get<StreamPtr>(&prim))
                    return garnish(scope, (*call)->hasOut());
                else
                    return eval("meta False.", scope);
            } else {
                throw doSystemArgError(scope, "streamOut#", 1, lst.size());
            }
        });
    callStreamPuts.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr stream, obj;
            if (bindArguments(lst, stream, obj)) {
                auto stream0 = stream->prim();
                auto obj0 = obj->prim();
                if (auto stream1 = boost::get<StreamPtr>(&stream0)) {
                    if (auto obj1 = boost::get<string>(&obj0)) {
                        if (!(*stream1)->hasOut())
                            throw doEtcError(scope, "StreamError",
                                             "Stream not designated for output");
                        try {
                            (*stream1)->writeText(*obj1);
                            return eval("meta Nil.", scope);
                        } catch (ios_base::failure err) {
                            throw doEtcError(scope, "IOError", err.what());
                        }
                    } else {
                        throw doEtcError(scope, "StreamError",
                                         "Object to print is not a string");
                    }
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(scope, "streamPuts#", 2, lst.size());
            }
        });
    callStreamPutln.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr stream, obj;
            if (bindArguments(lst, stream, obj)) {
                auto stream0 = stream->prim();
                auto obj0 = obj->prim();
                if (auto stream1 = boost::get<StreamPtr>(&stream0)) {
                    if (auto obj1 = boost::get<string>(&obj0)) {
                        if (!(*stream1)->hasOut())
                            throw doEtcError(scope, "StreamError",
                                             "Stream not designated for output");
                        try {
                            (*stream1)->writeLine(*obj1);
                            return eval("meta Nil.", scope);
                        } catch (ios_base::failure err) {
                            throw doEtcError(scope, "IOError", err.what());
                        }
                    } else {
                        throw doEtcError(scope, "StreamError",
                                         "Object to print is not a string");
                    }
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(scope, "streamPutln#", 2, lst.size());
            }
        });
    callPrimToString.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                string result = primToString(obj);
                return garnish(scope, result);
            } else {
                throw doSystemArgError(scope, "primToString#", 1, lst.size());
            }
        });
    callIfStatement.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr cond, tr, fl;
            if (bindArguments(lst, cond, tr, fl)) {
                // These are unused except to verify that the prim() is correct
                auto tr_ = boost::get<Method>(&tr->prim());
                auto fl_ = boost::get<Method>(&fl->prim());
                if ((tr_ == NULL) || (fl_ == NULL))
                    throw doEtcError(scope, "TypeError",
                                     "If statement body is not a method");
                //
                auto definitelyTrue = eval("meta True.", scope).lock();
                if (cond == definitelyTrue)
                    return doCallWithArgs(scope, scope.lex, tr);
                else
                    return doCallWithArgs(scope, scope.lex, fl);
            } else {
                throw doSystemArgError(scope, "ifThenElse#", 3, lst.size());
            }
        });
    callStreamDump.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr stream, obj;
            if (bindArguments(lst, stream, obj)) {
                if (auto stream0 = boost::get<StreamPtr>(&stream->prim())) {
                    try {
                        dumpObject(scope, **stream0, obj);
                        return eval("meta Nil.", scope);
                    } catch (ios_base::failure err) {
                        throw doEtcError(scope, "IOError", err.what());
                    }
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(scope, "streamDump#", 2, lst.size());
            }
        });
    callClone.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                return clone(obj);
            } else {
                throw doSystemArgError(scope, "doClone#", 1, lst.size());
            }
        });
    callGetSlot.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj, slot;
            if (bindArguments(lst, obj, slot)) {
                if (auto sym = boost::get<Symbolic>(&slot->prim())) {
                    return getInheritedSlot(scope, obj, *sym);
                } else {
                    throw doEtcError(scope, "TypeError", "Slot name must be a symbol");
                }
            } else {
                throw doSystemArgError(scope, "accessSlot#", 2, lst.size());
            }
        });
    callHasSlot.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj, slot;
            if (bindArguments(lst, obj, slot)) {
                if (auto sym = boost::get<Symbolic>(&slot->prim())) {
                    return garnish(scope,
                                   hasInheritedSlot(scope, obj, *sym));
                } else {
                    return eval("meta False.", scope);
                }
            } else {
                throw doSystemArgError(scope, "checkSlot#", 2, lst.size());
            }
        });
    callPutSlot.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj, slot, value;
            if (bindArguments(lst, obj, slot, value)) {
                if (auto sym = boost::get<Symbolic>(&slot->prim())) {
                    obj->put(*sym, value);
                    return ObjectPtr(value);
                } else {
                    throw doEtcError(scope, "TypeError", "Slot name must be a symbol");
                }
            } else {
                throw doSystemArgError(scope, "putSlot#", 3, lst.size());
            }
        });
    callGensym.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr symbol;
            if (bindArguments(lst, symbol)) {
                ObjectPtr gen = clone(symbol);
                gen.lock()->prim( Symbols::gensym() );
                return gen;
            } else {
                throw doSystemArgError(scope, "gensym#", 1, lst.size());
            }
        });
    callGensymOf.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr symbol, name;
            if (bindArguments(lst, symbol, name)) {
                if (auto str = boost::get<std::string>(&name->prim())) {
                    ObjectPtr gen = clone(symbol);
                    gen.lock()->prim( Symbols::gensym(*str) );
                    return gen;
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Expected string gensym prefix");
                }
            } else {
                throw doSystemArgError(scope, "gensymOf#", 2, lst.size());
            }
        });
    callCallCC.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr mthd;
            if (bindArguments(lst, mthd)) {
                // Dies when it goes out of scope
                shared_ptr<SignalValidator> livingTag(new SignalValidator());
                //
                Symbolic sym = Symbols::gensym("CONT");
                ObjectPtr symObj = getInheritedSlot(scope,
                                                    meta(scope, scope.lex),
                                                    Symbols::get()["Symbol"]);
                ObjectPtr symObj1 = clone(symObj);
                symObj1.lock()->prim(sym);
                ObjectPtr validator = getInheritedSlot(scope,
                                                       meta(scope, scope.lex),
                                                       Symbols::get()["ContValidator"]);
                ObjectPtr validator1 = clone(validator);
                validator1.lock()->prim(weak_ptr<SignalValidator>(livingTag));
                // Define a callback to store the tagged validator in the dynamic scope of the method
                auto callback = [sym, validator1](Scope scope1) { scope1.dyn.lock()->put(sym, validator1); };
                ObjectPtr cont = getInheritedSlot(scope,
                                                  meta(scope, scope.lex),
                                                  Symbols::get()["Cont"]);
                ObjectPtr cont1 = clone(cont);
                cont1.lock()->put(Symbols::get()["tag"], symObj1);
                try {
                    return doCall(scope, scope.lex, mthd, { cont1 }, callback);
                } catch (Signal& signal) {
                    if (signal.match(sym)) {
                        return signal.getObject();
                    } else {
                        throw;
                    }
                }
            } else {
                throw doSystemArgError(scope, "callCC#", 1, lst.size());
            }
        });
    callExitCC.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr tag, arg;
            if (bindArguments(lst, tag, arg)) {
                if (auto sym = boost::get<Symbolic>(&tag->prim())) {
                    if (hasInheritedSlot(scope, scope.dyn, *sym)) {
                        auto validator = getInheritedSlot(scope, scope.dyn, *sym);
                        if (auto val =
                            boost::get< weak_ptr<SignalValidator> >(&validator.lock()
                                                                    ->prim())) {
                            if (val->expired())
                                throw doEtcError(scope, "ContError",
                                                 "Out of continuation bounds");
                            throw Signal(*sym, arg);
                        } else {
                            throw doEtcError(scope, "TypeError",
                                             "Invalid continuation in exitCC#");
                        }
                    } else {
                        throw doEtcError(scope, "ContError",
                                         "Out of continuation bounds");
                    }
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Invalid continuation in exitCC#");
                }
                return eval("meta Nil.", scope); // Should never happen
            } else {
                throw doSystemArgError(scope, "exitCC#", 2, lst.size());
            }
        });
    callInstanceof.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj0, obj1;
            if (bindArguments(lst, obj0, obj1)) {
                auto hier = hierarchy(obj0);
                bool result = (find_if(hier.begin(), hier.end(),
                                       [&obj1](auto& o){ return o.lock() == obj1; })
                               != hier.end());
                return garnish(scope, result);
            } else {
                throw doSystemArgError(scope, "instanceOf#", 2, lst.size());
            }
        });
    callTryStmt.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr lhs, cond, rhs;
            if (bindArguments(lst, lhs, cond, rhs)) {
                try {
                    return doCallWithArgs(scope, scope.lex, lhs);
                } catch (ProtoError& err) {
                    ObjectPtr result1 = doCallWithArgs(scope, scope.lex, cond, err.getObject());
                    auto definitelyTrue = eval("meta True.", scope).lock();
                    if (result1.lock() == definitelyTrue) {
                        return doCallWithArgs(scope, scope.lex, rhs, err.getObject());
                    } else {
                        throw;
                    }
                }
            } else {
                throw doSystemArgError(scope, "try#", 3, lst.size());
            }
        });
    callThrowStmt.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                throwProtoError(obj);
                return eval("meta Nil.", scope); // Should never happen
            } else {
                throw doSystemArgError(scope, "throw#", 1, lst.size());
            }
        });
    callNumAdd.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto n0 = boost::get<Number>(&obj1->prim());
                auto n1 = boost::get<Number>(&obj2->prim());
                if (n0 && n1)
                    return garnish(scope, *n0 + *n1);
                else
                    throw doEtcError(scope, "TypeError",
                                     "Got non-numeral in arithmetic operation");
            } else {
                throw doSystemArgError(scope, "numAdd#", 2, lst.size());
            }
        });
    callNumSub.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto n0 = boost::get<Number>(&obj1->prim());
                auto n1 = boost::get<Number>(&obj2->prim());
                if (n0 && n1)
                    return garnish(scope, *n0 - *n1);
                else
                    throw doEtcError(scope, "TypeError",
                                     "Got non-numeral in arithmetic operation");
            } else {
                throw doSystemArgError(scope, "numSub#", 2, lst.size());
            }
        });
    callNumMul.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto n0 = boost::get<Number>(&obj1->prim());
                auto n1 = boost::get<Number>(&obj2->prim());
                if (n0 && n1)
                    return garnish(scope, *n0 * *n1);
                else
                    throw doEtcError(scope, "TypeError",
                                     "Got non-numeral in arithmetic operation");
            } else {
                throw doSystemArgError(scope, "numMul#", 2, lst.size());
            }
        });
    callNumDiv.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto n0 = boost::get<Number>(&obj1->prim());
                auto n1 = boost::get<Number>(&obj2->prim());
                if (n0 && n1)
                    return garnish(scope, *n0 / *n1);
                else
                    throw doEtcError(scope, "TypeError",
                                     "Got non-numeral in arithmetic operation");
            } else {
                throw doSystemArgError(scope, "numDiv#", 2, lst.size());
            }
        });
    callNumMod.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto n0 = boost::get<Number>(&obj1->prim());
                auto n1 = boost::get<Number>(&obj2->prim());
                if (n0 && n1)
                    return garnish(scope, *n0 % *n1);
                else
                    throw doEtcError(scope, "TypeError",
                                     "Got non-numeral in arithmetic operation");
            } else {
                throw doSystemArgError(scope, "numMod#", 2, lst.size());
            }
        });
    callNumPow.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto n0 = boost::get<Number>(&obj1->prim());
                auto n1 = boost::get<Number>(&obj2->prim());
                if (n0 && n1)
                    return garnish(scope, (*n0).pow(*n1));
                else
                    throw doEtcError(scope, "TypeError",
                                     "Got non-numeral in arithmetic operation");
            } else {
                throw doSystemArgError(scope, "numPow#", 2, lst.size());
            }
        });
    callTripleEquals.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                return garnish(scope, obj1 == obj2);
            } else {
                throw doSystemArgError(scope, "ptrEquals#", 2, lst.size());
            }
        });
    callPrimEquals.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                return garnish(scope, primEquals(obj1, obj2));
            } else {
                throw doSystemArgError(scope, "primEquals#", 2, lst.size());
            }
        });
    callStringConcat.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto str1 = boost::get<std::string>(&obj1->prim());
                auto str2 = boost::get<std::string>(&obj2->prim());
                if (str1 && str2)
                    return garnish(scope, *str1 + *str2);
                else
                    throw doEtcError(scope, "TypeError",
                                     "Got non-string in stringConcat#");
            } else {
                throw doSystemArgError(scope, "stringConcat#", 2, lst.size());
            }
        });
    callStreamRead.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr stream;
            if (bindArguments(lst, stream)) {
                auto stream0 = stream->prim();
                if (auto stream1 = boost::get<StreamPtr>(&stream0)) {
                    if (!(*stream1)->hasIn())
                        throw doEtcError(scope, "StreamError",
                                         "Stream not designated for input");
                    try {
                        return garnish(scope, (*stream1)->readLine());
                    } catch (ios_base::failure err) {
                        throw doEtcError(scope, "IOError", err.what());
                    }
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(scope, "streamRead#", 1, lst.size());
            }
        });
    callStreamReadChar.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr stream;
            if (bindArguments(lst, stream)) {
                auto stream0 = stream->prim();
                if (auto stream1 = boost::get<StreamPtr>(&stream0)) {
                    if (!(*stream1)->hasIn())
                        throw doEtcError(scope, "StreamError",
                                         "Stream not designated for input");
                    try {
                        return garnish(scope, (*stream1)->readText(1));
                    } catch (ios_base::failure err) {
                        throw doEtcError(scope, "IOError", err.what());
                    }
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(scope, "streamReadChar#", 1, lst.size());
            }
        });
    callStreamEof.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr stream;
            if (bindArguments(lst, stream)) {
                auto stream0 = stream->prim();
                if (auto stream1 = boost::get<StreamPtr>(&stream0)) {
                    return garnish(scope, (*stream1)->isEof());
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(scope, "streamEof#", 1, lst.size());
            }
        });
    callStreamClose.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr stream;
            if (bindArguments(lst, stream)) {
                auto stream0 = stream->prim();
                if (auto stream1 = boost::get<StreamPtr>(&stream0)) {
                    (*stream1)->close();
                    return garnish(scope, boost::blank());
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(scope, "streamClose#", 1, lst.size());
            }
        });
    callStreamFileOpen.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr stream, filename, access, mode;
            if (bindArguments(lst, stream, filename, access, mode)) {
                auto newStream = clone(stream); // TODO Call clone in-language rather than forcing it
                auto filename0 = boost::get<string>(&filename->prim());
                auto access0 = boost::get<Symbolic>(&access->prim());
                auto mode0 = boost::get<Symbolic>(&mode->prim());
                if (filename0 && access0 && mode0) {
                    auto access1 = Symbols::get()[*access0];
                    auto mode1 = Symbols::get()[*mode0];
                    FileAccess access2;
                    FileMode mode2;
                    if (access1 == "read")
                        access2 = FileAccess::READ;
                    else if (access1 == "write")
                        access2 = FileAccess::WRITE;
                    else
                        throw doEtcError(scope, "SystemCallError",
                                         "Invalid access specifier when opening file");
                    if (mode1 == "text")
                        mode2 = FileMode::TEXT;
                    else if (mode1 == "binary")
                        mode2 = FileMode::BINARY;
                    else
                        throw doEtcError(scope, "SystemCallError",
                                         "Invalid mode specifier when opening file");
                    newStream.lock()->prim( StreamPtr(new FileStream(*filename0, access2, mode2)) );
                    return newStream;
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Type error while opening file");
                }
            } else {
                throw doSystemArgError(scope, "streamFileOpen#", 4, lst.size());
            }
        });
    callScopeProtect.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr block1, block2;
            if (bindArguments(lst, block1, block2)) {
                // Runs block1, then block2. Will run block2 even if
                // block1 throws or exits abnormally. block1 is called
                // with no args; block2 is called with a single Boolean,
                // which is true if the exit was "abnormal" (either an
                // exception, a continuation exit, or a deeper system
                // error). Note that if block2 attempts an abnormal exit that
                // would escape the scope on the scopeProtect# call, the behavior
                // is completely undefined.
                bool abnormal = true;
                BOOST_SCOPE_EXIT(&abnormal, block2, scope) {
                    try {
                        ObjectPtr status = garnish(scope, abnormal);
                        doCallWithArgs(scope, scope.lex, block2, status);
                    } catch (...) {
                        cerr << "Attempted abnormal exit from protected block!" << endl;
                        terminate();
                    }
                } BOOST_SCOPE_EXIT_END;
                ObjectPtr result = doCallWithArgs(scope, scope.lex, block1);
                abnormal = false;
                return result;
            } else {
                throw doSystemArgError(scope, "scopeProtect#", 2, lst.size());
            }
        });
    callInvoke.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr self, mthd;
            if (bindArguments(lst, self, mthd)) {
                // Calls the method with "self" bound to the given object,
                // bypassing the normal self. All arguments ($n) remain the
                // same.
                return doCallWithArgs(scope, self, mthd);
            } else {
                throw doSystemArgError(scope, "invoke#", 2, lst.size());
            }
        });
    callOrigin.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr self, ref;
            if (bindArguments(lst, self, ref)) {
                auto sym = boost::get<Symbolic>(&ref->prim());
                if (!sym)
                    throw doEtcError(scope, "TypeError",
                                     "Slot name must be a symbol");
                return getInheritedOrigin(scope, self, *sym);
            } else {
                throw doSystemArgError(scope, "origin#", 2, lst.size());
            }
        });
    callIntern.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr self;
            if (bindArguments(lst, self)) {
                if (auto str = boost::get<string>(&self->prim())) {
                    ObjectPtr sym = getInheritedSlot(scope,
                                                     meta(scope, scope.lex),
                                                     Symbols::get()["Symbol"]);
                    ObjectPtr sym1 = clone(sym);
                    sym1.lock()->prim(Symbols::get()[*str]);
                    return sym1;
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object to intern is not a string");
                }
            } else {
                throw doSystemArgError(scope, "intern#", 1, lst.size());
            }
        });
    callSymbolic.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr self;
            if (bindArguments(lst, self)) {
                if (auto sym = boost::get<Symbolic>(&self->prim())) {
                    return garnish(scope, Symbols::get()[*sym]);
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not symbolic");
                }
            } else {
                throw doSystemArgError(scope, "symbolic#", 1, lst.size());
            }
        });
    callNumLevel.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr self;
            if (bindArguments(lst, self)) {
                if (auto num = boost::get<Number>(&self->prim())) {
                    return garnish(scope, num->hierarchyLevel());
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not numerical");
                }
            } else {
                throw doSystemArgError(scope, "numLevel#", 1, lst.size());
            }
        });
    callPrimLT.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                return garnish(scope, primLT(obj1, obj2));
            } else {
                throw doSystemArgError(scope, "primLT#", 2, lst.size());
            }
        });
    callNatSym.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr nat;
            if (bindArguments(lst, nat)) {
                if (auto num = boost::get<Number>(&nat->prim())) {
                    long value = num->asSmallInt();
                    if (value > 0) {
                        Symbolic sym = Symbols::get().natural(value);
                        return garnish(scope, sym);
                    } else {
                        throw doEtcError(scope, "TypeError",
                                         "Cannot produce natural symbol from numbers <= 0");
                    }
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not numerical");
                }
            } else {
                throw doSystemArgError(scope, "natSym#", 1, lst.size());
            }
        });
    callLoop.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr mthd;
            if (bindArguments(lst, mthd)) {
                // These are unused except to verify that the prim() is correct
                auto mthd_ = boost::get<Method>(&mthd->prim());
                if (mthd_ == NULL)
                    throw doEtcError(scope, "TypeError",
                                     "Loop body is not a method");
                //
                while (true)
                    doCallWithArgs(scope, scope.lex, mthd);
                return eval("meta Nil.", scope); // Should never happen
            } else {
                throw doSystemArgError(scope, "loop#", 1, lst.size());
            }
        });
    callLoadFile.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr filename;
            if (bindArguments(lst, filename)) {
                if (auto fname = boost::get<string>(&filename->prim())) {
                    // TODO Should we take global as an argument or capture it like we are now?
                    ObjectPtr result = evalFile(*fname, { global, global }, scope);
                    return result;
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "String filename expected");
                }
            } else {
                throw doSystemArgError(scope, "loadFile#", 1, lst.size());
            }
        });
    callStackDump.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr dyn0;
            if (bindArguments(lst, dyn0)) {
                auto arr = hierarchy(dyn0);
                ostringstream str;
                for (ObjectPtr value : arr) {
                    if (grabFileName(scope, value) != "???") {
                        str << "'" << grabFileName(scope, value) << "'"
                            << ":" << grabLineNumber(scope, value) << endl;
                    }
                }
                return garnish(scope, str.str());
            } else {
                throw doSystemArgError(scope, "stackDump#", 1, lst.size());
            }
        });
    callProcessCreate.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr process, str;
            if (bindArguments(lst, process, str)) {
                ObjectPtr process0 = clone(process);
                if (auto string0 = boost::get<string>(&str->prim())) {
                    ProcessPtr proc = makeProcess(*string0);
                    if (!proc)
                        throw doEtcError(scope, "NotSupportedError",
                                         "Asynchronous processes not supported on this system");
                    process0.lock()->prim(proc);
                    return process0;
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "String command expected");
                }
            } else {
                throw doSystemArgError(scope, "processCreate#", 2, lst.size());
            }
        });
    callProcessInStream.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr process;
            if (bindArguments(lst, process)) {
                if (auto process0 = boost::get<ProcessPtr>(&process->prim())) {
                    return garnish(scope, (*process0)->stdIn());
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a process");
                }
            } else {
                throw doSystemArgError(scope, "processInStream#", 1, lst.size());
            }
        });
    callProcessOutStream.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr process;
            if (bindArguments(lst, process)) {
                if (auto process0 = boost::get<ProcessPtr>(&process->prim())) {
                    return garnish(scope, (*process0)->stdOut());
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a process");
                }
            } else {
                throw doSystemArgError(scope, "processOutStream#", 1, lst.size());
            }
        });
    callProcessErrStream.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr process;
            if (bindArguments(lst, process)) {
                if (auto process0 = boost::get<ProcessPtr>(&process->prim())) {
                    return garnish(scope, (*process0)->stdErr());
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a process");
                }
            } else {
                throw doSystemArgError(scope, "processErrStream#", 1, lst.size());
            }
        });
    callProcessExec.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr process;
            if (bindArguments(lst, process)) {
                if (auto process0 = boost::get<ProcessPtr>(&process->prim())) {
                    bool status = (*process0)->run();
                    if (status)
                        return garnish(scope, boost::blank());
                    else
                        throw doEtcError(scope, "IOError",
                                         "Could not start process");
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a process");
                }
            } else {
                throw doSystemArgError(scope, "processExec#", 1, lst.size());
            }
        });
    callProcessFinished.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr process;
            if (bindArguments(lst, process)) {
                if (auto process0 = boost::get<ProcessPtr>(&process->prim())) {
                    return garnish(scope, (*process0)->isDone());
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a process");
                }
            } else {
                throw doSystemArgError(scope, "processFinished#", 1, lst.size());
            }
        });
    callProcessRunning.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr process;
            if (bindArguments(lst, process)) {
                if (auto process0 = boost::get<ProcessPtr>(&process->prim())) {
                    return garnish(scope, (*process0)->isRunning());
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a process");
                }
            } else {
                throw doSystemArgError(scope, "processRunning#", 1, lst.size());
            }
        });
    callProcessExitCode.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr process;
            if (bindArguments(lst, process)) {
                if (auto process0 = boost::get<ProcessPtr>(&process->prim())) {
                    return garnish(scope, (*process0)->getExitCode());
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Object is not a process");
                }
            } else {
                throw doSystemArgError(scope, "processExitCode#", 1, lst.size());
            }
        });
    callDoWithCallback.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr self, mthd0, mthd1;
            if (bindArguments(lst, self, mthd0, mthd1)) {
                return doCall(scope, self, mthd0, {}, [&scope, &self, &mthd1](Scope scope1) {
                        doCallWithArgs(scope, self, mthd1, scope1.lex, scope1.dyn);
                    });
            } else {
                throw doSystemArgError(scope, "doWithCallback#", 3, lst.size());
            }
        });
    callStringReplace.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr str, substr, mthd;
            if (bindArguments(lst, str, substr, mthd)) {
                // Calls the method with a single argument (the index of the match start) to determine
                // the replacement at each step
                auto str0 = boost::get<string>(&str->prim());
                auto substr0 = boost::get<string>(&substr->prim());
                if (str0 && substr0) {
                    string str1 = *str0;
                    string substr1 = *substr0;
                    string result = "";
                    string buffer = "";
                    size_t i = 0;
                    size_t j = 0;
                    size_t startIndex = 0;
                    for (char ch : str1) {
                        if (ch == substr1[j]) {
                            // Match
                            buffer += ch;
                            if (j == 0)
                                startIndex = i;
                            j++;
                            if (j == substr1.length()) {
                                // Full match
                                j = 0;
                                buffer = "";
                                ObjectPtr newStr = doCallWithArgs(scope,
                                                                  str,
                                                                  mthd,
                                                                  garnish(scope, (long)startIndex));
                                if (auto newStr0 = boost::get<string>(&newStr.lock()->prim())) {
                                    result += *newStr0;
                                } else {
                                    throw doEtcError(scope, "TypeError",
                                                     "String expected in replacement");
                                }
                            }
                        } else {
                            // No match
                            j = 0;
                            result += buffer + ch;
                            buffer = "";
                        }
                        i++;
                    }
                    return garnish(scope, result);
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "String expected in replacement");
                }
            } else {
                throw doSystemArgError(scope, "stringReplace#", 3, lst.size());
            }
        });
    callStringSubstring.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr str, start, end;
            if (bindArguments(lst, str, start, end)) {
                auto str0 = boost::get<string>(&str->prim());
                auto start0 = boost::get<Number>(&start->prim());
                auto end0 = boost::get<Number>(&end->prim());
                if (str0 && start0 && end0) {
                    long start1 = (*start0).asSmallInt();
                    long end1 = (*end0).asSmallInt();
                    long size = (*str0).length();
                    if (start1 < 0)
                        start1 += size;
                    if (end1 < 0)
                        end1 += size;
                    if (start1 >= size)
                        start1 = size;
                    if (end1 >= size)
                        end1 = size;
                    if (start1 < 0)
                        start1 = 0;
                    if (end1 < 0)
                        end1 = 0;
                    long len = end1 - start1;
                    if (len < 0)
                        len = 0;
                    return garnish(scope, str0->substr(start1, len));
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "Invalid argument in substring access");
                }
            } else {
                throw doSystemArgError(scope, "stringSubstring#", 3, lst.size());
            }
        });
    callStringLength.lock()->prim([global](Scope scope, list<ObjectPtr> lst) {
            ObjectSPtr str;
            if (bindArguments(lst, str)) {
                if (auto str0 = boost::get<string>(&str->prim())) {
                    return garnish(scope, (long)(str0->length()));
                } else {
                    throw doEtcError(scope, "TypeError",
                                     "String expected in length check");
                }
            } else {
                throw doSystemArgError(scope, "stringLength#", 1, lst.size());
            }
        });

    sys.lock()->put(Symbols::get()["streamIn#"], callStreamIn);
    sys.lock()->put(Symbols::get()["streamOut#"], callStreamOut);
    sys.lock()->put(Symbols::get()["streamPuts#"], callStreamPuts);
    sys.lock()->put(Symbols::get()["streamPutln#"], callStreamPutln);
    sys.lock()->put(Symbols::get()["primToString#"], callPrimToString);
    sys.lock()->put(Symbols::get()["ifThenElse#"], callIfStatement);
    sys.lock()->put(Symbols::get()["streamDump#"], callStreamDump);
    sys.lock()->put(Symbols::get()["doClone#"], callClone);
    sys.lock()->put(Symbols::get()["accessSlot#"], callGetSlot);
    sys.lock()->put(Symbols::get()["checkSlot#"], callHasSlot);
    sys.lock()->put(Symbols::get()["putSlot#"], callPutSlot);
    sys.lock()->put(Symbols::get()["gensym#"], callGensym);
    sys.lock()->put(Symbols::get()["gensymOf#"], callGensymOf);
    sys.lock()->put(Symbols::get()["callCC#"], callCallCC);
    sys.lock()->put(Symbols::get()["exitCC#"], callExitCC);
    sys.lock()->put(Symbols::get()["instanceOf#"], callInstanceof);
    sys.lock()->put(Symbols::get()["try#"], callTryStmt);
    sys.lock()->put(Symbols::get()["throw#"], callThrowStmt);
    sys.lock()->put(Symbols::get()["numAdd#"], callNumAdd);
    sys.lock()->put(Symbols::get()["numSub#"], callNumSub);
    sys.lock()->put(Symbols::get()["numMul#"], callNumMul);
    sys.lock()->put(Symbols::get()["numDiv#"], callNumDiv);
    sys.lock()->put(Symbols::get()["numMod#"], callNumMod);
    sys.lock()->put(Symbols::get()["numPow#"], callNumPow);
    sys.lock()->put(Symbols::get()["ptrEquals#"], callTripleEquals);
    sys.lock()->put(Symbols::get()["primEquals#"], callPrimEquals);
    sys.lock()->put(Symbols::get()["stringConcat#"], callStringConcat);
    sys.lock()->put(Symbols::get()["streamRead#"], callStreamRead);
    sys.lock()->put(Symbols::get()["streamReadChar#"], callStreamReadChar);
    sys.lock()->put(Symbols::get()["streamEof#"], callStreamEof);
    sys.lock()->put(Symbols::get()["streamClose#"], callStreamClose);
    sys.lock()->put(Symbols::get()["streamFileOpen#"], callStreamFileOpen);
    sys.lock()->put(Symbols::get()["scopeProtect#"], callScopeProtect);
    sys.lock()->put(Symbols::get()["invoke#"], callInvoke);
    sys.lock()->put(Symbols::get()["origin#"], callOrigin);
    sys.lock()->put(Symbols::get()["intern#"], callIntern);
    sys.lock()->put(Symbols::get()["symbolic#"], callSymbolic);
    sys.lock()->put(Symbols::get()["numLevel#"], callNumLevel);
    sys.lock()->put(Symbols::get()["primLT#"], callPrimLT);
    sys.lock()->put(Symbols::get()["natSym#"], callNatSym);
    sys.lock()->put(Symbols::get()["loop#"], callLoop);
    sys.lock()->put(Symbols::get()["loadFile#"], callLoadFile);
    sys.lock()->put(Symbols::get()["stackDump#"], callStackDump);
    sys.lock()->put(Symbols::get()["processCreate#"], callProcessCreate);
    sys.lock()->put(Symbols::get()["processInStream#"], callProcessInStream);
    sys.lock()->put(Symbols::get()["processOutStream#"], callProcessOutStream);
    sys.lock()->put(Symbols::get()["processErrStream#"], callProcessErrStream);
    sys.lock()->put(Symbols::get()["processExec#"], callProcessExec);
    sys.lock()->put(Symbols::get()["processFinished#"], callProcessFinished);
    sys.lock()->put(Symbols::get()["processRunning#"], callProcessRunning);
    sys.lock()->put(Symbols::get()["processExitCode#"], callProcessExitCode);
    sys.lock()->put(Symbols::get()["doWithCallback#"], callDoWithCallback);
    sys.lock()->put(Symbols::get()["stringReplace#"], callStringReplace);
    sys.lock()->put(Symbols::get()["stringSubstring#"], callStringSubstring);
    sys.lock()->put(Symbols::get()["stringLength#"], callStringLength);

}

// TODO Have a flag in `meta` somewhere to indicate whether exceptions have been loaded or not

bool areExceptionsEnabled(Scope scope) {
    ObjectPtr meta_ = meta(scope, scope.lex);
    ObjectPtr definitelyTrue = garnish(scope, true);
    ObjectPtr exc = getInheritedSlot(scope, meta_, Symbols::get()["exceptions?"]);
    return (exc.lock() == definitelyTrue.lock());
}

[[ noreturn ]] void gracefulTerminate(Scope scope, string category, string message) {
    std::cerr << "Fatal error! " << category << std::endl;
    std::cerr << message << std::endl;
    // Throw something that won't get caught until the top-level
    struct {} expr;
    throw expr;
}

ProtoError doSystemArgError(Scope scope,
                            string name,
                            int expected,
                            int got) {
    if (!areExceptionsEnabled(scope))
        gracefulTerminate(scope, "SystemArgError", name);
    ObjectPtr meta_ = meta(scope, scope.lex);
    ObjectPtr err = clone(getInheritedSlot(scope, meta_, Symbols::get()["SystemArgError"]));
    err.lock()->put(Symbols::get()["stack"], scope.dyn);
    err.lock()->put(Symbols::get()["gotArguments"], garnish(scope, got));
    err.lock()->put(Symbols::get()["expectedArguments"], garnish(scope, expected));
    err.lock()->put(Symbols::get()["functionName"], garnish(scope, name));
    return ProtoError(err);
}

ProtoError doSlotError(Scope scope, ObjectPtr problem, Symbolic slotName) {
    if (!areExceptionsEnabled(scope))
        gracefulTerminate(scope, "SlotError", Symbols::get()[slotName]);
    ObjectPtr meta_ = meta(scope, scope.lex);
    ObjectPtr err = clone(getInheritedSlot(scope, meta_, Symbols::get()["SlotError"]));
    err.lock()->put(Symbols::get()["stack"], scope.dyn);
    err.lock()->put(Symbols::get()["slotName"], garnish(scope, slotName));
    err.lock()->put(Symbols::get()["objectInstance"], problem);
    return ProtoError(err);
}

ProtoError doParseError(Scope scope) {
    if (!areExceptionsEnabled(scope))
        gracefulTerminate(scope, "ParseError", "");
    ObjectPtr meta_ = meta(scope, scope.lex);
    ObjectPtr err = clone(getInheritedSlot(scope, meta_, Symbols::get()["ParseError"]));
    err.lock()->put(Symbols::get()["stack"], scope.dyn);
    return ProtoError(err);
}

ProtoError doParseError(Scope scope, string message) {
    if (!areExceptionsEnabled(scope))
        gracefulTerminate(scope, "ParseError", message);
    ObjectPtr meta_ = meta(scope, scope.lex);
    ObjectPtr err = clone(getInheritedSlot(scope, meta_, Symbols::get()["ParseError"]));
    err.lock()->put(Symbols::get()["message"], garnish(scope, message));
    err.lock()->put(Symbols::get()["stack"], scope.dyn);
    return ProtoError(err);
}

ProtoError doEtcError(Scope scope, string errorName, string msg) {
    if (!areExceptionsEnabled(scope))
        gracefulTerminate(scope, errorName, msg);
    ObjectPtr meta_ = meta(scope, scope.lex);
    ObjectPtr err = clone(getInheritedSlot(scope, meta_, Symbols::get()[errorName]));
    err.lock()->put(Symbols::get()["stack"], scope.dyn);
    err.lock()->put(Symbols::get()["message"], garnish(scope, msg));
    return ProtoError(err);
}

// TODO Something about the fact that system calls cause Latitude to "double report" stack frames
//  std/core.lat: 180
//  std/core.lat: 180
// Where one of these is technically a system call that isn't in core.lat
ObjectPtr defineMethod(ObjectPtr global, ObjectPtr method, InstrSeq&& code) {
    ObjectPtr obj = clone(method);
    (makeAssemblerLine(Instr::RET)).appendOnto(code);
    obj.lock()->prim(code);
    obj.lock()->put(Symbols::get()["closure"], global);
    return obj;
}

ObjectPtr defineMethodNoRet(ObjectPtr global, ObjectPtr method, InstrSeq&& code) {
    ObjectPtr obj = clone(method);
    obj.lock()->prim(code);
    obj.lock()->put(Symbols::get()["closure"], global);
    return obj;
}

void spawnSystemCallsNew(ObjectPtr global, ObjectPtr method, ObjectPtr sys, IntState& state) {
    static constexpr long
        TERMINATE = 0,
        KERNEL_LOAD = 1,
        STREAM_DIR = 2,
        STREAM_PUT = 3,
        TO_STRING = 4,
        GENSYM = 5,
        INSTANCE_OF = 6,
        STREAM_READ = 7,
        EVAL = 8,
        SYM_NAME = 9,
        SYM_NUM = 10,
        SYM_INTERN = 11,
        SIMPLE_CMP = 12,
        NUM_LEVEL = 13,
        ORIGIN = 14,
        PROCESS_TASK = 15,
        OBJECT_KEYS = 16,
        FILE_OPEN = 17,
        FILE_CLOSE = 18,
        FILE_EOF = 19,
        STRING_LENGTH = 20,
        STRING_SUB = 21,
        STRING_FIND = 22;

    // TERMINATE
    state.cpp[TERMINATE] = [](IntState& state0) {
        // A last-resort termination of a fiber that malfunctioned; this should ONLY
        // be used as a last resort, as it does not correctly unwind the frames
        // before aborting
        state0 = intState();
    };

    // KERNEL_LOAD ($1 = filename, $2 = global)
    // kernelLoad#: filename, global.
    state.cpp[KERNEL_LOAD] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr str = (*dyn.lock())[ Symbols::get()["$1"] ].getPtr();
        ObjectPtr global = (*dyn.lock())[ Symbols::get()["$2"] ].getPtr();
        if ((!str.expired()) && (!global.expired())) {
            auto str0 = boost::get<string>(&str.lock()->prim());
            if (str0)
                readFileNew(*str0, { global, global }, state0);
            else
                throwError(state0, "TypeError", "String expected");
        } else {
            throwError(state0, "SystemArgError", "Wrong number of arguments");
        }
    };
    sys.lock()->put(Symbols::get()["kernelLoad#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::CPP, KERNEL_LOAD))));

    // accessSlot#: obj, sym.
    sys.lock()->put(Symbols::get()["accessSlot#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                                         makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV))));

    // doClone#: obj, sym.
    sys.lock()->put(Symbols::get()["doClone#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::CLONE))));

    // invoke#: obj, mthd.
    sys.lock()->put(Symbols::get()["invoke#"],
                    defineMethodNoRet(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                              makeAssemblerLine(Instr::SYM, "$1"),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                              makeAssemblerLine(Instr::RTRV),
                                                              makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                              makeAssemblerLine(Instr::GETD),
                                                              makeAssemblerLine(Instr::SYM, "$2"),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                              makeAssemblerLine(Instr::RTRV),
                                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                              // We want to forward the parent's arguments
                                                              makeAssemblerLine(Instr::RET),
                                                              makeAssemblerLine(Instr::CALL, 0L))));

    // STREAM_DIR ($1 = argument) (where %num0 specifies the direction; 0 = in, 1 = out)
    // streamIn#: stream.
    // streamOut#: stream.
    state.cpp[STREAM_DIR] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn.lock())[ Symbols::get()["$1"] ].getPtr();
        if (!stream.expired()) {
            auto stream0 = boost::get<StreamPtr>(&stream.lock()->prim());
            if (stream0) {
                switch (state0.num0.asSmallInt()) {
                case 0:
                    garnishNew(state0, (*stream0)->hasIn());
                    break;
                case 1:
                    garnishNew(state0, (*stream0)->hasOut());
                    break;
                }
            } else {
                throwError(state0, "TypeError", "Stream expected");
            }
        } else {
            throwError(state0, "SystemArgError", "Wrong number of arguments");
        }
    };
    sys.lock()->put(Symbols::get()["streamIn#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::INT, 0L),
                                                         makeAssemblerLine(Instr::CPP, STREAM_DIR))));
    sys.lock()->put(Symbols::get()["streamOut#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::INT, 1L),
                                                         makeAssemblerLine(Instr::CPP, STREAM_DIR))));

    // STREAM_PUT ($1 = stream, $2 = string) (where %num0 specifies whether a newline is added; 0 = no, 1 = yes)
    // streamPuts#: stream, str.
    // streamPutln#: stream, str.
    state.cpp[STREAM_PUT] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn.lock())[ Symbols::get()["$1"] ].getPtr();
        ObjectPtr str = (*dyn.lock())[ Symbols::get()["$2"] ].getPtr();
        if ((!stream.expired()) && (!str.expired())) {
            auto stream0 = boost::get<StreamPtr>(&stream.lock()->prim());
            auto str0 = boost::get<string>(&str.lock()->prim());
            if (stream0 && str0) {
                if ((*stream0)->hasOut()) {
                    switch (state0.num0.asSmallInt()) {
                    case 0:
                        (*stream0)->writeText(*str0);
                        break;
                    case 1:
                        (*stream0)->writeLine(*str0);
                        break;
                    }
                    garnishNew(state0, boost::blank());
                } else {
                    throwError(state0, "IOError", "Stream not designated for output");
                }
            }
        } else {
            throwError(state0, "SystemArgError", "Wrong number of arguments");
        }
    };
    sys.lock()->put(Symbols::get()["streamPuts#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::INT, 0L),
                                                         makeAssemblerLine(Instr::CPP, STREAM_PUT))));
    sys.lock()->put(Symbols::get()["streamPutln#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::INT, 1L),
                                                         makeAssemblerLine(Instr::CPP, STREAM_PUT))));

    // TO_STRING (where %num0 specifies which register to use)
    //   0 = %num1
    //   1 = %str0
    //   2 = %sym
    // numToString#: num
    // strToString#: str
    // symToString#: sym
    state.cpp[TO_STRING] = [](IntState& state0) {
        ostringstream oss;
        switch (state0.num0.asSmallInt()) {
        case 0:
            oss << state0.num1.asString();
            break;
        case 1:
            oss << '"';
            for (char ch : state0.str0) {
                if (ch == '"')
                    oss << "\\\"";
                else if (ch == '\\')
                    oss << "\\\\";
                else
                    oss << ch;
            }
            oss << '"';
            break;
        case 2: {
            string str = Symbols::get()[state0.sym];
            if (Symbols::requiresEscape(str)) {
                oss << "'(";
                for (char ch : str) {
                    if (ch == '(')
                        oss << "\\(";
                    else if (ch == ')')
                        oss << "\\)";
                    else if (ch == '\\')
                        oss << "\\\\";
                    else
                        oss << ch;
                }
                oss << ")";
            } else {
                if (!Symbols::isUninterned(str))
                    oss << '\'';
                oss << str;
            }
        }
            break;
        }
        garnishNew(state0, oss.str());
    };
    sys.lock()->put(Symbols::get()["numToString#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::INT, 0L),
                                                         makeAssemblerLine(Instr::CPP, TO_STRING))));
    sys.lock()->put(Symbols::get()["strToString#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                                         makeAssemblerLine(Instr::INT, 1L),
                                                         makeAssemblerLine(Instr::CPP, TO_STRING))));
    sys.lock()->put(Symbols::get()["symToString#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                                         makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                                         makeAssemblerLine(Instr::INT, 2L),
                                                         makeAssemblerLine(Instr::CPP, TO_STRING))));

    // GENSYM (if %num0 is 1 then use %str0 as prefix, else if %num0 is 0 use default prefix; store in %sym)
    // gensym#: self.
    // gensymOf#: self, prefix.
    state.cpp[GENSYM] = [](IntState& state0) {
        switch (state0.num0.asSmallInt()) {
        case 0:
            state0.sym = Symbols::gensym();
            break;
        case 1:
            state0.sym = Symbols::gensym(state0.str0);
            break;
        }
    };
    sys.lock()->put(Symbols::get()["gensym#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::INT, 0L),
                                                         makeAssemblerLine(Instr::CPP, GENSYM),
                                                         makeAssemblerLine(Instr::LOAD, Reg::SYM),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["gensymOf#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                                         makeAssemblerLine(Instr::INT, 1L),
                                                         makeAssemblerLine(Instr::CPP, GENSYM),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::LOAD, Reg::SYM),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // ptrEquals#: obj1, obj2.
    sys.lock()->put(Symbols::get()["ptrEquals#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::TEST),
                                                         makeAssemblerLine(Instr::BOL))));

    // ifThenElse#: trueValue, cond, mthd0, mthd1.
    // _onTrue# and _onFalse# have underscores in front of their names for a reason.
    // DON'T call them directly; they will corrupt your call stack if called from
    // anywhere other than ifThenElse#.
    sys.lock()->put(Symbols::get()["_onTrue#"],
                    defineMethodNoRet(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                              makeAssemblerLine(Instr::SYM, "$3"),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                              makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                                              makeAssemblerLine(Instr::RTRV),
                                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                              makeAssemblerLine(Instr::CALL, 0L))));
    sys.lock()->put(Symbols::get()["_onFalse#"],
                    defineMethodNoRet(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                              makeAssemblerLine(Instr::SYM, "$4"),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                              makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                                              makeAssemblerLine(Instr::RTRV),
                                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                              makeAssemblerLine(Instr::CALL, 0L))));
    sys.lock()->put(Symbols::get()["ifThenElse#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETL),
                                                         makeAssemblerLine(Instr::SYM, "self"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::SYM, "_onTrue#"),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETL),
                                                         makeAssemblerLine(Instr::SYM, "self"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::SYM, "_onFalse#"),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::MTHDZ),
                                                         makeAssemblerLine(Instr::THROA, "Method expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::MTHD),
                                                         makeAssemblerLine(Instr::THROA, "Method expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::TEST),
                                                         makeAssemblerLine(Instr::BRANCH))));

    // putSlot#: obj, sym, val.
    sys.lock()->put(Symbols::get()["putSlot#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$3"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                                         makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::SETF),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // callCC#: newCont, mthd.
    // exitCC#: cont, ret.
    sys.lock()->put(Symbols::get()["callCC#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::CCALL))));
    sys.lock()->put(Symbols::get()["exitCC#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::CRET))));

    // Note that these two "methods" in particular do not have well-defined return values.
    // thunk#: before, after.
    // unthunk#.
    sys.lock()->put(Symbols::get()["thunk#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::WND),
                                                         makeAssemblerLine(Instr::THROA, "Method expected"))));
    sys.lock()->put(Symbols::get()["unthunk#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::UNWND))));

    // INSTANCE_OF (check if %slf is an instance of %ptr, put result in %flag)
    // instanceOf#: obj, anc
    state.cpp[INSTANCE_OF] = [](IntState& state0) {
        auto hier = hierarchy(state0.slf);
        state0.flag = (find_if(hier.begin(), hier.end(),
                               [&state0](auto& o){ return o.lock() == state0.ptr.lock(); }) != hier.end());
    };
    sys.lock()->put(Symbols::get()["instanceOf#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::CPP, INSTANCE_OF),
                                                         makeAssemblerLine(Instr::BOL))));

    // throw#: obj.
    sys.lock()->put(Symbols::get()["throw#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::THROW))));

    // handler#: obj.
    // unhandler#.
    sys.lock()->put(Symbols::get()["handler#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::HAND))));
    sys.lock()->put(Symbols::get()["unhandler#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::POP, Reg::HAND),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // kill#.
    sys.lock()->put(Symbols::get()["kill#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::CPP, 0L))));

    // STREAM_READ ($1 = stream) (constructs and stores the resulting string in %ret, uses %num0 for mode)
    // - 0 - Read a line
    // - 1 - Read a single character
    // streamRead#: stream.
    state.cpp[STREAM_READ] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn.lock())[ Symbols::get()["$1"] ].getPtr();
        if (!stream.expired()) {
            auto stream0 = boost::get<StreamPtr>(&stream.lock()->prim());
            if (stream0) {
                if ((*stream0)->hasIn()) {
                    if (state0.num0.asSmallInt() == 0)
                        garnishNew(state0, (*stream0)->readLine());
                    else
                        garnishNew(state0, (*stream0)->readText(1));
                } else {
                    throwError(state0, "IOError", "Stream not designated for output");
                }
            } else {
                throwError(state0, "TypeError", "Stream expected");
            }
        }
    };
    sys.lock()->put(Symbols::get()["streamRead#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::INT, 0L),
                                                         makeAssemblerLine(Instr::CPP, STREAM_READ))));
    sys.lock()->put(Symbols::get()["streamReadChar#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::INT, 1L),
                                                         makeAssemblerLine(Instr::CPP, STREAM_READ))));

    // EVAL (where %str0 is a string to evaluate; throws if something goes wrong)
    // eval#: lex, dyn, str.
    state.cpp[EVAL] = [](IntState& state0) {
        evalNew(state0, state0.str0);
    };
    sys.lock()->put(Symbols::get()["eval#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$3"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::DYN),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::LEX),
                                                         makeAssemblerLine(Instr::CPP, EVAL),
                                                         makeAssemblerLine(Instr::RET))));

    // stringConcat#: str1, str2.
    sys.lock()->put(Symbols::get()["stringConcat#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETL),
                                                         makeAssemblerLine(Instr::SYM, "meta"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::SYM, "String"),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::CLONE),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR1),
                                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                                         makeAssemblerLine(Instr::ADDS),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::LOAD, Reg::STR0),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // numAdd#: n1, n2.
    // numSub#: n1, n2.
    // numMul#: n1, n2.
    // numDiv#: n1, n2.
    // numMod#: n1, n2.
    // numPow#: n1, n2.
    sys.lock()->put(Symbols::get()["numAdd#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETL),
                                                         makeAssemblerLine(Instr::SYM, "meta"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::SYM, "Number"),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::CLONE),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::ARITH, 1L),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["numSub#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETL),
                                                         makeAssemblerLine(Instr::SYM, "meta"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::SYM, "Number"),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::CLONE),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::ARITH, 2L),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["numMul#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETL),
                                                         makeAssemblerLine(Instr::SYM, "meta"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::SYM, "Number"),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::CLONE),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::ARITH, 3L),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["numDiv#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETL),
                                                         makeAssemblerLine(Instr::SYM, "meta"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::SYM, "Number"),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::CLONE),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::ARITH, 4L),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["numMod#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETL),
                                                         makeAssemblerLine(Instr::SYM, "meta"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::SYM, "Number"),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::CLONE),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::ARITH, 5L),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["numPow#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETL),
                                                         makeAssemblerLine(Instr::SYM, "meta"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::SYM, "Number"),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::CLONE),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::ARITH, 6L),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // SYM_NAME (takes %sym, looks up its name, and outputs a string as %ret)
    // symName#: sym.
    state.cpp[SYM_NAME] = [](IntState& state0) {
        std::string name = Symbols::get()[ state0.sym ];
        garnishNew(state0, name);
    };
    sys.lock()->put(Symbols::get()["symName#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                                         makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                                         makeAssemblerLine(Instr::CPP, SYM_NAME))));

    // SYM_NUM (takes %num0 and outputs an appropriate symbol to %ret)
    // natSym#: num.
    state.cpp[SYM_NUM] = [](IntState& state0) {
        if (state0.num0.asSmallInt() <= 0) {
            throwError(state0, "TypeError", "Cannot produce symbols from non-positive numbers");
        } else {
            Symbolic sym = Symbols::natural((int)state0.num0.asSmallInt());
            garnishNew(state0, sym);
        }
    };
    sys.lock()->put(Symbols::get()["natSym#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::CPP, SYM_NUM))));

    // doWithCallback#: self, mthd, modifier
    // (This one manipulates the call stack a bit, so there is no RET at the end; there's one
    //  in the middle though that has basically the same effect)
    sys.lock()->put(Symbols::get()["doWithCallback#"],
                    defineMethodNoRet(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                              makeAssemblerLine(Instr::SYM, "$3"),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                              makeAssemblerLine(Instr::RTRV),
                                                              makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                              makeAssemblerLine(Instr::GETD),
                                                              makeAssemblerLine(Instr::SYM, "$2"),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                              makeAssemblerLine(Instr::RTRV),
                                                              makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                              makeAssemblerLine(Instr::GETD),
                                                              makeAssemblerLine(Instr::SYM, "$1"),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                              makeAssemblerLine(Instr::RTRV),
                                                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                                              makeAssemblerLine(Instr::RET),
                                                              makeAssemblerLine(Instr::XCALL0, 0L),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET),
                                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                                              makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET),
                                                              makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                                              makeAssemblerLine(Instr::POP, Reg::LEX),
                                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::ARG),
                                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::STO),
                                                              makeAssemblerLine(Instr::POP, Reg::DYN),
                                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::ARG),
                                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::STO),
                                                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                              makeAssemblerLine(Instr::CALL, 2L),
                                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::DYN),
                                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::LEX),
                                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                                              makeAssemblerLine(Instr::XCALL))));

    // SYM_INTERN (takes %str0, looks it up, and puts the result as a symbol in %ret)
    // intern#: str.
    state.cpp[SYM_INTERN] = [](IntState& state0) {
        Symbolic name = Symbols::get()[ state0.str0 ];
        garnishNew(state0, name);
    };
    sys.lock()->put(Symbols::get()["intern#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                                         makeAssemblerLine(Instr::CPP, SYM_INTERN))));

    // SIMPLE_CMP (compares %slf's and %ptr's respective prim fields based on the value of %num0)
    // - 0 - Compare for equality and put the result in %flag
    // - 1 - Compare for LT and put the result in %flag
    // In any case, if either argument lacks a prim or the prim fields have different types, false
    // is returned by default.
    // SIMPLE_CMP will compare strings, numbers, and symbols. Anything else returns false.
    // primEquals#: lhs, rhs.
    // primLT#: lhs, rhs.
    state.cpp[SIMPLE_CMP] = [](IntState& state0) {
        bool doLT = false;
        if (state0.num0.asSmallInt() == 1)
            doLT = true;
        auto magicCmp = [doLT, &state0](auto x, auto y) {
            if (doLT)
                state0.flag = (*x < *y);
            else
                state0.flag = (*x == *y);
        };
        state0.flag = false;
        auto prim0 = state0.slf.lock()->prim();
        auto prim1 = state0.ptr.lock()->prim();
        auto n0 = boost::get<Number>(&prim0);
        auto n1 = boost::get<Number>(&prim1);
        auto st0 = boost::get<string>(&prim0);
        auto st1 = boost::get<string>(&prim1);
        auto sy0 = boost::get<Symbolic>(&prim0);
        auto sy1 = boost::get<Symbolic>(&prim1);
        if (n0 && n1)
            magicCmp(n0, n1);
        else if (st0 && st1)
            magicCmp(st0, st1);
        else if (sy0 && sy1)
            magicCmp(sy0, sy1);
    };
    sys.lock()->put(Symbols::get()["primEquals#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::INT, 0L),
                                                         makeAssemblerLine(Instr::CPP, SIMPLE_CMP),
                                                         makeAssemblerLine(Instr::BOL))));
    sys.lock()->put(Symbols::get()["primLT#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::INT, 1L),
                                                         makeAssemblerLine(Instr::CPP, SIMPLE_CMP),
                                                         makeAssemblerLine(Instr::BOL))));

    // NUM_LEVEL (determine the "level" of %num0 and put the result in %ret)
    // numLevel#: num.
    state.cpp[NUM_LEVEL] = [](IntState& state0) {
        garnishNew(state0, state0.num0.hierarchyLevel());
    };
    sys.lock()->put(Symbols::get()["numLevel#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::CPP, NUM_LEVEL))));

    // stackTrace#.
    // stackTrace# only includes %trace, not %line nor %file. %line and %file are often
    // undesired in this case as they represent only the line and file of the stackTrace#
    // call, which is bogus, as stackTrace# is not defined in a file.
    sys.lock()->put(Symbols::get()["stackTrace#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::LOCRT))));

    // ORIGIN (find the origin of %sym in %slf, store resulting object in %ret, throw SlotError otherwise
    // origin#: self, sym.
    state.cpp[ORIGIN] = [](IntState& state0) {
        list<ObjectSPtr> parents;
        ObjectSPtr curr = state0.slf.lock();
        Symbolic name = state0.sym;
        ObjectPtr value;
        while (find(parents.begin(), parents.end(), curr) == parents.end()) {
            parents.push_back(curr);
            Slot slot = (*curr)[name];
            if (slot.getType() == SlotType::PTR) {
                value = curr;
                break;
            }
            curr = (*curr)[ Symbols::get()["parent"] ].getPtr().lock();
        }
        if (value.expired()) {
            throwError(state0, "SlotError", "Cannot find origin of nonexistent slot");
        } else {
            state0.ret = value;
        }
    };
    sys.lock()->put(Symbols::get()["origin#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                                         makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::CPP, ORIGIN))));

    // PROCESS_TASK - Do something with %slf and possibly other registers, based on %num0
    // - 0 - Create (%slf should be `Process`, %str0 should be the command, and %ret will be a new clone)
    // - 1 - Exec (%prcs should be the process, no return value)
    // - 2 - OutStream (%prcs should be the process, %ptr a stream object)
    // - 3 - InStream (%prcs should be the process, %ptr a stream object)
    // - 4 - ErrStream (%prcs should be the process, %ptr a stream object)
    // - 5 - IsRunning (%prcs should be the process, %ret will be a boolean)
    // - 6 - ExitCode (%prcs should be the process, %ret will be the number)
    // - 7 - IsFinished (%prcs should be the process, %ret will be a boolean)
    // processInStream#: strm, prc.
    // processOutStream#: strm, prc.
    // processErrStream#: strm, prc.
    // processCreate#: self, str.
    // processFinished#: prc.
    // processRunning#: prc.
    // processExitCode#: prc.
    // processExec#: prc.
    state.cpp[PROCESS_TASK] = [](IntState& state0) {
        switch (state0.num0.asSmallInt()) {
        case 0: {
            ProcessPtr proc = makeProcess(state0.str0);
            if (!proc)
                throwError(state0, "NotSupportedError",
                           "Asynchronous processes not supported on this system");
            state0.ret = clone(state0.slf);
            state0.ret.lock()->prim(proc);
            break;
        }
        case 1: {
            bool status = state0.prcs->run();
            if (!status)
                throwError(state0, "IOError",
                           "Could not start process");
            break;
        }
        case 2: {
            state0.ptr.lock()->prim(state0.prcs->stdOut());
            break;
        }
        case 3: {
            state0.ptr.lock()->prim(state0.prcs->stdIn());
            break;
        }
        case 4: {
            state0.ptr.lock()->prim(state0.prcs->stdErr());
            break;
        }
        case 5: {
            garnishNew(state0, state0.prcs->isRunning());
            break;
        }
        case 6: {
            garnishNew(state0, state0.prcs->getExitCode());
            break;
        }
        case 7: {
            garnishNew(state0, state0.prcs->isDone());
            break;
        }
        }
    };
    sys.lock()->put(Symbols::get()["processInStream#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::INT, 3L),
                                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["processOutStream#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::INT, 2L),
                                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["processErrStream#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::INT, 4L),
                                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["processCreate#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::INT, 0L),
                                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
    sys.lock()->put(Symbols::get()["processFinished#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                                         makeAssemblerLine(Instr::INT, 7L),
                                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
    sys.lock()->put(Symbols::get()["processRunning#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                                         makeAssemblerLine(Instr::INT, 5L),
                                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
    sys.lock()->put(Symbols::get()["processExitCode#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                                         makeAssemblerLine(Instr::INT, 6L),
                                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
    sys.lock()->put(Symbols::get()["processExec#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET),
                                                         makeAssemblerLine(Instr::INT, 1L),
                                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK))));

    // OBJECT_KEYS (takes an object in %slf and outputs an array-like construct using `meta brackets`
    //              which contains all of the slot names of %slf)
    // objectKeys#: obj.
    state.cpp[OBJECT_KEYS] = [](IntState& state0) {
        set<Symbolic> allKeys = keys(state0.slf);
        state0.stack.push(state0.cont);
        state0.cont = asmCode(makeAssemblerLine(Instr::GETL),
                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                              makeAssemblerLine(Instr::SYM, "meta"),
                              makeAssemblerLine(Instr::RTRV),
                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                              makeAssemblerLine(Instr::SYM, "brackets"),
                              makeAssemblerLine(Instr::RTRV),
                              makeAssemblerLine(Instr::GETL),
                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                              makeAssemblerLine(Instr::CALL, 0L),
                              makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO));
        for (Symbolic sym : allKeys) {
            InstrSeq seq = garnishSeq(sym);
            (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);
            (makeAssemblerLine(Instr::PEEK, Reg::STO)).appendOnto(seq);
            (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(seq);
            (makeAssemblerLine(Instr::SYM, "next")).appendOnto(seq);
            (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
            (makeAssemblerLine(Instr::PEEK, Reg::STO)).appendOnto(seq);
            (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(seq);
            (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
            (makeAssemblerLine(Instr::CALL, 1L)).appendOnto(seq);
            state0.cont.insert(state0.cont.end(), seq.begin(), seq.end());
        }
        (makeAssemblerLine(Instr::PEEK, Reg::STO)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::SYM, "finish")).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::RTRV)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::PEEK, Reg::STO)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::CALL, 0L)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::POP, Reg::STO)).appendOnto(state0.cont);
    };
    sys.lock()->put(Symbols::get()["objectKeys#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                                         makeAssemblerLine(Instr::CPP, OBJECT_KEYS))));

    // FILE_OPEN (%str0 is the filename, %ptr is a stream object to be filled, $3 and $4 are access and mode)
    // streamFileOpen#: strm, fname, access, mode.
    state.cpp[FILE_OPEN] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr access = (*dyn.lock())[ Symbols::get()["$3"] ].getPtr();
        ObjectPtr mode = (*dyn.lock())[ Symbols::get()["$4"] ].getPtr();
        auto access0 = boost::get<Symbolic>(&access.lock()->prim());
        auto mode0 = boost::get<Symbolic>(&mode.lock()->prim());
        if (access0 && mode0) {
            auto access1 = Symbols::get()[*access0];
            auto mode1 = Symbols::get()[*mode0];
            FileAccess access2;
            FileMode mode2;
            if (access1 == "read")
                access2 = FileAccess::READ;
            else if (access1 == "write")
                access2 = FileAccess::WRITE;
            else
                throwError(state0, "SystemArgError",
                           "Invalid access specifier when opening file");
            if (mode1 == "text")
                mode2 = FileMode::TEXT;
            else if (mode1 == "binary")
                mode2 = FileMode::BINARY;
            else
                throwError(state0, "SystemArgError",
                           "Invalid mode specifier when opening file");
            state0.ptr.lock()->prim( StreamPtr(new FileStream(state0.str0, access2, mode2)) );
        } else {
            throwError(state0, "TypeError",
                       "Symbol expected");
        }
    };
    sys.lock()->put(Symbols::get()["streamFileOpen#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::CPP, FILE_OPEN),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // FILE_CLOSE (takes %strm and closes it)
    // streamClose#: strm.
    state.cpp[FILE_CLOSE] = [](IntState& state0) {
        state0.strm->close();
    };
    sys.lock()->put(Symbols::get()["streamClose#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STRM),
                                                         makeAssemblerLine(Instr::THROA, "Stream expected"),
                                                         makeAssemblerLine(Instr::CPP, FILE_CLOSE),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // FILE_EOF (takes %strm and outputs whether it's at eof into %flag)
    // streamEof#: strm.
    state.cpp[FILE_EOF] = [](IntState& state0) {
        state0.flag = state0.strm->isEof();
    };
    sys.lock()->put(Symbols::get()["streamEof#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STRM),
                                                         makeAssemblerLine(Instr::THROA, "Stream expected"),
                                                         makeAssemblerLine(Instr::CPP, FILE_EOF),
                                                         makeAssemblerLine(Instr::BOL))));

    // STRING_LENGTH (outputs length of %str0 into %ret)
    // stringLength#: str.
    state.cpp[STRING_LENGTH] = [](IntState& state0) {
        // TODO Possible loss of precision from size_t to signed long?
        garnishNew(state0, (long)state0.str0.length());
    };
    sys.lock()->put(Symbols::get()["stringLength#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                                         makeAssemblerLine(Instr::CPP, STRING_LENGTH))));

    // STRING_SUB (outputs substring of %str0 from %num0 to %num1 into %ret)
    // stringSubstring#: str, beg, end.
    state.cpp[STRING_SUB] = [](IntState& state0) {
        long start1 = state0.num0.asSmallInt();
        long end1 = state0.num1.asSmallInt();
        long size = state0.str0.length();
        if (start1 < 0)
            start1 += size;
        if (end1 < 0)
            end1 += size;
        if (start1 >= size)
            start1 = size;
        if (end1 >= size)
            end1 = size;
        if (start1 < 0)
            start1 = 0;
        if (end1 < 0)
            end1 = 0;
        long len = end1 - start1;
        if (len < 0)
            len = 0;
        garnishNew(state0, state0.str0.substr(start1, len));
    };
    sys.lock()->put(Symbols::get()["stringSubstring#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$3"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                                         makeAssemblerLine(Instr::CPP, STRING_SUB))));

    // STRING_FIND (find first occurence of %str1 in %str0 starting at %num0 index, storing
    //              new index or Nil in %ret)
    // stringFindFirst#: str, substr, pos.
    state.cpp[STRING_FIND] = [](IntState& state0) {
        auto pos = state0.str0.find(state0.str1, state0.num0.asSmallInt());
        if (pos == string::npos)
            garnishNew(state0, boost::blank());
        else
            garnishNew(state0, (long)pos);
    };
    sys.lock()->put(Symbols::get()["stringFindFirst#"],
                    defineMethod(global, method, asmCode(makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$1"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$2"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                                         makeAssemblerLine(Instr::GETD),
                                                         makeAssemblerLine(Instr::SYM, "$3"),
                                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                                         makeAssemblerLine(Instr::RTRV),
                                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::ECLR),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR1),
                                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                                         makeAssemblerLine(Instr::CPP, STRING_FIND))));

}

ObjectPtr spawnObjectsNew(IntState& state) {

    ObjectPtr object(GC::get().allocate());
    ObjectPtr meta(clone(object));
    ObjectPtr global(clone(object));

    ObjectPtr proc(clone(object));
    ObjectPtr method(clone(proc));
    ObjectPtr systemCall(clone(method));
    ObjectPtr number(clone(object));
    ObjectPtr string(clone(object));
    ObjectPtr symbol(clone(object));

    ObjectPtr cont(clone(proc));
    ObjectPtr contValidator(clone(object));

    ObjectPtr exception(clone(object));
    ObjectPtr systemError(clone(exception));

    ObjectPtr process(clone(object));
    ObjectPtr stream(clone(object));
    ObjectPtr stdout_(clone(stream));
    ObjectPtr stdin_(clone(stream));
    ObjectPtr stderr_(clone(stream));

    ObjectPtr array_(clone(object));

    ObjectPtr sys(clone(object));
    ObjectPtr sigil(clone(object));
    ObjectPtr stackFrame(clone(object));
    ObjectPtr kernel(clone(object));

    ObjectPtr nil(clone(object));
    ObjectPtr boolean(clone(object));
    ObjectPtr true_(clone(boolean));
    ObjectPtr false_(clone(boolean));

    // Meta calls for basic types
    meta.lock()->put(Symbols::get()["Object"], object);
    meta.lock()->put(Symbols::get()["Proc"], proc);
    meta.lock()->put(Symbols::get()["Method"], method);
    meta.lock()->put(Symbols::get()["Number"], number);
    meta.lock()->put(Symbols::get()["String"], string);
    meta.lock()->put(Symbols::get()["Symbol"], symbol);
    meta.lock()->put(Symbols::get()["Stream"], stream);
    meta.lock()->put(Symbols::get()["Process"], process);
    meta.lock()->put(Symbols::get()["SystemCall"], systemCall);
    meta.lock()->put(Symbols::get()["True"], true_);
    meta.lock()->put(Symbols::get()["False"], false_);
    meta.lock()->put(Symbols::get()["Nil"], nil);
    meta.lock()->put(Symbols::get()["Boolean"], boolean);
    meta.lock()->put(Symbols::get()["Cont"], cont);
    meta.lock()->put(Symbols::get()["ContValidator"], contValidator);
    meta.lock()->put(Symbols::get()["sys"], sys);
    meta.lock()->put(Symbols::get()["Exception"], exception);
    meta.lock()->put(Symbols::get()["SystemError"], systemError);
    meta.lock()->put(Symbols::get()["Array"], array_);
    meta.lock()->put(Symbols::get()["Kernel"], kernel);

    // Object is its own parent
    object.lock()->put(Symbols::get()["parent"], object);

    // Meta linkage
    meta.lock()->put(Symbols::get()["meta"], meta);
    object.lock()->put(Symbols::get()["meta"], meta);
    meta.lock()->put(Symbols::get()["sys"], sys);
    meta.lock()->put(Symbols::get()["sigil"], sigil);
    meta.lock()->put(Symbols::get()["StackFrame"], stackFrame);

    // This ensures that problems in the core.lat file (before exceptions are well-defined)
    // do not cause the entire program to crash
    meta.lock()->put(Symbols::get()["exceptions?"], false_);

    // Global variables not accessible in meta
    global.lock()->put(Symbols::get()["stdin"], stdin_);
    global.lock()->put(Symbols::get()["stderr"], stderr_);
    global.lock()->put(Symbols::get()["stdout"], stdout_);
    global.lock()->put(Symbols::get()["global"], global);

    state.lex.push(global);
    state.dyn.push(global);

    // Method and system call properties
    spawnSystemCallsNew(global, method, sys, state);

    // Prim Fields
    method.lock()->prim(asmCode(makeAssemblerLine(Instr::RET)));
    number.lock()->prim(0.0);
    string.lock()->prim("");
    symbol.lock()->prim(Symbols::get()[""]);
    contValidator.lock()->prim(weak_ptr<SignalValidator>());
    cont.lock()->prim(StatePtr());
    stdout_.lock()->prim(outStream());
    stdin_.lock()->prim(inStream());
    stderr_.lock()->prim(errStream());

    // Location and line number (necessary for error printing; nice to have anyway)
    meta.lock()->put(Symbols::get()["lineStorage"], garnish({global, global}, Symbols::gensym("STORE")));
    meta.lock()->put(Symbols::get()["fileStorage"], garnish({global, global}, Symbols::gensym("STORE")));

    // The core libraries
    readFileNew("std/latitude.lat", { global, global }, state);

    return global;
}

void throwError(IntState& state, std::string name, std::string msg) {
    // TODO Make the throw / throq / throa instructions terminate if exceptions are disabled
    state.stack.push(state.cont);
    state.cont = asmCode(makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                         makeAssemblerLine(Instr::GETL),
                         makeAssemblerLine(Instr::SYM, "meta"),
                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                         makeAssemblerLine(Instr::RTRV),
                         makeAssemblerLine(Instr::SYM, name),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::RTRV),
                         makeAssemblerLine(Instr::POP, Reg::STO),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::CLONE),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::SYM, "message"),
                         makeAssemblerLine(Instr::SETF),
                         makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                         makeAssemblerLine(Instr::LOCRT),
                         makeAssemblerLine(Instr::POP, Reg::STO),
                         makeAssemblerLine(Instr::SYM, "stack"),
                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                         makeAssemblerLine(Instr::SETF),
                         makeAssemblerLine(Instr::THROW));
    state.stack.push(state.cont);
    state.cont.clear();
    garnishNew(state, msg);
}

void throwError(IntState& state, std::string name) {
    // TODO Make the throw / throq / throa instructions terminate if exceptions are disabled
    state.stack.push(state.cont);
    state.cont = asmCode(makeAssemblerLine(Instr::GETL),
                         makeAssemblerLine(Instr::SYM, "meta"),
                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                         makeAssemblerLine(Instr::RTRV),
                         makeAssemblerLine(Instr::SYM, name),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::RTRV),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::CLONE),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::GETD),
                         makeAssemblerLine(Instr::SYM, "stack"),
                         makeAssemblerLine(Instr::SETF),
                         makeAssemblerLine(Instr::THROW));
}
