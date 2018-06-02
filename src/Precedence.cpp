
#include <utility>
#include <list>
#include "Precedence.hpp"
#include "Parents.hpp"
#include "Reader.hpp"

// Simple binary tree data structure for intermediate results.
// Branches do not have data, so a node will either have a List* OR a
// left/right, not both.
struct OpTree {
    bool isLeaf;
    List* leaf;
    std::string name;
    OpTree* left;
    OpTree* right;
};

// An empty string signals the end of input
using op_pair_t = std::pair<List*, std::string>;

OperatorTable::OperatorTable(ObjectPtr table)
    : impl(table) {}

OperatorData OperatorTable::lookup(std::string op) const {
    Symbolic name = Symbols::get()[op];
    ObjectPtr result = objectGet(impl, name);
    // Doesn't exist? Fine, just use the default.
    if (result == nullptr)
        return { DEFAULT_PRECEDENCE, Associativity::LEFT };
    // We have the object; it had better be a small number in the range now.
    if (auto value = boost::get<Number>(&result->prim())) {
        if (value->hierarchyLevel() > 0)
            throw std::string("Invalid operator table at " + op);
        auto value1 = value->asSmallInt();
        if ((value1 < 0) || (value1 >= 256))
            throw std::string("Invalid operator table at " + op);
        OperatorData result;
        result.precedence = (int)value1;
        // Note: Everything is left associative at the moment. That
        // may (will?) change in the future.
        result.associativity = Associativity::LEFT;
        return result;
    } else {
        // Not a number
        throw std::string("Invalid operator table at " + op);
    }
}

std::list<op_pair_t> exprToSeq(Expr* expr) {
    std::list<op_pair_t> result;
    std::string op = "";
    while ((expr != nullptr) && (expr->isOperator)) {
        result.push_front({ expr->args, op });
        op = std::string(expr->name);
        Expr* temp = expr;
        expr = expr->lhs;
        temp->lhs = nullptr;
        temp->args = nullptr;
        cleanupE(temp);
    }
    if (expr == nullptr) {
        expr = makeExpr();
        expr->isDummy = true;
    }
    List* dummy = makeList();
    dummy->car = expr;
    dummy->cdr = makeList();
    dummy->cdr->car = nullptr;
    dummy->cdr->cdr = nullptr;
    result.push_front({ dummy, op });
    return result;
}

bool shouldPop(const std::stack< std::pair<std::string, OperatorData> >& ops,
               const std::pair<std::string, OperatorData>& curr) {
    if (ops.empty())
        return false;
    auto& top = ops.top();
    if (top.second.precedence > curr.second.precedence) {
        return true;
    } else if (top.second.precedence < curr.second.precedence) {
        return false;
    } else {
        if (top.second.associativity != curr.second.associativity) {
            std::ostringstream err;
            err << "Operators " << top.first << " and " << curr.first
                << " have contradictory associativity";
            throw err.str();
        } else {
            switch (top.second.associativity) {
            case Associativity::LEFT:
                return true;
            case Associativity::RIGHT:
                return false;
            case Associativity::NONE:
                {
                    std::ostringstream err;
                    err << "Operators " << top.first << " and " << curr.first
                        << " do not associate";
                    throw err.str();
                }
            default:
                assert(false); // Pleeeeeeeease don't run this line of code :)
            }
        }
    }
}

void doPop(std::stack< std::pair<std::string, OperatorData> >& ops,
           std::stack<OpTree*>& nodes) {
    assert(!ops.empty());
    std::string popped = ops.top().first;
    ops.pop();
    assert(!nodes.empty());
    OpTree* rhs = nodes.top();
    nodes.pop();
    assert(!nodes.empty());
    OpTree* lhs = nodes.top();
    nodes.pop();
    OpTree* node = new OpTree();
    node->name = popped;
    node->left = lhs;
    node->right = rhs;
    nodes.push(node);
}

OpTree* seqToTree(std::list<op_pair_t>& seq, const OperatorTable& table) {
    std::stack<OpTree*> nodes;
    std::stack< std::pair<std::string, OperatorData> > ops;
    for (auto& elem : seq) {
        OpTree* op = new OpTree();
        op->isLeaf = true;
        op->leaf = elem.first;
        nodes.push(op);
        if (elem.second != "") {
            std::pair<std::string, OperatorData> curr = { elem.second, table.lookup(elem.second) };
            while (shouldPop(ops, curr)) {
                doPop(ops, nodes);
            }
            ops.push(curr);
        }
    }
    while (!ops.empty()) {
        doPop(ops, nodes);
    }
    // By this point, we should have exactly one node: the expression node.
    assert(nodes.size() == 1);
    // Great! Then pop it.
    OpTree* result = nodes.top();
    nodes.pop();
    return result;
}

List* treeToList(OpTree* tree);
Expr* treeToExpr(OpTree* tree);

List* treeToList(OpTree* tree) {
    if (tree->isLeaf) {
        // Leaf node
        List* args = tree->leaf;
        delete tree;
        tree = nullptr;
        // So I think this could maybe be an assertion failure. I
        // don't think it should ever happen in the course of
        // Latitude execution, whereas the equivalent error in
        // treeToExpr could very well occur in normal use.
        if ((args->car) && (args->car->isDummy)) {
            throw std::string("Invalid implied argument on right-hand-side of operator");
        }
        return args;
    } else {
        // Branch
        Expr* expr = treeToExpr(tree);
        List* list = makeList();
        list->car = expr;
        list->cdr = makeList();
        list->cdr->car = nullptr;
        list->cdr->cdr = nullptr;
        return list;
    }
}

Expr* treeToExpr(OpTree* tree) {
    if (tree->isLeaf) {
        // Leaf node
        List* args = tree->leaf;
        delete tree;
        tree = nullptr;
        // Must be of length 1
        if ((!args->car) && (!args->cdr))
            throw std::string("Invalid argument list on left-hand-side of operator");
        if ((args->cdr->car) || (args->cdr->cdr))
            throw std::string("Invalid argument list on left-hand-side of operator");
        Expr* result = args->car;
        args->car = nullptr;
        cleanupL(args);
        if (result->isDummy) {
            cleanupE(result);
            result = nullptr;
        }
        return result;
    } else {
        // Branch
        Expr* expr = makeExpr();
        expr->lhs = treeToExpr(tree->left);
        expr->args = treeToList(tree->right);
        expr->name = (char*)malloc(tree->name.length() + 1);
        strcpy(expr->name, tree->name.c_str());
        delete tree;
        return expr;
    }
}

Expr* reorganizePrecedence(const OperatorTable& table, Expr* expr) {
    auto seq = exprToSeq(expr);
    auto tree = seqToTree(seq, table);
    auto result = treeToExpr(tree);
    return result;
}

OperatorTable getTable(ObjectPtr lex) {

    ObjectPtr meta = objectGet(lex, Symbols::get()["meta"]);
    if (meta == nullptr)
        throw std::string("Could not find operator table");
    ObjectPtr table = objectGet(meta, Symbols::get()["operators"]);
    if (table == nullptr)
        throw std::string("Could not find operator table");
    ObjectPtr impl = objectGet(table, Symbols::get()["&impl"]);
    if (impl == nullptr)
        throw std::string("Could not find operator table");
    return OperatorTable(impl);
}
