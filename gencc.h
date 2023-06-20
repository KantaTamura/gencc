#include<ctype.h>
#include<stdarg.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

//
// tokenize.c
//

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数トークン
    TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

// トークン型
typedef struct Token Token;
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    long val;       // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    long len;        // トークンの長さ
};


void error(char *fmt, ...);
void error_at(const char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
char *strndup(char *p, long len);
Token *consume(char *op);
Token *consume_ident();
void expect(char *op);
long expect_number();
char *expect_ident();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, long len);
Token *tokenize();

extern char *user_input;
extern Token *token;

//
// parse.c
//

// ローカル変数の型
typedef struct Var Var;
struct Var {
    char *name; // 変数の名前
    int offset; // RBPからのオフセット
};

// ローカル変数のリスト
typedef struct VarList VarList;
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
    ND_IF,          // "if"
    ND_WHILE,       // "while"
    ND_FOR,         // "for"
    ND_RETURN,      // "return"
    ND_BLOCK,       // "{" ... "}"
    ND_FUNCALL,     // 関数呼び出し
    ND_EXPR_STMT,   // 最後に必要ない値をpushする式
    ND_VAR,         // ローカル変数
    ND_NUM,         // 整数
} NodeKind;

// 抽象構文木のノード型
typedef struct Node Node;
struct Node {
    NodeKind kind; // ノードの型
    Node *next;    // 次のノード
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

    // "block"
    Node *body;

    // function call
    char *funcname;
    Node *args;

    long val;      // kindがND_NUMの場合のみ使う
    Var *var;      // kindがND_LVARの場合のみ使う
};

// プログラム本体を管理する型
typedef struct Function Function;
struct Function {
    Function *next; // 次の関数
    char *name;     // 関数名
    VarList *params;// 関数の引数リスト

    Node *node;     // 関数の構文木
    VarList *locals;// ローカル変数のリスト
    int stack_size; // ローカル変数で使用するスタックサイズ
};

Function *program();

//
// codegen.c
//

void codegen(Function *prog);
