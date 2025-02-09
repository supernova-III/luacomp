#pragma once
#include <cstddef>
#include "tokenizer.hh"

struct PrefixExpression;

struct Expression {
  enum Type {
    TYPE_SIMPLE,
    TYPE_UNARY,
    TYPE_BINARY,
    TYPE_PREFIX
  } type;

  union {
    Token simple;

    struct {
      Token op;
      Expression* expression;
    } unary_operation;

    struct {
      Expression* left;
      Expression* right;
      Token op;
    } binary_operation;

    PrefixExpression* prefix_expression;
  } value;
};

struct Variable {
  enum Type {
    // name
    TYPE_NAME,
    // prefix_expr '[' expr ']'
    TYPE_ARRAY_SUBSCRIPT,
    // prefix_expr.'name'
    TYPE_OBJECT_SUBSCRIPT
  } type;
  union {
    Token name;

    struct {
      PrefixExpression* prefix_expression;
      Expression expression;
    } subscript;

    struct {
      PrefixExpression* prefix_expression;
      Token name;
    } object_subscript;
  } value;
};

struct PrefixExpression {
  enum Type {
    TYPE_VARIABLE,
    // '(' expr ')'
    TYPE_EXPRESSION
  } type;

  union {
    Variable variable;
    Expression parenthesized_expression;
  } value;
};

struct ExpressionList {
  Expression* expressions;
  size_t size;
};

struct NameList {
  Token* names;
  size_t size;
};

struct VariableList {
  Variable* variables;
  size_t size;
};

struct ReturnStatement {
  // return [ exprlist ] [ ';' ]
  ExpressionList expressions;
};

struct Statement;

struct Block {
  Statement* statement;
  ReturnStatement return_statement;
};

struct Statement {
  enum Type {
    TYPE_ASSIGNMENT,
    TYPE_DO_BLOCK,
    TYPE_WHILE_LOOP,
    TYPE_REPEAT_UNTIL_LOOP,
    TYPE_IF,
    TYPE_FOR_LOOP,
    TYPE_FOR_RANGE_LOOP
  } type;

  union {
  } label;
};