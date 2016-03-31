#include "Standard.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include "GC.hpp"
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

    ObjectPtr stream(clone(object));
    ObjectPtr stdout_(clone(stream));
    ObjectPtr stdin_(clone(stream));
    ObjectPtr stderr_(clone(stream));

    ObjectPtr sys(clone(object));

    // Object is its own parent
    object.lock()->put("parent", object);

    // Meta linkage
    meta.lock()->put("meta", meta);
    object.lock()->put("meta", meta);

    // Primitives (Method, double, string)
    method.lock()->prim(Method());
    number.lock()->prim(0.0);
    string.lock()->prim("");
    symbol.lock()->prim(Symbols::get()["0"]); // TODO Better default?

    // Global scope contains basic types
    global.lock()->put("Global", global);
    global.lock()->put("Object", global);
    global.lock()->put("Proc", proc);
    global.lock()->put("Method", method);
    global.lock()->put("Number", number);
    global.lock()->put("String", string);
    global.lock()->put("Symbol", symbol);
    global.lock()->put("Stream", stream);
    global.lock()->put("stdin", stdin_);
    global.lock()->put("stderr", stderr_);
    global.lock()->put("stdout", stdout_);
    global.lock()->put("SystemCall", systemCall);
    global.lock()->put("True", true_);
    global.lock()->put("False", false_);
    global.lock()->put("Nil", nil);
    global.lock()->put("Boolean", boolean);

    // Meta calls for basic types
    meta.lock()->put("Method", method);
    meta.lock()->put("Number", number);
    meta.lock()->put("String", string);
    meta.lock()->put("Symbol", symbol);
    meta.lock()->put("Stream", stream);
    meta.lock()->put("SystemCall", systemCall);
    meta.lock()->put("True", true_);
    meta.lock()->put("False", false_);
    meta.lock()->put("Nil", nil);
    meta.lock()->put("Boolean", boolean);
    meta.lock()->put("sys", sys);

    // Method and system call properties
    spawnSystemCalls(global, systemCall, sys);

    // Procs and Methods
    method.lock()->put("closure", global);
    proc.lock()->put("call", clone(method));
    // TODO Should this use `meta Proc clone` or `Proc clone`?
    global.lock()->put("proc", eval(R"({ curr := Proc clone.
                                         curr call := { parent dynamic $1. }.
                                         curr. }.)", global, global));

    // The basics for cloning and metaprogramming
    object.lock()->put("clone", eval("{ meta sys doClone: self. }.", global, global));
    object.lock()->put("slot", eval("{ meta sys accessSlot: self, $1. }.",
                                    global, global));
    object.lock()->put("has", eval("{ meta sys checkSlot: self, $1. }.",
                                    global, global));

    // Stream setup
    stdout_.lock()->prim(outStream());
    stdin_.lock()->prim(inStream());
    stderr_.lock()->prim(errStream());
    stream.lock()->put("in?", eval("{ meta sys streamIn?: self. }.", global, global));
    stream.lock()->put("out?", eval("{ meta sys streamOut?: self. }.", global, global));
    stream.lock()->put("puts", eval("{meta sys streamPuts: self, $1.}.", global, global));
    stream.lock()->put("putln", eval("{meta sys streamPutln: self, $1.}.",
                                     global, global));
    stream.lock()->put("print", eval("{ self puts: $1 toString. }.", global, global));
    stream.lock()->put("println", eval("{ self putln: $1 toString. }.", global, global));
    stream.lock()->put("dump", eval("{meta sys streamDump: lexical, dynamic, self, $1.}.",
                             global, global));

    // Self-reference in scopes, etc.
    global.lock()->put("scope", eval("{ self. }.", global, global));
    global.lock()->put("$scope", eval("{ self. }.", global, global));
    // These are now done much more reliably in callMethod directly
    //global->put("lexical", eval("{ self. }.", global, global));
    //global->put("dynamic", eval("{ $scope parent. }.", global, global));
    object.lock()->put("me", eval("{ self. }.", global, global));

    // More method setup (now that we have system calls)
    method.lock()->put("call", eval("{ self. }.", global, global));

    // Boolean casts and operations
    object.lock()->put("toBool", true_);
    false_.lock()->put("toBool", false_);
    nil.lock()->put("toBool", false_);
    global.lock()->put("if", eval(R"({
                               meta sys ifThenElse: dynamic,
                                                    $1 toBool,
                                                    { parent dynamic $2. },
                                                    { parent dynamic $3. }.
                             }.)",
                           global, global));
    object.lock()->put("ifTrue", eval(R"({ if: self,
                                        { parent dynamic $1. },
                                        { meta Nil. }.
                                  }.)", global, global));
    object.lock()->put("ifFalse", eval(R"({ if: self,
                                        { meta Nil. },
                                        { parent dynamic $1. }.
                                  }.)", global, global));
    object.lock()->put("and", eval(R"({ if: self,
                                            { parent dynamic $1. },
                                            { meta False. }. }.)", global, global));
    object.lock()->put("or", eval(R"({ if: self,
                                            { parent self. },
                                            { parent dynamic $1. }. }.)", global, global));

    // Ordinary objects print very simply
    object.lock()->put("toString", eval("\"Object\".", global, global));

    // Basic data types print appropriately
    method.lock()->put("toString", eval("\"Method\".", global, global));
    proc.lock()->put("toString", eval("\"Proc\".", global, global));
    stream.lock()->put("toString", eval("\"Stream\".", global, global));
    string.lock()->put("toString", eval("{ meta sys primToString: self. }.",
                                        global, global));
    symbol.lock()->put("toString", eval("{ meta sys primToString: self. }.",
                                        global, global));
    number.lock()->put("toString", eval("{ meta sys primToString: self. }.",
                                        global, global));
    boolean.lock()->put("toString", eval("\"Boolean\".", global, global));
    true_.lock()->put("toString", eval("\"True\".", global, global));
    false_.lock()->put("toString", eval("\"False\".", global, global));
    nil.lock()->put("toString", eval("\"Nil\".", global, global));

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
                    return getInheritedSlot(obj, Symbols::get()[sym->index]);
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
                                   hasInheritedSlot(obj, Symbols::get()[sym->index]));
                } else {
                    return eval("meta False.", global, global);
                }
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });

    sys.lock()->put("streamIn?", callStreamIn);
    sys.lock()->put("streamOut?", callStreamOut);
    sys.lock()->put("streamPuts", callStreamPuts);
    sys.lock()->put("streamPutln", callStreamPutln);
    sys.lock()->put("primToString", callPrimToString);
    sys.lock()->put("ifThenElse", callIfStatement);
    sys.lock()->put("streamDump", callStreamDump);
    sys.lock()->put("doClone", callClone);
    sys.lock()->put("accessSlot", callGetSlot);
    sys.lock()->put("checkSlot", callHasSlot);

}

// TODO Take a lot of the ', global, global);' statements and make them scoped
// TODO Reset the line number when we parse a second time (should be easy)
