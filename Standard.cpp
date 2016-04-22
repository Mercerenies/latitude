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

void spawnSystemCalls(ObjectPtr& global, ObjectPtr& systemCall, ObjectPtr& sys);
void spawnExceptions(ObjectPtr& global, ObjectPtr& error, ObjectPtr& meta);

ProtoError doSystemArgError(ObjectPtr global, string name, int expected, int got);
ProtoError doSlotError(ObjectPtr global, ObjectPtr problem, Symbolic slotName);
ProtoError doParseError(ObjectPtr global);
ProtoError doParseError(ObjectPtr global, string message);
ProtoError doEtcError(ObjectPtr global, string errorName, string msg);

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

    ObjectPtr stream(clone(object));
    ObjectPtr stdout_(clone(stream));
    ObjectPtr stdin_(clone(stream));
    ObjectPtr stderr_(clone(stream));

    ObjectPtr array_(clone(object));

    ObjectPtr sys(clone(object));
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

    // Global variables not accessible in meta
    global.lock()->put(Symbols::get()["stdin"], stdin_);
    global.lock()->put(Symbols::get()["stderr"], stderr_);
    global.lock()->put(Symbols::get()["stdout"], stdout_);
    global.lock()->put(Symbols::get()["global"], global);

    // Method and system call properties / exceptions
    spawnSystemCalls(global, systemCall, sys);
    spawnExceptions(global, systemError, meta);

    // Prim Fields
    method.lock()->prim(Method());
    number.lock()->prim(0.0);
    string.lock()->prim("");
    symbol.lock()->prim(Symbols::get()["0"]); // TODO Better default?
    contValidator.lock()->prim(weak_ptr<SignalValidator>());
    stdout_.lock()->prim(outStream());
    stdin_.lock()->prim(inStream());
    stderr_.lock()->prim(errStream());

    // The core libraries
    ifstream file("std/latitude.lat");
    BOOST_SCOPE_EXIT(&file) {
        file.close();
    } BOOST_SCOPE_EXIT_END;
    eval(file, "std/latitude.lat", global, global, global, global);

    return global;
}

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
    ObjectPtr callTripleEquals(clone(systemCall));
    ObjectPtr callPrimEquals(clone(systemCall));
    ObjectPtr callStringConcat(clone(systemCall));
    ObjectPtr callStreamRead(clone(systemCall));
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

    systemCall.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            return eval("meta Nil.", global, global);
        });
    callStreamIn.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr stream;
            if (bindArguments(lst, stream)) {
                auto prim = stream->prim();
                if (auto call = boost::get<StreamPtr>(&prim))
                    return garnish(global, (*call)->hasIn());
                else
                    return eval("meta False.", global, global);
            } else {
                throw doSystemArgError(global, "streamIn#", 1, lst.size());
            }
        });
    callStreamOut.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr stream;
            if (bindArguments(lst, stream)) {
                auto prim = stream->prim();
                if (auto call = boost::get<StreamPtr>(&prim))
                    return garnish(global, (*call)->hasOut());
                else
                    return eval("meta False.", global, global);
            } else {
                throw doSystemArgError(global, "streamOut#", 1, lst.size());
            }
        });
    callStreamPuts.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr stream, obj;
            if (bindArguments(lst, stream, obj)) {
                auto stream0 = stream->prim();
                auto obj0 = obj->prim();
                if (auto stream1 = boost::get<StreamPtr>(&stream0)) {
                    if (auto obj1 = boost::get<string>(&obj0)) {
                        if (!(*stream1)->hasOut())
                            throw doEtcError(global, "StreamError",
                                             "Stream not designated for output");
                        (*stream1)->writeText(*obj1);
                        return eval("meta Nil.", global, global);
                    } else {
                        throw doEtcError(global, "StreamError",
                                         "Object to print is not a string");
                    }
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(global, "streamPuts#", 2, lst.size());
            }
        });
    callStreamPutln.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr stream, obj;
            if (bindArguments(lst, stream, obj)) {
                auto stream0 = stream->prim();
                auto obj0 = obj->prim();
                if (auto stream1 = boost::get<StreamPtr>(&stream0)) {
                    if (auto obj1 = boost::get<string>(&obj0)) {
                        if (!(*stream1)->hasOut())
                            throw doEtcError(global, "StreamError",
                                             "Stream not designated for output");
                        (*stream1)->writeLine(*obj1);
                        return eval("meta Nil.", global, global);
                    } else {
                        throw doEtcError(global, "StreamError",
                                         "Object to print is not a string");
                    }
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(global, "streamPutln#", 2, lst.size());
            }
        });
    callPrimToString.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                string result = primToString(obj);
                return garnish(global, result);
            } else {
                throw doSystemArgError(global, "primToString#", 1, lst.size());
            }
        });
    callIfStatement.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr cond, tr, fl;
            if (bindArguments(lst, cond, tr, fl)) {
                // These are unused except to verify that the prim() is correct
                auto tr_ = boost::get<Method>(&tr->prim());
                auto fl_ = boost::get<Method>(&fl->prim());
                if ((tr_ == NULL) || (fl_ == NULL))
                    throw doEtcError(global, "TypeError",
                                     "If statement body is not a method");
                //
                auto definitelyTrue = eval("meta True.", global, global).lock();
                if (cond == definitelyTrue)
                    return doCallWithArgs(lex, dyn, lex, tr);
                else
                    return doCallWithArgs(lex, dyn, lex, fl);
            } else {
                throw doSystemArgError(global, "ifThenElse#", 3, lst.size());
            }
        });
    callStreamDump.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr stream, obj;
            if (bindArguments(lst, stream, obj)) {
                if (auto stream0 = boost::get<StreamPtr>(&stream->prim())) {
                    dumpObject(lex, dyn, **stream0, obj);
                    return eval("meta Nil.", global, global);
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(global, "streamDump#", 2, lst.size());
            }
        });
    callClone.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                return clone(obj);
            } else {
                throw doSystemArgError(global, "doClone#", 1, lst.size());
            }
        });
    callGetSlot.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj, slot;
            if (bindArguments(lst, obj, slot)) {
                if (auto sym = boost::get<Symbolic>(&slot->prim())) {
                    return getInheritedSlot(dyn, obj, *sym);
                } else {
                    throw doEtcError(global, "TypeError", "Slot name must be a symbol");
                }
            } else {
                throw doSystemArgError(global, "accessSlot#", 2, lst.size());
            }
        });
    callHasSlot.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj, slot;
            if (bindArguments(lst, obj, slot)) {
                if (auto sym = boost::get<Symbolic>(&slot->prim())) {
                    return garnish(global,
                                   hasInheritedSlot(dyn, obj, *sym));
                } else {
                    return eval("meta False.", global, global);
                }
            } else {
                throw doSystemArgError(global, "checkSlot#", 2, lst.size());
            }
        });
    callPutSlot.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj, slot, value;
            if (bindArguments(lst, obj, slot, value)) {
                if (auto sym = boost::get<Symbolic>(&slot->prim())) {
                    obj->put(*sym, value);
                    return ObjectPtr(value);
                } else {
                    throw doEtcError(global, "TypeError", "Slot name must be a symbol");
                }
            } else {
                throw doSystemArgError(global, "putSlot#", 3, lst.size());
            }
        });
    callGensym.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr symbol;
            if (bindArguments(lst, symbol)) {
                ObjectPtr gen = clone(symbol);
                gen.lock()->prim( Symbols::gensym() );
                return gen;
            } else {
                throw doSystemArgError(global, "gensym#", 1, lst.size());
            }
        });
    callGensymOf.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr symbol, name;
            if (bindArguments(lst, symbol, name)) {
                if (auto str = boost::get<std::string>(&name->prim())) {
                    ObjectPtr gen = clone(symbol);
                    gen.lock()->prim( Symbols::gensym(*str) );
                    return gen;
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Expected string gensym prefix");
                }
            } else {
                throw doSystemArgError(global, "gensymOf#", 2, lst.size());
            }
        });
    callCallCC.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr mthd;
            if (bindArguments(lst, mthd)) {
                // Dies when it goes out of scope
                shared_ptr<SignalValidator> livingTag(new SignalValidator());
                //
                Symbolic sym = Symbols::gensym("CONT");
                ObjectPtr symObj = getInheritedSlot(dyn,
                                                    meta(dyn, lex),
                                                    Symbols::get()["Symbol"]);
                ObjectPtr symObj1 = clone(symObj);
                symObj1.lock()->prim(sym);
                ObjectPtr validator = getInheritedSlot(dyn,
                                                       meta(dyn, lex),
                                                       Symbols::get()["ContValidator"]);
                ObjectPtr validator1 = clone(validator);
                validator1.lock()->prim(weak_ptr<SignalValidator>(livingTag));
                // Define a callback to store the tagged validator in the dynamic scope of the method
                auto callback = [sym, validator1](ObjectPtr& dyn1) { dyn1.lock()->put(sym, validator1); };
                ObjectPtr cont = getInheritedSlot(dyn,
                                                  meta(dyn, lex),
                                                  Symbols::get()["Cont"]);
                ObjectPtr cont1 = clone(cont);
                cont1.lock()->put(Symbols::get()["tag"], symObj1);
                try {
                    return doCall(lex, dyn, lex, mthd, { cont1 }, callback);
                } catch (Signal& signal) {
                    if (signal.match(sym)) {
                        return signal.getObject();
                    } else {
                        throw;
                    }
                }
            } else {
                throw doSystemArgError(global, "callCC#", 1, lst.size());
            }
        });
    callExitCC.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr tag, arg;
            if (bindArguments(lst, tag, arg)) {
                if (auto sym = boost::get<Symbolic>(&tag->prim())) {
                    if (hasInheritedSlot(dyn, dyn, *sym)) {
                        auto validator = getInheritedSlot(dyn, dyn, *sym);
                        if (auto val =
                            boost::get< weak_ptr<SignalValidator> >(&validator.lock()
                                                                    ->prim())) {
                            if (val->expired())
                                throw doEtcError(global, "ContError",
                                                 "Out of continuation bounds");
                            throw Signal(*sym, arg);
                        } else {
                            throw doEtcError(global, "TypeError",
                                             "Invalid continuation in exitCC#");
                        }
                    } else {
                        throw doEtcError(global, "ContError",
                                         "Out of continuation bounds");
                    }
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Invalid continuation in exitCC#");
                }
                return eval("meta Nil.", global, global); // Should never happen
            } else {
                throw doSystemArgError(global, "exitCC#", 2, lst.size());
            }
        });
    callInstanceof.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj0, obj1;
            if (bindArguments(lst, obj0, obj1)) {
                auto hier = hierarchy(obj0);
                bool result = (find_if(hier.begin(), hier.end(),
                                       [&obj1](auto& o){ return o.lock() == obj1; })
                               != hier.end());
                return garnish(global, result);
            } else {
                throw doSystemArgError(global, "instanceOf#", 2, lst.size());
            }
        });
    callTryStmt.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr lhs, cond, rhs;
            if (bindArguments(lst, lhs, cond, rhs)) {
                try {
                    return doCallWithArgs(lex, dyn, lex, lhs);
                } catch (ProtoError& err) {
                    ObjectPtr result1 = doCallWithArgs(lex, dyn, lex, cond, err.getObject());
                    auto definitelyTrue = eval("meta True.", global, global).lock();
                    if (result1.lock() == definitelyTrue) {
                        return doCallWithArgs(lex, dyn, lex, rhs, err.getObject());
                    } else {
                        throw;
                    }
                }
            } else {
                throw doSystemArgError(global, "try#", 3, lst.size());
            }
        });
    callThrowStmt.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                throwProtoError(obj);
                return eval("meta Nil.", global, global); // Should never happen
            } else {
                throw doSystemArgError(global, "throw#", 1, lst.size());
            }
        });
    callNumAdd.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto n0 = boost::get<Number>(&obj1->prim());
                auto n1 = boost::get<Number>(&obj2->prim());
                if (n0 && n1)
                    return garnish(global, *n0 + *n1);
                else
                    throw doEtcError(global, "TypeError",
                                     "Got non-numeral in arithmetic operation");
            } else {
                throw doSystemArgError(global, "numAdd#", 2, lst.size());
            }
        });
    callNumSub.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto n0 = boost::get<Number>(&obj1->prim());
                auto n1 = boost::get<Number>(&obj2->prim());
                if (n0 && n1)
                    return garnish(global, *n0 - *n1);
                else
                    throw doEtcError(global, "TypeError",
                                     "Got non-numeral in arithmetic operation");
            } else {
                throw doSystemArgError(global, "numSub#", 2, lst.size());
            }
        });
    callNumMul.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto n0 = boost::get<Number>(&obj1->prim());
                auto n1 = boost::get<Number>(&obj2->prim());
                if (n0 && n1)
                    return garnish(global, *n0 * *n1);
                else
                    throw doEtcError(global, "TypeError",
                                     "Got non-numeral in arithmetic operation");
            } else {
                throw doSystemArgError(global, "numMul#", 2, lst.size());
            }
        });
    callNumDiv.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto n0 = boost::get<Number>(&obj1->prim());
                auto n1 = boost::get<Number>(&obj2->prim());
                if (n0 && n1)
                    return garnish(global, *n0 / *n1);
                else
                    throw doEtcError(global, "TypeError",
                                     "Got non-numeral in arithmetic operation");
            } else {
                throw doSystemArgError(global, "numDiv#", 2, lst.size());
            }
        });
    callNumMod.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto n0 = boost::get<Number>(&obj1->prim());
                auto n1 = boost::get<Number>(&obj2->prim());
                if (n0 && n1)
                    return garnish(global, *n0 % *n1);
                else
                    throw doEtcError(global, "TypeError",
                                     "Got non-numeral in arithmetic operation");
            } else {
                throw doSystemArgError(global, "numMod#", 2, lst.size());
            }
        });
    callTripleEquals.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                return garnish(global, obj1 == obj2);
            } else {
                throw doSystemArgError(global, "ptrEquals#", 2, lst.size());
            }
        });
    callPrimEquals.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                return garnish(global, primEquals(obj1, obj2));
            } else {
                throw doSystemArgError(global, "primEquals#", 2, lst.size());
            }
        });
    callStringConcat.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                auto str1 = boost::get<std::string>(&obj1->prim());
                auto str2 = boost::get<std::string>(&obj2->prim());
                if (str1 && str2)
                    return garnish(global, *str1 + *str2);
                else
                    throw doEtcError(global, "TypeError",
                                     "Got non-string in stringConcat#");
            } else {
                throw doSystemArgError(global, "stringConcat#", 2, lst.size());
            }
        });
    callStreamRead.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr stream;
            if (bindArguments(lst, stream)) {
                auto stream0 = stream->prim();
                if (auto stream1 = boost::get<StreamPtr>(&stream0)) {
                    if (!(*stream1)->hasIn())
                        throw doEtcError(global, "StreamError",
                                         "Stream not designated for input");
                    return garnish(global, (*stream1)->readLine());
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(global, "streamRead#", 1, lst.size());
            }
        });
    callScopeProtect.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
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
                BOOST_SCOPE_EXIT(&abnormal, global, block2, lex, dyn) {
                    try {
                        ObjectPtr status = garnish(global, abnormal);
                        doCallWithArgs(lex, dyn, lex, block2, status);
                    } catch (...) {
                        cerr << "Attempted abnormal exit from protected block!" << endl;
                        terminate();
                    }
                } BOOST_SCOPE_EXIT_END;
                ObjectPtr result = doCallWithArgs(lex, dyn, lex, block1);
                abnormal = false;
                return result;
            } else {
                throw doSystemArgError(global, "scopeProtect#", 2, lst.size());
            }
        });
    callInvoke.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr self, mthd;
            if (bindArguments(lst, self, mthd)) {
                // Calls the method with "self" bound to the given object,
                // bypassing the normal self. All arguments ($n) remain the
                // same.
                return doCallWithArgs(lex, dyn, self, mthd);
            } else {
                throw doSystemArgError(global, "invoke#", 2, lst.size());
            }
        });
    callOrigin.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr self, ref;
            if (bindArguments(lst, self, ref)) {
                auto sym = boost::get<Symbolic>(&ref->prim());
                if (!sym)
                    throw doEtcError(global, "TypeError",
                                     "Slot name must be a symbol");
                return getInheritedOrigin(dyn, self, *sym);
            } else {
                throw doSystemArgError(global, "origin#", 2, lst.size());
            }
        });
    callIntern.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr self;
            if (bindArguments(lst, self)) {
                if (auto str = boost::get<string>(&self->prim())) {
                    ObjectPtr sym = getInheritedSlot(dyn,
                                                     meta(dyn, lex),
                                                     Symbols::get()["Symbol"]);
                    ObjectPtr sym1 = clone(sym);
                    sym1.lock()->prim(Symbols::get()[*str]);
                    return sym1;
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Object to intern is not a string");
                }
            } else {
                throw doSystemArgError(global, "intern#", 1, lst.size());
            }
        });
    callSymbolic.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr self;
            if (bindArguments(lst, self)) {
                if (auto sym = boost::get<Symbolic>(&self->prim())) {
                    return garnish(global, Symbols::get()[*sym]);
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Object is not symbolic");
                }
            } else {
                throw doSystemArgError(global, "symbolic#", 1, lst.size());
            }
        });
    callNumLevel.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr self;
            if (bindArguments(lst, self)) {
                if (auto num = boost::get<Number>(&self->prim())) {
                    return garnish(global, num->hierarchyLevel());
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Object is not numerical");
                }
            } else {
                throw doSystemArgError(global, "numLevel#", 1, lst.size());
            }
        });
    callPrimLT.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                return garnish(global, primLT(obj1, obj2));
            } else {
                throw doSystemArgError(global, "primLT#", 2, lst.size());
            }
        });
    callNatSym.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr nat;
            if (bindArguments(lst, nat)) {
                if (auto num = boost::get<Number>(&nat->prim())) {
                    long value = num->asSmallInt();
                    if (value > 0) {
                        Symbolic sym = Symbols::get().natural(value);
                        return garnish(global, sym);
                    } else {
                        throw doEtcError(global, "TypeError",
                                         "Cannot produce natural symbol from numbers <= 0");
                    }
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Object is not numerical");
                }
            } else {
                throw doSystemArgError(global, "natSym#", 1, lst.size());
            }
        });
    callLoop.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr mthd;
            if (bindArguments(lst, lex, dyn, mthd)) {
                // These are unused except to verify that the prim() is correct
                auto mthd_ = boost::get<Method>(&mthd->prim());
                if (mthd_ == NULL)
                    throw doEtcError(global, "TypeError",
                                     "Loop body is not a method");
                //
                while (true)
                    doCallWithArgs(lex, dyn, lex, mthd);
                return eval("meta Nil.", global, global); // Should never happen
            } else {
                throw doSystemArgError(global, "loop#", 1, lst.size());
            }
        });
    callLoadFile.lock()->prim([global](ObjectPtr lex, ObjectPtr dyn, list<ObjectPtr> lst) {
            ObjectSPtr global, filename;
            if (bindArguments(lst, global, filename)) {
                if (auto fname = boost::get<string>(&filename->prim())) {
                    ifstream file(*fname);
                    BOOST_SCOPE_EXIT(&file) {
                        file.close();
                    } BOOST_SCOPE_EXIT_END;
                    ObjectPtr result = eval(file, *fname, global, global, lex, dyn);
                    return result;
                } else {
                    throw doEtcError(global, "TypeError",
                                     "String filename expected");
                }
            } else {
                throw doSystemArgError(global, "loadFile#", 2, lst.size());
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
    sys.lock()->put(Symbols::get()["ptrEquals#"], callTripleEquals);
    sys.lock()->put(Symbols::get()["primEquals#"], callPrimEquals);
    sys.lock()->put(Symbols::get()["stringConcat#"], callStringConcat);
    sys.lock()->put(Symbols::get()["streamRead#"], callStreamRead);
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

}

void spawnExceptions(ObjectPtr& global, ObjectPtr& error, ObjectPtr& meta) {
    // TODO Better messages for these exceptions once we have some kind of
    //      string formatting capabilities.

    ObjectPtr systemCallError(clone(error));
    systemCallError.lock()->put(Symbols::get()["message"],
                                eval("\"Error in system call\".", global, global));

    ObjectPtr systemArgError(clone(error));
    systemArgError.lock()->put(Symbols::get()["message"],
                               eval("\"Wrong number of arguments to system call\".",
                                    global, global));
    systemArgError.lock()->put(Symbols::get()["gotArguments"],
                               eval("0.", global, global));
    systemArgError.lock()->put(Symbols::get()["expectedArguments"],
                               eval("0.", global, global));
    systemArgError.lock()->put(Symbols::get()["functionName"],
                               eval("\"\".", global, global));

    ObjectPtr streamError(clone(error));
    streamError.lock()->put(Symbols::get()["message"],
                            eval("\"Stream error\".", global, global));

    ObjectPtr typeError(clone(error));
    typeError.lock()->put(Symbols::get()["message"],
                          eval("\"Type error\".", global, global));

    ObjectPtr slotError(clone(error));
    slotError.lock()->put(Symbols::get()["message"],
                          eval("\"Could not find slot\".", global, global));
    slotError.lock()->put(Symbols::get()["slotName"],
                          eval("meta Nil.", global, global));
    slotError.lock()->put(Symbols::get()["objectInstance"],
                          eval("meta Nil.", global, global));

    ObjectPtr contError(clone(error));
    contError.lock()->put(Symbols::get()["message"],
                          eval("\"Continuation error\".", global, global));

    ObjectPtr parseError(clone(error));
    parseError.lock()->put(Symbols::get()["message"],
                           eval("\"Parse error\".", global, global));

    ObjectPtr boundsError(clone(error));
    boundsError.lock()->put(Symbols::get()["message"],
                            eval("\"Bounds error\".", global, global));

    meta.lock()->put(Symbols::get()["SystemCallError"], systemCallError);
    meta.lock()->put(Symbols::get()["SystemArgError"], systemArgError);
    meta.lock()->put(Symbols::get()["StreamError"], streamError);
    meta.lock()->put(Symbols::get()["TypeError"], typeError);
    meta.lock()->put(Symbols::get()["SlotError"], slotError);
    meta.lock()->put(Symbols::get()["ContError"], contError);
    meta.lock()->put(Symbols::get()["ParseError"], parseError);
    meta.lock()->put(Symbols::get()["BoundsError"], boundsError);
    global.lock()->put(Symbols::get()["SystemCallError"], systemCallError);
    global.lock()->put(Symbols::get()["SystemArgError"], systemArgError);
    global.lock()->put(Symbols::get()["StreamError"], streamError);
    global.lock()->put(Symbols::get()["TypeError"], typeError);
    global.lock()->put(Symbols::get()["SlotError"], slotError);
    global.lock()->put(Symbols::get()["ContError"], contError);
    global.lock()->put(Symbols::get()["ParseError"], parseError);
    global.lock()->put(Symbols::get()["BoundsError"], boundsError);
}

// TODO Take a lot of the ', global, global);' statements and make them scoped

ProtoError doSystemArgError(ObjectPtr global,
                                string name,
                                int expected,
                                int got) {
    ObjectPtr meta_ = meta(global, global);
    ObjectPtr err = clone(getInheritedSlot(global, meta_, Symbols::get()["SystemArgError"]));
    err.lock()->put(Symbols::get()["gotArguments"], garnish(global, got));
    err.lock()->put(Symbols::get()["expectedArguments"], garnish(global, expected));
    err.lock()->put(Symbols::get()["functionName"], garnish(global, name));
    return ProtoError(err);
}

ProtoError doSlotError(ObjectPtr global, ObjectPtr problem, Symbolic slotName) {
    ObjectPtr meta_ = meta(global, global);
    ObjectPtr err = clone(getInheritedSlot(global, meta_, Symbols::get()["SlotError"]));
    err.lock()->put(Symbols::get()["slotName"], garnish(global, slotName));
    err.lock()->put(Symbols::get()["objectInstance"], problem);
    return ProtoError(err);
}

ProtoError doParseError(ObjectPtr global) {
    ObjectPtr meta_ = meta(global, global);
    ObjectPtr err = clone(getInheritedSlot(global, meta_, Symbols::get()["ParseError"]));
    return ProtoError(err);
}

ProtoError doParseError(ObjectPtr global, string message) {
    ObjectPtr meta_ = meta(global, global);
    ObjectPtr err = clone(getInheritedSlot(global, meta_, Symbols::get()["ParseError"]));
    err.lock()->put(Symbols::get()["message"], garnish(global, message));
    return ProtoError(err);
}

ProtoError doEtcError(ObjectPtr global, string errorName, string msg) {
    ObjectPtr meta_ = meta(global, global);
    ObjectPtr err = clone(getInheritedSlot(global, meta_, Symbols::get()[errorName]));
    err.lock()->put(Symbols::get()["message"], garnish(global, msg));
    return ProtoError(err);
}

