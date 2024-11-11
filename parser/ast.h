#pragma once

#include <memory>
#include <variant>
#include <iostream>
#include <cassert>
#include <vector>

namespace ast {

enum Operator {
    Plus,
    Minus,
    Times,
    Divide,
    LT,
    GT,
    EqEq
};


class Node {
public:
    enum NodeKind { NodeList, NodeReturn, NodeIdent, NodeInteger, NodeFloating, NodeInfix,
        NodePrefix, NodeCall, NodeFnDef, NodeIf, NodeFor, NodeLet, NodeSet };

    NodeKind getKind() const { return kind; }
    Node(NodeKind kind) : kind(kind) {}

private:
    NodeKind kind;
};


// a class to represent a list of nodes such as a comma-separated list of expressions.
template <typename T>
class List : public Node {
public:
    List() : Node(NodeList) {}
    List(T* item) : Node(NodeList) { list.push_back(item); }

    void cons(T *item) { list.insert(list.begin(), item); }
    size_t size() { return list.size(); }

    static bool classof(const Node *node) { return node->getKind() == NodeList; }

    std::vector<T*> list;
};


struct Return : public Node {
    Return(Node* expr) : Node(NodeReturn), expr(expr) {}
    static bool classof(const Node *node) { return node->getKind() == NodeReturn; }
    Node* expr;
};


struct Ident : public Node {
    Ident(const std::string &ident) : Node(NodeIdent), ident(ident) {}
    static bool classof(const Node *node) { return node->getKind() == NodeIdent; }
    std::string ident;
};


struct Integer : public Node {
    Integer(int integer) : Node(NodeInteger), integer(integer) {}
    int integer;
    static bool classof(const Node *node) { return node->getKind() == NodeInteger; }
};

struct Floating : public Node {
    Floating(double floating) : Node(NodeFloating), floating(floating) {}
    double floating;
    static bool classof(const Node *node) { return node->getKind() == NodeFloating; }
};

struct Infix : public Node {
    Node *left, *right;
    Operator op;

    Infix(Node *left, Operator op, Node *right) :
        Node(NodeInfix),
        left(left),
        right(right),
        op(op)
    {}
    static bool classof(const Node *node) { return node->getKind() == NodeInfix; }
};

struct Prefix : public Node {
    Node *right;
    Operator op;

    Prefix(Operator op, Node* right) : Node(NodePrefix), right(right), op(op) {}
    static bool classof(const Node *node) { return node->getKind() == NodePrefix; }
};

struct Call : public Node {
    Call(std::string& name, List<Node> *args)
        : Node(NodeCall), name(name), args(args) {}
    static bool classof(const Node *node) { return node->getKind() == NodeCall; }

    std::string      name; 
    List<Node> *args;
};

struct FnDef : public Node {
    FnDef(Ident * name,
          List<Ident> *args,
          List<Node> *body)
            : Node(NodeFnDef), name(name), args(args), body(body) {}

    Ident *name;
    List<Ident> *args;
    List<Node> *body;

    static bool classof(const Node *node) { return node->getKind() == NodeFnDef; }
};


struct If : public Node {
    If(Node *cnd, List<Node> *trueBody, List<Node> *falseBody)
        : Node(NodeIf), cnd(cnd), trueBody(trueBody), falseBody(falseBody) {}

    Node *cnd;
    List<Node> *trueBody;
    List<Node> *falseBody;

    static bool classof(const Node *node) { return node->getKind() == NodeIf; }
};


struct Let : public Node {
    Let(std::string &name, Node *expr) : Node(NodeLet), name(name), expr(expr) {}

    std::string name;
    Node *expr;

    static bool classof(const Node *node) { return node->getKind() == NodeLet; }
};

struct Set : public Node {
    Set(std::string &name, Node *expr) : Node(NodeSet), name(name), expr(expr) {}

    std::string name;
    Node *expr;

    static bool classof(const Node *node) { return node->getKind() == NodeSet; }
};


struct For : public Node {
    For(Node *cnd, List<Node> *body)
        : Node(NodeFor), cnd(cnd), body(body) {}

    Node *cnd;
    List<Node> *body;

    static bool classof(const Node *node) { return node->getKind() == NodeFor; }
};
}

