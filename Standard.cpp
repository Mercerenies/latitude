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

// TODO More objects should have toString so they don't all default to showing "Object"

// TODO Make primitive objects like String and Number clone properly (prim() fields don't clone)

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
                    process0.lock()->prim(makeProcess(*string0));
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
                    throw doEtcError(scope, "ProcessError",
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
                    throw doEtcError(scope, "ProcessError",
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
                    throw doEtcError(scope, "ProcessError",
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
                    throw doEtcError(scope, "ProcessError",
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
                    throw doEtcError(scope, "ProcessError",
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
                    throw doEtcError(scope, "ProcessError",
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
                    throw doEtcError(scope, "ProcessError",
                                     "Object is not a process");
                }
            } else {
                throw doSystemArgError(scope, "processExitCode#", 1, lst.size());
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

