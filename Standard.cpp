#include "Standard.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include "GC.hpp"
#include "Cont.hpp"
#include <list>
#include <sstream>

using namespace std;

// TODO Failed lookups should return nil or err properly, not segfault

void spawnSystemCalls(ObjectPtr& global, ObjectPtr& systemCall, ObjectPtr& sys);

ObjectPtr spawnObjects() {

    ObjectPtr object(GC::get().allocate());
    ObjectPtr meta(clone(object));
    ObjectPtr global(clone(object));
    ObjectPtr proc(clone(object));
    ObjectPtr method(clone(proc));
    ObjectPtr number(clone(object));
    ObjectPtr string(clone(object));
    ObjectPtr symbol(clone(object));
    ObjectPtr nil(clone(object));
    ObjectPtr boolean(clone(object));
    ObjectPtr true_(clone(boolean));
    ObjectPtr false_(clone(boolean));
    ObjectPtr systemCall(clone(method));
    ObjectPtr cont(clone(proc));
    ObjectPtr contValidator(clone(object));

    ObjectPtr stream(clone(object));
    ObjectPtr stdout_(clone(stream));
    ObjectPtr stdin_(clone(stream));
    ObjectPtr stderr_(clone(stream));

    ObjectPtr sys(clone(object));

    // Object is its own parent
    object.lock()->put(Symbols::get()["parent"], object);

    // Meta linkage
    meta.lock()->put(Symbols::get()["meta"], meta);
    object.lock()->put(Symbols::get()["meta"], meta);

    // Primitives (Method, double, string)
    method.lock()->prim(Method());
    number.lock()->prim(0.0);
    string.lock()->prim("");
    symbol.lock()->prim(Symbols::get()["0"]); // TODO Better default?
    contValidator.lock()->prim(weak_ptr<SignalValidator>());

    // Global scope contains basic types
    global.lock()->put(Symbols::get()["Global"], global);
    global.lock()->put(Symbols::get()["Object"], global);
    global.lock()->put(Symbols::get()["Proc"], proc);
    global.lock()->put(Symbols::get()["Method"], method);
    global.lock()->put(Symbols::get()["Number"], number);
    global.lock()->put(Symbols::get()["String"], string);
    global.lock()->put(Symbols::get()["Symbol"], symbol);
    global.lock()->put(Symbols::get()["Stream"], stream);
    global.lock()->put(Symbols::get()["stdin"], stdin_);
    global.lock()->put(Symbols::get()["stderr"], stderr_);
    global.lock()->put(Symbols::get()["stdout"], stdout_);
    global.lock()->put(Symbols::get()["SystemCall"], systemCall);
    global.lock()->put(Symbols::get()["True"], true_);
    global.lock()->put(Symbols::get()["False"], false_);
    global.lock()->put(Symbols::get()["Nil"], nil);
    global.lock()->put(Symbols::get()["Boolean"], boolean);
    global.lock()->put(Symbols::get()["Cont"], cont);

    // Meta calls for basic types
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

    // Method and system call properties
    spawnSystemCalls(global, systemCall, sys);

    // Procs and Methods
    method.lock()->put(Symbols::get()["closure"], global);
    proc.lock()->put(Symbols::get()["call"], clone(method));
    // TODO Should this use `meta Proc clone` or `Proc clone`? (Note that currently
    //      `meta Proc clone` does not exist)
    global.lock()->put(Symbols::get()["proc"], eval(R"({ curr := Proc clone.
                                         curr call := { parent dynamic $1. }.
                                         curr. }.)", global, global));

    // Continuations
    cont.lock()->put(Symbols::get()["tag"], nil);
    cont.lock()->put(Symbols::get()["call"],
                     eval("{ meta sys exitCC#: lexical, dynamic, self tag, $1. }.",
                          global, global));
    global.lock()->put(Symbols::get()["callCC"],
                       eval("{meta sys callCC#: lexical, dynamic, {parent dynamic $1.}.}.",
                            global, global));

    // The basics for cloning and metaprogramming
    object.lock()->put(Symbols::get()["clone"], eval("{ meta sys doClone#: self. }.",
                                                     global, global));
    object.lock()->put(Symbols::get()["is"], eval("{ meta sys instanceOf#: self, $1. }.",
                                                  global, global));
    object.lock()->put(Symbols::get()["slot"], eval("{ meta sys accessSlot#: self, $1. }.",
                                    global, global));
    object.lock()->put(Symbols::get()["get"], eval("{ meta sys accessSlot#: self, $1. }.",
                                                   global, global));
    object.lock()->put(Symbols::get()["has"], eval("{ meta sys checkSlot#: self, $1. }.",
                                    global, global));
    object.lock()->put(Symbols::get()["put"], eval("{ meta sys putSlot#: self, $1, $2. }.",
                                    global, global));

    // Exception throwing and handling routines
    global.lock()->put(Symbols::get()["handle"],
                       eval(R"({ meta sys try#: lexical, dynamic,
                                                { parent dynamic $1. },
                                                { (parent dynamic $2: $1) toBool. },
                                                { parent dynamic $3. }. }.)",
                            global, global));
    global.lock()->put(Symbols::get()["try"],
                       eval(R"({ self handle: { parent dynamic $1. },
                                              { $1 is: parent dynamic $2. },
                                              { parent dynamic $3. }. }.)",
                            global, global));
    object.lock()->put(Symbols::get()["throw"],
                       eval("{ meta sys throw#: self. }.", global, global));

    // Stream setup
    stdout_.lock()->prim(outStream());
    stdin_.lock()->prim(inStream());
    stderr_.lock()->prim(errStream());
    stream.lock()->put(Symbols::get()["in?"], eval("{ meta sys streamIn#: self. }.",
                                                   global, global));
    stream.lock()->put(Symbols::get()["out?"], eval("{ meta sys streamOut#: self. }.",
                                                    global, global));
    stream.lock()->put(Symbols::get()["puts"], eval("{meta sys streamPuts#: self, $1.}.",
                                                    global, global));
    stream.lock()->put(Symbols::get()["putln"], eval("{meta sys streamPutln#: self, $1.}.",
                                     global, global));
    stream.lock()->put(Symbols::get()["print"], eval("{ self puts: $1 toString. }.",
                                                     global, global));
    stream.lock()->put(Symbols::get()["println"], eval("{ self putln: $1 toString. }.",
                                                       global, global));
    stream.lock()->put(Symbols::get()["dump"],
                       eval("{meta sys streamDump#: lexical, dynamic, self, $1.}.",
                            global, global));

    // Self-reference in scopes, etc.
    global.lock()->put(Symbols::get()["scope"], eval("{ self. }.", global, global));
    global.lock()->put(Symbols::get()["$scope"], eval("{ self. }.", global, global));
    object.lock()->put(Symbols::get()["me"], eval("{ self. }.", global, global));

    // More method setup (now that we have system calls)
    method.lock()->put(Symbols::get()["call"], eval("{ self. }.", global, global));

    // Symbol Functions
    symbol.lock()->put(Symbols::get()["gensym"], eval("{ meta sys gensym#: self. }.",
                                                      global, global));
    symbol.lock()->put(Symbols::get()["gensymOf"],
                       eval("{ meta sys gensymOf#: self, $1. }.",
                            global, global));

    // Boolean casts and operations
    object.lock()->put(Symbols::get()["toBool"], true_);
    false_.lock()->put(Symbols::get()["toBool"], false_);
    nil.lock()->put(Symbols::get()["toBool"], false_);
    global.lock()->put(Symbols::get()["if"], eval(R"({
                               meta sys ifThenElse#: dynamic,
                                                     $1 toBool,
                                                     { parent dynamic $2. },
                                                     { parent dynamic $3. }.
                             }.)",
                           global, global));
    object.lock()->put(Symbols::get()["ifTrue"], eval(R"({ if: self,
                                        { parent dynamic $1. },
                                        { meta Nil. }.
                                  }.)", global, global));
    object.lock()->put(Symbols::get()["ifFalse"], eval(R"({ if: self,
                                        { meta Nil. },
                                        { parent dynamic $1. }.
                                  }.)", global, global));
    object.lock()->put(Symbols::get()["and"], eval(R"({ if: self,
                                            { parent dynamic $1. },
                                            { meta False. }. }.)", global, global));
    object.lock()->put(Symbols::get()["or"], eval(R"({ if: self,
                                            { parent self. },
                                            { parent dynamic $1. }. }.)", global, global));

    // Ordinary objects print very simply
    object.lock()->put(Symbols::get()["toString"], eval("\"Object\".", global, global));

    // Basic data types print appropriately
    method.lock()->put(Symbols::get()["toString"], eval("\"Method\".", global, global));
    proc.lock()->put(Symbols::get()["toString"], eval("\"Proc\".", global, global));
    stream.lock()->put(Symbols::get()["toString"], eval("\"Stream\".", global, global));
    string.lock()->put(Symbols::get()["toString"], eval("{meta sys primToString#: self.}.",
                                        global, global));
    symbol.lock()->put(Symbols::get()["toString"], eval("{meta sys primToString#: self.}.",
                                        global, global));
    number.lock()->put(Symbols::get()["toString"], eval("{meta sys primToString#: self.}.",
                                        global, global));
    boolean.lock()->put(Symbols::get()["toString"], eval("\"Boolean\".", global, global));
    true_.lock()->put(Symbols::get()["toString"], eval("\"True\".", global, global));
    false_.lock()->put(Symbols::get()["toString"], eval("\"False\".", global, global));
    nil.lock()->put(Symbols::get()["toString"], eval("\"Nil\".", global, global));

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

    systemCall.lock()->prim([&global](list<ObjectPtr> lst) {
            return eval("meta Nil.", global, global);
        });
    callStreamIn.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr stream;
            if (bindArguments(lst, stream)) {
                auto prim = stream->prim();
                if (auto call = boost::get<StreamPtr>(&prim))
                    return garnish(global, (*call)->hasIn());
                else
                    return eval("meta False.", global, global);
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callStreamOut.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr stream;
            if (bindArguments(lst, stream)) {
                auto prim = stream->prim();
                if (auto call = boost::get<StreamPtr>(&prim))
                    return garnish(global, (*call)->hasOut());
                else
                    return eval("meta False.", global, global);
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callStreamPuts.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr stream, obj;
            if (bindArguments(lst, stream, obj)) {
                auto stream0 = stream->prim();
                auto obj0 = obj->prim();
                if (auto stream1 = boost::get<StreamPtr>(&stream0)) {
                    if (auto obj1 = boost::get<string>(&obj0)) {
                        if (!(*stream1)->hasOut())
                            return eval("meta Nil.", global, global); // TODO Throw error
                        (*stream1)->writeText(*obj1);
                        return eval("meta Nil.", global, global); // TODO Return value?
                    } else {
                        return eval("meta Nil.", global, global); // TODO Throw error
                    }
                } else {
                    return eval("meta Nil.", global, global); // TODO Throw error
                }
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callStreamPutln.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr stream, obj;
            if (bindArguments(lst, stream, obj)) {
                auto stream0 = stream->prim();
                auto obj0 = obj->prim();
                if (auto stream1 = boost::get<StreamPtr>(&stream0)) {
                    if (auto obj1 = boost::get<string>(&obj0)) {
                        if (!(*stream1)->hasOut())
                            return eval("meta Nil.", global, global); // TODO Throw error
                        (*stream1)->writeLine(*obj1);
                        return eval("meta Nil.", global, global); // TODO Return value?
                    } else {
                        return eval("meta Nil.", global, global); // TODO Throw error
                    }
                } else {
                    return eval("meta Nil.", global, global); // TODO Throw error
                }
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callPrimToString.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                string result = primToString(obj);
                return garnish(global, result);
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callIfStatement.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr dyn, cond, tr, fl;
            if (bindArguments(lst, dyn, cond, tr, fl)) {
                auto result = eval("meta Nil.", global, global);
                // These are unused except to verify that the prim() is correct
                auto tr_ = boost::get<Method>(&tr->prim());
                auto fl_ = boost::get<Method>(&fl->prim());
                if ((tr_ == NULL) || (fl_ == NULL))
                    return eval("meta Nil.", global, global); // TODO Throw error
                //
                auto definitelyTrue = eval("meta True.", global, global).lock();
                if (cond == definitelyTrue)
                    return callMethod(result, nullptr, tr, clone(dyn));
                else
                    return callMethod(result, nullptr, fl, clone(dyn));
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callStreamDump.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr lex, dyn, stream, obj;
            if (bindArguments(lst, lex, dyn, stream, obj)) {
                if (auto stream0 = boost::get<StreamPtr>(&stream->prim())) {
                    dumpObject(lex, dyn, **stream0, obj);
                    return eval("meta Nil.", global, global);
                } else {
                    return eval("meta Nil.", global, global); // TODO Throw error
                }
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callClone.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                return clone(obj);
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callGetSlot.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr obj, slot;
            if (bindArguments(lst, obj, slot)) {
                if (auto sym = boost::get<Symbolic>(&slot->prim())) {
                    // TODO Check hasInheritedSlot and see if need to throw error
                    return getInheritedSlot(obj, *sym);
                } else {
                    return eval("meta Nil.", global, global); // TODO Throw error
                }
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callHasSlot.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr obj, slot;
            if (bindArguments(lst, obj, slot)) {
                if (auto sym = boost::get<Symbolic>(&slot->prim())) {
                    return garnish(global,
                                   hasInheritedSlot(obj, *sym));
                } else {
                    return eval("meta False.", global, global);
                }
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callPutSlot.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr obj, slot, value;
            if (bindArguments(lst, obj, slot, value)) {
                if (auto sym = boost::get<Symbolic>(&slot->prim())) {
                    obj->put(*sym, value);
                    return ObjectPtr(value);
                } else {
                    return eval("meta Nil.", global, global); // TODO Throw error
                }
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callGensym.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr symbol;
            if (bindArguments(lst, symbol)) {
                ObjectPtr gen = clone(symbol);
                gen.lock()->prim( Symbols::gensym() );
                return gen;
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callGensymOf.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr symbol, name;
            if (bindArguments(lst, symbol, name)) {
                if (auto str = boost::get<std::string>(&name->prim())) {
                    ObjectPtr gen = clone(symbol);
                    gen.lock()->prim( Symbols::gensym(*str) );
                    return gen;
                } else {
                    return eval("meta Nil.", global, global); // TODO Throw error
                }
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callCallCC.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr lex, dyn, mthd;
            if (bindArguments(lst, lex, dyn, mthd)) {
                // Dies when it goes out of scope
                shared_ptr<SignalValidator> livingTag(new SignalValidator());
                //
                ObjectPtr dyn1 = clone(dyn);
                Symbolic sym = Symbols::gensym("CONT");
                ObjectPtr symObj = getInheritedSlot(meta(lex),
                                                    Symbols::get()["Symbol"]);
                ObjectPtr symObj1 = clone(symObj);
                symObj1.lock()->prim(sym);
                ObjectPtr validator = getInheritedSlot(meta(lex),
                                                       Symbols::get()["ContValidator"]);
                ObjectPtr validator1 = clone(validator);
                validator1.lock()->prim(weak_ptr<SignalValidator>(livingTag));
                dyn1.lock()->put(sym, validator1);
                ObjectPtr cont = getInheritedSlot(meta(lex),
                                                  Symbols::get()["Cont"]);
                ObjectPtr cont1 = clone(cont);
                cont1.lock()->put(Symbols::get()["tag"], symObj1);
                try {
                    ObjectPtr result = eval("meta Nil.", global, global);
                    dyn1.lock()->put(Symbols::get()["$1"], cont1);
                    return callMethod(result, ObjectSPtr(), mthd, dyn1);
                } catch (Signal& signal) {
                    if (signal.match(sym)) {
                        return signal.getObject();
                    } else {
                        throw;
                    }
                }
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callExitCC.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr lex, dyn, tag, arg;
            if (bindArguments(lst, lex, dyn, tag, arg)) {
                if (auto sym = boost::get<Symbolic>(&tag->prim())) {
                    if (hasInheritedSlot(dyn, *sym)) {
                        auto validator = getInheritedSlot(dyn, *sym);
                        if (auto val =
                            boost::get< weak_ptr<SignalValidator> >(&validator.lock()
                                                                    ->prim())) {
                            if (val->expired())
                                return eval("meta Nil.",
                                            global, global); // TODO Throw error
                            throw Signal(*sym, arg);
                        } else {
                            return eval("meta Nil.", global, global); // TODO Throw error
                        }
                    } else {
                        return eval("meta Nil.", global, global); // TODO Throw error
                    }
                } else {
                    return eval("meta Nil.", global, global); // TODO Throw error
                }
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callInstanceof.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr obj0, obj1;
            if (bindArguments(lst, obj0, obj1)) {
                auto hier = hierarchy(obj0);
                bool result = (find_if(hier.begin(), hier.end(),
                                       [&obj1](auto& o){ return o.lock() == obj1; })
                               != hier.end());
                return garnish(global, result);
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callTryStmt.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr lex, dyn, lhs, cond, rhs;
            if (bindArguments(lst, lex, dyn, lhs, cond, rhs)) {
                ObjectPtr result = eval("meta Nil.", global, global);
                try {
                    ObjectPtr dyn1 = clone(dyn);
                    return callMethod(result, ObjectSPtr(), lhs, dyn1);
                } catch (ProtoError& err) {
                    ObjectPtr dyn1 = clone(dyn);
                    dyn1.lock()->put(Symbols::get()["$1"], err.getObject());
                    ObjectPtr result1 = callMethod(result, ObjectSPtr(), cond, dyn1);
                    auto definitelyTrue = eval("meta True.", global, global).lock();
                    if (result1.lock() == definitelyTrue) {
                        ObjectPtr dyn2 = clone(dyn);
                        dyn2.lock()->put(Symbols::get()["$1"], err.getObject());
                        return callMethod(result, ObjectSPtr(), rhs, dyn2);
                    } else {
                        throw;
                    }
                }
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callThrowStmt.lock()->prim([&global](list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                throwProtoError(obj);
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
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

}

// TODO Take a lot of the ', global, global);' statements and make them scoped
