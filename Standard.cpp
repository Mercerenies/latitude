#include "Standard.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include <list>
#include <sstream>

using namespace std;

// TODO Failed lookups should return nil or err properly, not segfault

void spawnSystemCalls(ObjectPtr& global, ObjectPtr& systemCall, ObjectPtr& sys);

ObjectPtr spawnObjects() {

    ObjectPtr object(new Object());
    ObjectPtr meta(clone(object));
    ObjectPtr global(clone(object));
    ObjectPtr method(clone(object));
    ObjectPtr number(clone(object));
    ObjectPtr string(clone(object));
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
    object->put("parent", object);

    // Meta linkage
    meta->put("meta", meta);
    object->put("meta", meta);

    // Primitives (Method, double, string)
    method->put("closure", global);
    method->prim(Method());
    number->prim(0.0);
    string->prim("");

    // Global scope contains basic types
    global->put("Global", global);
    global->put("Object", global);
    global->put("Method", method);
    global->put("Number", number);
    global->put("String", string);
    global->put("Stream", stream);
    global->put("stdin", stdin_);
    global->put("stderr", stderr_);
    global->put("stdout", stdout_);
    global->put("SystemCall", systemCall);
    global->put("True", true_);
    global->put("False", false_);
    global->put("Nil", nil);
    global->put("Boolean", boolean);

    // Meta calls for basic types
    meta->put("Method", method);
    meta->put("Number", number);
    meta->put("String", string);
    meta->put("Stream", stream);
    meta->put("SystemCall", systemCall);
    meta->put("True", true_);
    meta->put("False", false_);
    meta->put("Nil", nil);
    meta->put("Boolean", boolean);
    meta->put("sys", sys);

    // Method and system call properties
    spawnSystemCalls(global, systemCall, sys);

    // Stream setup
    stdout_->prim(outStream());
    stdin_->prim(inStream());
    stderr_->prim(errStream());
    stream->put("in?", eval("{ meta sys streamIn?: self. }.", global, global));
    stream->put("out?", eval("{ meta sys streamOut?: self. }.", global, global));
    stream->put("puts", eval("{ meta sys streamPuts: self, $1. }.", global, global));
    stream->put("putln", eval("{ meta sys streamPutln: self, $1. }.", global, global));
    stream->put("print", eval("{ self puts: $1 toString. }.", global, global));
    stream->put("println", eval("{ self putln: $1 toString. }.", global, global));
    stream->put("dump", eval("{ meta sys streamDump: lexical, dynamic, self, $1. }.",
                             global, global));

    // Self-reference in scopes, etc.
    global->put("scope", eval("{ self. }.", global, global));
    global->put("$scope", eval("{ self. }.", global, global));
    global->put("lexical", eval("{ self. }.", global, global));
    global->put("dynamic", eval("{ $scope parent. }.", global, global));
    object->put("me", eval("{ self. }.", global, global));

    // More method setup (now that we have system calls)
    method->put("call", eval("{ self. }.", global, global));

    // Boolean casts and operations
    object->put("toBool", true_);
    false_->put("toBool", false_);
    nil->put("toBool", false_);
    global->put("if", eval(R"({
                               meta sys ifThenElse: dynamic,
                                                    $1 toBool,
                                                    { parent dynamic $2. },
                                                    { parent dynamic $3. }.
                             }.)",
                           global, global));
    /////
    object->put("ifTrue", eval(R"({ if: self,
                                        { parent dynamic $1. },
                                        { meta Nil. }.
                                  }.)", global, global));
    object->put("ifFalse", eval(R"({ if: self,
                                        { meta Nil. },
                                        { parent dynamic $1. }.
                                  }.)", global, global));

    // Ordinary objects print very simply
    object->put("toString", eval("\"Object\".", global, global));

    // Basic data types print appropriately
    method->put("toString", eval("\"Method\".", global, global));
    stream->put("toString", eval("\"Stream\".", global, global));
    string->put("toString", eval("{ meta sys primToString: self. }.", global, global));
    number->put("toString", eval("{ meta sys primToString: self. }.", global, global));
    boolean->put("toString", eval("\"Boolean\".", global, global));
    true_->put("toString", eval("\"True\".", global, global));
    false_->put("toString", eval("\"False\".", global, global));
    nil->put("toString", eval("\"Nil\".", global, global));

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

    systemCall->prim([&global](list<ObjectPtr> lst) {
            return eval("meta Nil.", global, global);
        });
    callStreamIn->prim([&global](list<ObjectPtr> lst) {
            ObjectPtr stream;
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
    callStreamOut->prim([&global](list<ObjectPtr> lst) {
            ObjectPtr stream;
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
    callStreamPuts->prim([&global](list<ObjectPtr> lst) {
            ObjectPtr stream, obj;
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
    callStreamPutln->prim([&global](list<ObjectPtr> lst) {
            ObjectPtr stream, obj;
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
    callPrimToString->prim([&global](list<ObjectPtr> lst) {
            ObjectPtr obj;
            if (bindArguments(lst, obj)) {
                string result = primToString(obj);
                return garnish(global, result);
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callIfStatement->prim([&global](list<ObjectPtr> lst) {
            ObjectPtr dyn, cond, tr, fl;
            if (bindArguments(lst, dyn, cond, tr, fl)) {
                auto result = eval("meta Nil.", global, global);
                // These are unused except to verify that the prim() is correct
                auto tr_ = boost::get<Method>(&tr->prim());
                auto fl_ = boost::get<Method>(&fl->prim());
                if ((tr_ == NULL) || (fl_ == NULL))
                    return eval("meta Nil.", global, global); // TODO Throw error
                //
                auto definitelyTrue = eval("meta True.", global, global);
                if (cond == definitelyTrue)
                    return callMethod(result, nullptr, tr, clone(dyn));
                else
                    return callMethod(result, nullptr, fl, clone(dyn));
            } else {
                return eval("meta Nil.", global, global); // TODO Throw error
            }
        });
    callStreamDump->prim([&global](list<ObjectPtr> lst) {
            ObjectPtr lex, dyn, stream, obj;
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

    sys->put("streamIn?", callStreamIn);
    sys->put("streamOut?", callStreamOut);
    sys->put("streamPuts", callStreamPuts);
    sys->put("streamPutln", callStreamPutln);
    sys->put("primToString", callPrimToString);
    sys->put("ifThenElse", callIfStatement);
    sys->put("streamDump", callStreamDump);

}

// TODO Take a lot of the ', global, global);' statements and make them scoped
// TODO Reset the line number when we parse a second time (should be easy)
