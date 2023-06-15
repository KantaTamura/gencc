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

void error_at(const char *loc, char *fmt, ...);
bool consume(char *op);
void expect(char *op);
long expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, long len);
Token *tokenize();

extern char *user_input;
extern Token *token;

//
// parse.c
//

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_NUM, // 整数
} NodeKind;

// 抽象構文木のノード型
typedef struct Node Node;
struct Node {
    NodeKind kind; // ノードの型
    Node *next;    // 次のノード
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    long val;      // kindがND_NUMの場合のみ使う
};

Node *program();

//
// codegen.c
//

void codegen(Node *node);
