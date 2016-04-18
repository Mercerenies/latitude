#include "Standard.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include "GC.hpp"
#include "Cont.hpp"
#include <list>
#include <sstream>
#include <boost/scope_exit.hpp>

using namespace std;

// TODO More objects should have toString so they don't all default to showing "Object"

void spawnSystemCalls(ObjectPtr& global, ObjectPtr& systemCall, ObjectPtr& sys);
void spawnExceptions(ObjectPtr& global, ObjectPtr& error, ObjectPtr& meta);

ProtoError doSystemArgError(ObjectPtr global, string name, int expected, int got);
ProtoError doSlotError(ObjectPtr global, ObjectPtr problem, string slotName);
ProtoError doParseError(ObjectPtr global);
ProtoError doParseError(ObjectPtr global, string message);
ProtoError doEtcError(ObjectPtr global, string errorName, string msg);

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
    ObjectPtr exception(clone(object));
    ObjectPtr systemError(clone(exception));
    ObjectPtr latchkey(clone(object));
    ObjectPtr lockbox(clone(object));

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
    global.lock()->put(Symbols::get()["global"], global);
    global.lock()->put(Symbols::get()["Object"], object);
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
    global.lock()->put(Symbols::get()["Exception"], exception);
    global.lock()->put(Symbols::get()["SystemError"], systemError);
    global.lock()->put(Symbols::get()["Lockbox"], lockbox);
    global.lock()->put(Symbols::get()["Latchkey"], latchkey);

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

    // Method and system call properties
    spawnSystemCalls(global, systemCall, sys);

    // Exceptions
    exception.lock()->put(Symbols::get()["message"],
                          eval("\"Exception!\".", global, global));
    spawnExceptions(global, systemError, meta);

    // Procs and Methods
    method.lock()->put(Symbols::get()["closure"], global);
    proc.lock()->put(Symbols::get()["call"], clone(method));
    global.lock()->put(Symbols::get()["proc"], eval(R"({ curr := self Proc clone.
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
    object.lock()->put(Symbols::get()["hold"],
                       eval("{ meta sys accessSlot#: self, $1. }.",
                            global, global));
    object.lock()->put(Symbols::get()["get"], eval("{ self hold me. }.",
                                                   global, global));
    object.lock()->put(Symbols::get()["has"], eval("{ meta sys checkSlot#: self, $1. }.",
                                    global, global));
    object.lock()->put(Symbols::get()["put"],
                       eval("{ meta sys putSlot#: self, $1, (dynamic hold: '$2). }.",
                            global, global));
    object.lock()->put(Symbols::get()["origin"],
                       eval("{ meta sys origin#: self, $1. }.",
                            global, global));
    // TODO "above"

    // Exception throwing and handling routines
    method.lock()->put(Symbols::get()["handle"],
                       eval(R"({ meta sys try#: lexical, dynamic,
                                                { parent self. },
                                                { (parent dynamic $1: $1) toBool. },
                                                { parent dynamic $2. }. }.)",
                            global, global));
    method.lock()->put(Symbols::get()["catch"],
                       eval(R"({ ({ parent self. }) handle:
                                   { $1 is: parent dynamic $1. },
                                   { parent dynamic $2. }. }.)",
                          global, global));
    object.lock()->put(Symbols::get()["throw"],
                       eval("{ meta sys throw#: self. }.", global, global));
    method.lock()->put(Symbols::get()["protect"],
                       eval(R"({ meta sys scopeProtect#: lexical, dynamic,
                                                         { parent self. },
                                                         { parent dynamic $1. }. }.)",
                            global, global));

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
    stream.lock()->put(Symbols::get()["readln"], eval("{ meta sys streamRead#: self. }.",
                                                      global, global));

    // Self-reference in scopes, etc.
    global.lock()->put(Symbols::get()["scope"], eval("{ self. }.", global, global));
    global.lock()->put(Symbols::get()["$scope"], eval("{ self. }.", global, global));
    object.lock()->put(Symbols::get()["me"], eval("{ self. }.", global, global));
    object.lock()->put(Symbols::get()["invoke"],
                       eval(R"({ pr := meta Proc clone.
                                 pr call := { meta sys invoke#: lexical,
                                                                dynamic,
                                                                parent self,
                                                                (parent dynamic
                                                                 hold '$1).}.
                                 pr. }.)",
                            global, global));
    global.lock()->put(Symbols::get()["here"],
                       eval("{ if: (has 'again), { hold 'again. }, { meta Nil. }. }.",
                            global, global));

    // More method setup (now that we have system calls)
    method.lock()->put(Symbols::get()["call"], eval("{ self. }.", global, global));

    // Symbol Functions
    symbol.lock()->put(Symbols::get()["gensym"], eval("{ meta sys gensym#: self. }.",
                                                      global, global));
    symbol.lock()->put(Symbols::get()["gensymOf"],
                       eval("{ meta sys gensymOf#: self, $1. }.",
                            global, global));
    string.lock()->put(Symbols::get()["intern"],
                       eval("{ meta sys intern#: lexical, self. }.",
                            global, global));
    symbol.lock()->put(Symbols::get()["asText"],
                       eval("{ meta sys symbolic#: lexical, self. }.",
                            global, global));

    // Basic arithmetic operations
    number.lock()->put(Symbols::get()["+"], eval("{ meta sys numAdd#: self, $1. }.",
                                                 global, global));
    number.lock()->put(Symbols::get()["-"], eval("{ meta sys numSub#: self, $1. }.",
                                                 global, global));
    number.lock()->put(Symbols::get()["*"], eval("{ meta sys numMul#: self, $1. }.",
                                                 global, global));
    number.lock()->put(Symbols::get()["/"], eval("{ meta sys numDiv#: self, $1. }.",
                                                 global, global));
    number.lock()->put(Symbols::get()["mod"], eval("{ meta sys numMod#: self, $1. }.",
                                                   global, global));

    // Latchkeys and Lockboxes
    latchkey.lock()->put(Symbols::get()["tag"], symbol);
    latchkey.lock()->put(Symbols::get()["make"],
                         eval(R"({ key := self clone.
                                   key tag := key tag gensymOf: "KEY".
                                   key. }.)", global, global));
    lockbox.lock()->put(Symbols::get()["store"],
                        eval(R"({ self put: $1 tag, $2. }.)", global, global));
    lockbox.lock()->put(Symbols::get()["fits"],
                        eval(R"({ self has: $1 tag. }.)", global, global));
    lockbox.lock()->put(Symbols::get()["retrieve"],
                        eval(R"({ if: { parent self fits: parent dynamic $1. },
                                      { parent self get: parent dynamic $1 tag. },
                                      { meta Nil. }. }.)", global, global));

    // Boolean casts and operations
    object.lock()->put(Symbols::get()["toBool"], true_);
    false_.lock()->put(Symbols::get()["toBool"], false_);
    nil.lock()->put(Symbols::get()["toBool"], false_);
    global.lock()->put(Symbols::get()["if"], eval(R"({
                               meta sys ifThenElse#: lexical,
                                                     dynamic,
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
    object.lock()->put(Symbols::get()["not"], eval(R"({ if: self,
                                            { meta False. },
                                            { meta True. }. }.)", global, global));

    // Stringification
    // The `stringify` method calls `toString` unless the object is already a string,
    // in which case it returns the object itself.
    object.lock()->put(Symbols::get()["stringify"], eval("{ self toString. }.",
                                                         global, global));
    string.lock()->put(Symbols::get()["stringify"], eval("{ self. }.", global, global));
    object.lock()->put(Symbols::get()["++"],
                       eval("{ meta sys stringConcat#: self stringify, $1 stringify. }.",
                            global, global));

    // Equality
    // Triple-equals is used for pointer equality and should almost never
    // be overriden by children. Double-equals defaults to pointer equality
    // but can and should be overriden if a better version of "conceptual"
    // equality exists for the type
    object.lock()->put(Symbols::get()["==="], eval("{ meta sys ptrEquals#: self, $1. }.",
                                                   global, global));
    object.lock()->put(Symbols::get()["=="], eval("{ (self) === ($1). }.",
                                                  global, global));

    // Ordinary objects print very simply
    // NOTE: The following analogy is appropriate:
    //           `toString` is to `pretty` in this language as
    //           `__repr__` is to `__str__` in Python. The `toString`
    //           should print a composable machine representation for
    //           debug use, and the `pretty` should print a user-friendly string.
    //           Note that `pretty` defaults to `toString`, so if you only wish
    //           to implement one, implement `toString`.
    object.lock()->put(Symbols::get()["toString"], eval("\"Object\".", global, global));
    object.lock()->put(Symbols::get()["pretty"],
                       eval("{ self toString. }.", global, global));

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
    exception.lock()->put(Symbols::get()["toString"],
                          eval("\"Exception\".", global, global));
    lockbox.lock()->put(Symbols::get()["toString"],
                        eval("\"Lockbox\".", global, global));
    latchkey.lock()->put(Symbols::get()["toString"],
                         eval("\"Latchkey\".", global, global));

    // TODO The pretty prints here in more detail once we have string formatting
    //      stuff.
    lockbox.lock()->put(Symbols::get()["pretty"],
                        eval("{ self toString. }.", global, global));
    latchkey.lock()->put(Symbols::get()["pretty"],
                         eval("{ self toString. }.", global, global));
    exception.lock()->put(Symbols::get()["pretty"],
                          eval("{ self message. }.", global, global));
    string.lock()->put(Symbols::get()["pretty"],
                       eval("{ self. }.", global, global));
    symbol.lock()->put(Symbols::get()["pretty"],
                       eval("{ self asText. }.", global, global));

    // Equality comparisons for basic types
    string.lock()->put(Symbols::get()["=="],
                       eval("{ meta sys primEquals#: self, $1. }.",
                            global, global));
    symbol.lock()->put(Symbols::get()["=="],
                       eval("{ meta sys primEquals#: self, $1. }.",
                            global, global));
    number.lock()->put(Symbols::get()["=="],
                       eval("{ meta sys primEquals#: self, $1. }.",
                            global, global));
    latchkey.lock()->put(Symbols::get()["=="],
                         eval("{ (self tag) == (self tag). }.",
                              global, global));

    // Relational Comparison Operators
    // The primary relational operators are all based on the < and == operators.
    // A type which defines these two appropriately will have the others provided.
    object.lock()->put(Symbols::get()[">"],
                       eval("{ ($1) < (self). }.", global, global));
    object.lock()->put(Symbols::get()[">="],
                       eval("{ ((self) > ($1)) or ((self) == ($1)). }.", global, global));
    object.lock()->put(Symbols::get()["<="],
                       eval("{ ((self) < ($1)) or ((self) == ($1)). }.", global, global));
    object.lock()->put(Symbols::get()["/="],
                       eval("{ ((self) == ($1)) not. }.", global, global));

    // Relational operators on built-in types
    method.lock()->put(Symbols::get()["<"], // Evaluate the method and then try again
                       eval("{ (self) < ($1). }.", global, global));
    string.lock()->put(Symbols::get()["<"],
                       eval("{ meta sys primLT#: self, $1. }.",
                            global, global));
    symbol.lock()->put(Symbols::get()["<"],
                       eval("{ meta sys primLT#: self, $1. }.",
                            global, global));
    number.lock()->put(Symbols::get()["<"],
                       eval("{ meta sys primLT#: self, $1. }.",
                            global, global));

    // Numerical Type Checking
    number.lock()->put(Symbols::get()["isBasicInt?"],
                       eval("{ (meta sys numLevel#: self) <= 0. }.", global, global));
    number.lock()->put(Symbols::get()["isInteger?"],
                       eval("{ (meta sys numLevel#: self) <= 1. }.", global, global));
    number.lock()->put(Symbols::get()["isRational?"],
                       eval("{ (meta sys numLevel#: self) <= 2. }.", global, global));
    number.lock()->put(Symbols::get()["isFloating?"],
                       eval("{ (meta sys numLevel#: self) == 3. }.", global, global));

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

    systemCall.lock()->prim([global](list<ObjectPtr> lst) {
            return eval("meta Nil.", global, global);
        });
    callStreamIn.lock()->prim([global](list<ObjectPtr> lst) {
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
    callStreamOut.lock()->prim([global](list<ObjectPtr> lst) {
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
    callStreamPuts.lock()->prim([global](list<ObjectPtr> lst) {
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
    callStreamPutln.lock()->prim([global](list<ObjectPtr> lst) {
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
    callPrimToString.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                string result = primToString(obj);
                return garnish(global, result);
            } else {
                throw doSystemArgError(global, "primToString#", 1, lst.size());
            }
        });
    callIfStatement.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr lex, dyn, cond, tr, fl;
            if (bindArguments(lst, lex, dyn, cond, tr, fl)) {
                // These are unused except to verify that the prim() is correct
                auto tr_ = boost::get<Method>(&tr->prim());
                auto fl_ = boost::get<Method>(&fl->prim());
                if ((tr_ == NULL) || (fl_ == NULL))
                    throw doEtcError(global, "TypeError",
                                     "If statement body is not a method");
                //
                auto definitelyTrue = eval("meta True.", global, global).lock();
                if (cond == definitelyTrue)
                    return callMethod(lex, tr, clone(dyn));
                else
                    return callMethod(lex, fl, clone(dyn));
            } else {
                throw doSystemArgError(global, "ifThenElse#", 5, lst.size());
            }
        });
    callStreamDump.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr lex, dyn, stream, obj;
            if (bindArguments(lst, lex, dyn, stream, obj)) {
                if (auto stream0 = boost::get<StreamPtr>(&stream->prim())) {
                    dumpObject(lex, dyn, **stream0, obj);
                    return eval("meta Nil.", global, global);
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Object is not a stream");
                }
            } else {
                throw doSystemArgError(global, "streamDump#", 4, lst.size());
            }
        });
    callClone.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                return clone(obj);
            } else {
                throw doSystemArgError(global, "doClone#", 1, lst.size());
            }
        });
    callGetSlot.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr obj, slot;
            if (bindArguments(lst, obj, slot)) {
                if (auto sym = boost::get<Symbolic>(&slot->prim())) {
                    return getInheritedSlot(obj, *sym);
                } else {
                    throw doEtcError(global, "TypeError", "Slot name must be a symbol");
                }
            } else {
                throw doSystemArgError(global, "accessSlot#", 2, lst.size());
            }
        });
    callHasSlot.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr obj, slot;
            if (bindArguments(lst, obj, slot)) {
                if (auto sym = boost::get<Symbolic>(&slot->prim())) {
                    return garnish(global,
                                   hasInheritedSlot(obj, *sym));
                } else {
                    return eval("meta False.", global, global);
                }
            } else {
                throw doSystemArgError(global, "checkSlot#", 2, lst.size());
            }
        });
    callPutSlot.lock()->prim([global](list<ObjectPtr> lst) {
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
    callGensym.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr symbol;
            if (bindArguments(lst, symbol)) {
                ObjectPtr gen = clone(symbol);
                gen.lock()->prim( Symbols::gensym() );
                return gen;
            } else {
                throw doSystemArgError(global, "gensym#", 1, lst.size());
            }
        });
    callGensymOf.lock()->prim([global](list<ObjectPtr> lst) {
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
    callCallCC.lock()->prim([global](list<ObjectPtr> lst) {
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
                    dyn1.lock()->put(Symbols::get()["$1"], cont1);
                    return callMethod(lex, mthd, dyn1);
                } catch (Signal& signal) {
                    if (signal.match(sym)) {
                        return signal.getObject();
                    } else {
                        throw;
                    }
                }
            } else {
                throw doSystemArgError(global, "callCC#", 3, lst.size());
            }
        });
    callExitCC.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr lex, dyn, tag, arg;
            if (bindArguments(lst, lex, dyn, tag, arg)) {
                if (auto sym = boost::get<Symbolic>(&tag->prim())) {
                    if (hasInheritedSlot(dyn, *sym)) {
                        auto validator = getInheritedSlot(dyn, *sym);
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
                throw doSystemArgError(global, "exitCC#", 4, lst.size());
            }
        });
    callInstanceof.lock()->prim([global](list<ObjectPtr> lst) {
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
    callTryStmt.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr lex, dyn, lhs, cond, rhs;
            if (bindArguments(lst, lex, dyn, lhs, cond, rhs)) {
                try {
                    ObjectPtr dyn1 = clone(dyn);
                    return callMethod(lex, lhs, dyn1);
                } catch (ProtoError& err) {
                    ObjectPtr dyn1 = clone(dyn);
                    dyn1.lock()->put(Symbols::get()["$1"], err.getObject());
                    ObjectPtr result1 = callMethod(lex, cond, dyn1);
                    auto definitelyTrue = eval("meta True.", global, global).lock();
                    if (result1.lock() == definitelyTrue) {
                        ObjectPtr dyn2 = clone(dyn);
                        dyn2.lock()->put(Symbols::get()["$1"], err.getObject());
                        return callMethod(lex, rhs, dyn2);
                    } else {
                        throw;
                    }
                }
            } else {
                throw doSystemArgError(global, "try#", 5, lst.size());
            }
        });
    callThrowStmt.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr obj;
            if (bindArguments(lst, obj)) {
                throwProtoError(obj);
                return eval("meta Nil.", global, global); // Should never happen
            } else {
                throw doSystemArgError(global, "throw#", 1, lst.size());
            }
        });
    callNumAdd.lock()->prim([global](list<ObjectPtr> lst) {
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
    callNumSub.lock()->prim([global](list<ObjectPtr> lst) {
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
    callNumMul.lock()->prim([global](list<ObjectPtr> lst) {
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
    callNumDiv.lock()->prim([global](list<ObjectPtr> lst) {
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
    callNumMod.lock()->prim([global](list<ObjectPtr> lst) {
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
    callTripleEquals.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                return garnish(global, obj1 == obj2);
            } else {
                throw doSystemArgError(global, "ptrEquals#", 2, lst.size());
            }
        });
    callPrimEquals.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                return garnish(global, primEquals(obj1, obj2));
            } else {
                throw doSystemArgError(global, "primEquals#", 2, lst.size());
            }
        });
    callStringConcat.lock()->prim([global](list<ObjectPtr> lst) {
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
    callStreamRead.lock()->prim([global](list<ObjectPtr> lst) {
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
    callScopeProtect.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr lex, dyn, block1, block2;
            if (bindArguments(lst, lex, dyn, block1, block2)) {
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
                        ObjectPtr dyn1 = clone(dyn);
                        dyn1.lock()->put(Symbols::get()["$1"], status);
                        callMethod(lex, block2, dyn1);
                    } catch (...) {
                        cerr << "Attempted abnormal exit from protected block!" << endl;
                        terminate();
                    }
                } BOOST_SCOPE_EXIT_END;
                ObjectPtr result = callMethod(lex, block1, clone(dyn));
                abnormal = false;
                return result;
            } else {
                throw doSystemArgError(global, "scopeProtect#", 4, lst.size());
            }
        });
    callInvoke.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr lex, dyn, self, mthd;
            if (bindArguments(lst, lex, dyn, self, mthd)) {
                // Calls the method with "self" bound to the given object,
                // bypassing the normal self. All arguments ($n) remain the
                // same.
                return callMethod(self, mthd, clone(dyn));
            } else {
                throw doSystemArgError(global, "invoke#", 4, lst.size());
            }
        });
    callOrigin.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr self, ref;
            if (bindArguments(lst, self, ref)) {
                auto sym = boost::get<Symbolic>(&ref->prim());
                if (!sym)
                    throw doEtcError(global, "TypeError",
                                     "Slot name must be a symbol");
                return getInheritedOrigin(self, *sym);
            } else {
                throw doSystemArgError(global, "origin#", 2, lst.size());
            }
        });
    callIntern.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr lex, self;
            if (bindArguments(lst, lex, self)) {
                if (auto str = boost::get<string>(&self->prim())) {
                    ObjectPtr sym = getInheritedSlot(meta(lex),
                                                     Symbols::get()["Symbol"]);
                    ObjectPtr sym1 = clone(sym);
                    sym1.lock()->prim(Symbols::get()[*str]);
                    return sym1;
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Object to intern is not a string");
                }
            } else {
                throw doSystemArgError(global, "intern#", 2, lst.size());
            }
        });
    callSymbolic.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr lex, self;
            if (bindArguments(lst, lex, self)) {
                if (auto sym = boost::get<Symbolic>(&self->prim())) {
                    return garnish(global, Symbols::get()[*sym]);
                } else {
                    throw doEtcError(global, "TypeError",
                                     "Object is not symbolic");
                }
            } else {
                throw doSystemArgError(global, "symbolic#", 2, lst.size());
            }
        });
    callNumLevel.lock()->prim([global](list<ObjectPtr> lst) {
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
    callPrimLT.lock()->prim([global](list<ObjectPtr> lst) {
            ObjectSPtr obj1, obj2;
            if (bindArguments(lst, obj1, obj2)) {
                return garnish(global, primLT(obj1, obj2));
            } else {
                throw doSystemArgError(global, "primLT#", 2, lst.size());
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
                          eval("\"\".", global, global));
    slotError.lock()->put(Symbols::get()["objectInstance"],
                          eval("meta Nil.", global, global));

    ObjectPtr contError(clone(error));
    contError.lock()->put(Symbols::get()["message"],
                          eval("\"Continuation error\".", global, global));

    ObjectPtr parseError(clone(error));
    parseError.lock()->put(Symbols::get()["message"],
                           eval("\"Parse error\".", global, global));

    meta.lock()->put(Symbols::get()["SystemCallError"], systemCallError);
    meta.lock()->put(Symbols::get()["SystemArgError"], systemArgError);
    meta.lock()->put(Symbols::get()["StreamError"], streamError);
    meta.lock()->put(Symbols::get()["TypeError"], typeError);
    meta.lock()->put(Symbols::get()["SlotError"], slotError);
    meta.lock()->put(Symbols::get()["ContError"], contError);
    meta.lock()->put(Symbols::get()["ParseError"], parseError);
    global.lock()->put(Symbols::get()["SystemCallError"], systemCallError);
    global.lock()->put(Symbols::get()["SystemArgError"], systemArgError);
    global.lock()->put(Symbols::get()["StreamError"], streamError);
    global.lock()->put(Symbols::get()["TypeError"], typeError);
    global.lock()->put(Symbols::get()["SlotError"], slotError);
    global.lock()->put(Symbols::get()["ContError"], contError);
    global.lock()->put(Symbols::get()["ParseError"], parseError);
}

// TODO Take a lot of the ', global, global);' statements and make them scoped

ProtoError doSystemArgError(ObjectPtr global,
                                string name,
                                int expected,
                                int got) {
    ObjectPtr meta_ = meta(global);
    ObjectPtr err = clone(getInheritedSlot(meta_, Symbols::get()["SystemArgError"]));
    err.lock()->put(Symbols::get()["gotArguments"], garnish(global, got));
    err.lock()->put(Symbols::get()["expectedArguments"], garnish(global, expected));
    err.lock()->put(Symbols::get()["functionName"], garnish(global, name));
    return ProtoError(err);
}

ProtoError doSlotError(ObjectPtr global, ObjectPtr problem, string slotName) {
    ObjectPtr meta_ = meta(global);
    ObjectPtr err = clone(getInheritedSlot(meta_, Symbols::get()["SlotError"]));
    err.lock()->put(Symbols::get()["slotName"], garnish(global, slotName));
    err.lock()->put(Symbols::get()["objectInstance"], problem);
    return ProtoError(err);
}

ProtoError doParseError(ObjectPtr global) {
    ObjectPtr meta_ = meta(global);
    ObjectPtr err = clone(getInheritedSlot(meta_, Symbols::get()["ParseError"]));
    return ProtoError(err);
}

ProtoError doParseError(ObjectPtr global, string message) {
    ObjectPtr meta_ = meta(global);
    ObjectPtr err = clone(getInheritedSlot(meta_, Symbols::get()["ParseError"]));
    err.lock()->put(Symbols::get()["message"], garnish(global, message));
    return ProtoError(err);
}

ProtoError doEtcError(ObjectPtr global, string errorName, string msg) {
    ObjectPtr meta_ = meta(global);
    ObjectPtr err = clone(getInheritedSlot(meta_, Symbols::get()[errorName]));
    err.lock()->put(Symbols::get()["message"], garnish(global, msg));
    return ProtoError(err);
}

