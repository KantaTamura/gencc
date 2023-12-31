#include<assert.h>
#include<ctype.h>
#include<errno.h>
#include<stdarg.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef struct Token Token;
typedef struct Var Var;
typedef struct VarList VarList;
typedef struct Node Node;
typedef struct Function Function;
typedef struct Program Program;
typedef struct Type Type;
typedef struct Member Member;

//
// tokenize.c
//

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_STR,      // 文字列リテラル
    TK_NUM,      // 整数トークン
    TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

// トークン型
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    long val;       // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    long len;       // トークンの長さ

    char *contents; // 文字列リテラルの内容
    long cont_len;  // 文字列リテラルの長さ
};


void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
char *strndup(char *p, long len);
Token *peek(char *op);
Token *consume(char *op);
Token *consume_ident();
void expect(char *op);
long expect_number();
char *expect_ident();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, long len);
Token *tokenize();

extern char *filename;
extern char *user_input;
extern Token *token;

//
// parse.c
//

// 変数の型
struct Var {
    char *name;     // 変数の名前
    Type *ty;       // 変数の型
    bool is_local;  // ローカル変数かどうか

    // ローカル変数の場合のみ
    long offset;    // RBPからのオフセット

    // グローバル変数の場合のみ
    char *contents; // 文字列リテラルの内容
    long cont_len;  // 文字列リテラルの長さ
};

// ローカル変数のリスト
struct VarList {
    VarList *next;  // 次の変数
    Var *var;       // 変数
};

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD,         // +
    ND_SUB,         // -
    ND_MUL,         // *
    ND_DIV,         // /
    ND_EQ,          // ==
    ND_NE,          // !=
    ND_LT,          // <
    ND_LE,          // <=
    ND_ASSIGN,      // =
    ND_MEMBER,      // . (構造体のメンバへのアクセス)
    ND_ADDR,        // unary &
    ND_DEREF,       // unary *
    ND_IF,          // "if"
    ND_WHILE,       // "while"
    ND_FOR,         // "for"
    ND_SIZEOF,      // "sizeof"
    ND_RETURN,      // "return"
    ND_BLOCK,       // "{" ... "}"
    ND_FUNCALL,     // 関数呼び出し
    ND_EXPR_STMT,   // 最後に必要ない値をpushする式
    ND_STMT_EXPR,   // 文の中にある式
    ND_VAR,         // ローカル変数
    ND_NUM,         // 整数
    ND_NULL,        // 空文
} NodeKind;

// 抽象構文木のノード型
struct Node {
    NodeKind kind; // ノードの型
    Node *next;    // 次のノード
    Type *ty;      // int or pointer to int
    Token *tok;    // トークン

    Node *lhs;     // 左辺
    Node *rhs;     // 右辺

    // "if" or "while"
    Node *cond;    // 条件
    Node *then;    // condが真なら実行
    Node *els;     // condが偽なら実行

    // "for"
    Node *init;    // 初期化
    Node *inc;     //

    // "block" or "stmt-expr"
    Node *body;

    // struct member access
    char *member_name;
    Member *member;

    // function call
    char *funcname;
    Node *args;

    long val;      // kindがND_NUMの場合のみ使う
    Var *var;      // kindがND_LVARの場合のみ使う
};

// 関数本体
struct Function {
    Function *next; // 次の関数
    char *name;     // 関数名
    VarList *params;// 関数の引数リスト

    Node *node;     // 関数の構文木
    VarList *locals;// ローカル変数のリスト
    long stack_size;// ローカル変数で使用するスタックサイズ
};

// プログラム本体
struct Program {
    VarList *global;    // グローバル変数
    Function *fns;      // 関数のリスト
};

Program *program();

//
// type.c
//

typedef enum {
    TY_CHAR,
    TY_INT,
    TY_PTR,
    TY_ARRAY,
    TY_STRUCT,
} TypeKind;

struct Type {
    TypeKind kind;
    Type *base;         // pointer or array
    long array_size;    // array
    Member *members;    // struct
};

struct Member {
    Member *next;
    Type *ty;
    char *name;
    long offset;
};

Type *char_type();
Type *int_type();
Type *pointer_to(Type *base);
Type *array_of(Type *base, long size);
long size_of(Type *ty);
void add_type(Program *prog);

//
// codegen.c
//

void codegen(Program *prog);
