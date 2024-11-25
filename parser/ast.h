#pragma once

#include <memory>
#include <variant>
#include <iostream>
#include <cassert>
#include <vector>

#include "lexer.h"

namespace ast {

enum Operator { Plus, Minus, Times, Divide, LT, GT, EqEq };


class Node {
public:
    enum NodeKind { NodeList, NodeReturn, NodeIdent, NodeInteger, NodeFloating, NodeInfix,
        NodePrefix, NodeCall, NodeFnDef, NodeIf, NodeFor, NodeLet, NodeSet, NodeProgram };

    Node(NodeKind kind, TextPos pos) : kind(kind), pos(pos) {}

    NodeKind getKind() const { return kind; }
    TextPos getPos() const { return pos; }

private:
    NodeKind kind;
    TextPos pos;
};


// a class to represent a list of nodes such as a comma-separated list of expressions.
template <typename T>
class List : public Node {
public:
    List(TextPos pos) : Node(NodeList, pos) {}
    List(TextPos pos, T* item) : Node(NodeList, pos) { list.push_back(item); }

    void cons(T *item) { list.insert(list.begin(), item); }
    size_t size() { return list.size(); }

    static bool classof(const Node *node) { return node->getKind() == NodeList; }

    std::vector<T*> list;
};


struct Return : public Node {
    Return(TextPos pos, Node* expr) : Node(NodeReturn, pos), expr(expr) {}
    static bool classof(const Node *node) { return node->getKind() == NodeReturn; }
    Node* expr;
};


struct Ident : public Node {
    Ident(TextPos pos, const std::string &ident) : Node(NodeIdent, pos), ident(ident) {}
    static bool classof(const Node *node) { return node->getKind() == NodeIdent; }
    std::string ident;
};


struct Integer : public Node {
    Integer(TextPos pos, int integer) : Node(NodeInteger, pos), integer(integer) {}
    int integer;
    static bool classof(const Node *node) { return node->getKind() == NodeInteger; }
};

struct Floating : public Node {
    Floating(TextPos pos, double floating) : Node(NodeFloating, pos), floating(floating) {}
    double floating;
    static bool classof(const Node *node) { return node->getKind() == NodeFloating; }
};

struct Infix : public Node {
    Node *left, *right;
    Operator op;

    Infix(TextPos pos, Node *left, Operator op, Node *right) :
        Node(NodeInfix, pos),
        left(left),
        right(right),
        op(op)
    {}
    static bool classof(const Node *node) { return node->getKind() == NodeInfix; }
};

struct Prefix : public Node {
    Node *right;
    Operator op;

    Prefix(TextPos pos, Operator op, Node* right) : Node(NodePrefix, pos), right(right), op(op) {}
    static bool classof(const Node *node) { return node->getKind() == NodePrefix; }
};

struct Call : public Node {
    Call(TextPos pos, std::string& name, List<Node> *args)
        : Node(NodeCall, pos), name(name), args(args) {}
    static bool classof(const Node *node) { return node->getKind() == NodeCall; }

    std::string      name; 
    List<Node> *args;
};

struct FnDef : public Node {
    FnDef(TextPos pos, Ident * name,
          List<Ident> *args,
          List<Node> *body)
            : Node(NodeFnDef, pos), name(name), args(args), body(body) {}

    Ident *name;
    List<Ident> *args;
    List<Node> *body;

    static bool classof(const Node *node) { return node->getKind() == NodeFnDef; }
};


struct If : public Node {
    If(TextPos pos, Node *cnd, List<Node> *trueBody, List<Node> *falseBody)
        : Node(NodeIf, pos), cnd(cnd), trueBody(trueBody), falseBody(falseBody) {}

    Node *cnd;
    List<Node> *trueBody;
    List<Node> *falseBody;

    static bool classof(const Node *node) { return node->getKind() == NodeIf; }
};


struct Let : public Node {
    Let(TextPos pos, std::string &name, Node *expr) : Node(NodeLet, pos), name(name), expr(expr) {}

    std::string name;
    Node *expr;

    static bool classof(const Node *node) { return node->getKind() == NodeLet; }
};

struct Set : public Node {
    Set(TextPos pos, std::string &name, Node *expr) : Node(NodeSet, pos), name(name), expr(expr) {}

    std::string name;
    Node *expr;

    static bool classof(const Node *node) { return node->getKind() == NodeSet; }
};


struct For : public Node {
    For(TextPos pos, Node *cnd, List<Node> *body)
        : Node(NodeFor, pos), cnd(cnd), body(body) {}

    Node *cnd;
    List<Node> *body;

    static bool classof(const Node *node) { return node->getKind() == NodeFor; }
};

struct Program : public Node {
    Program(TextPos pos, List<Node> *stmts) : Node(NodeProgram, pos), stmts(stmts) {}

    List<Node> *stmts;

    static bool classof(const Node *node) { return node->getKind() == NodeProgram; }
};
}

