#include "gencc.h"

char *user_input;
Token *token;

// エラー箇所を報告する
void error_at(const char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    long pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", (int)pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 次のトークンが期待している記号のときは，トークンを1つ読み進めて真を返す．
// それ以外の場合には偽を返す．
bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len) != 0) {
        return false;
    }

    token = token->next;
    return true;
}

// 次のトークンが期待している記号のときは，トークンを1つ読み進める．
// それ以外の場合にはエラーを報告する．
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len) != 0) {
        error_at(token->str, "expect '%c'", op);
    }

    token = token->next;
}

// 次のトークンが数値の場合，トークンを1つ読み進めてその数値を返す．
// それ以外の場合にはエラーを報告する．
long expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str, "not a number");
    }

    long val = token->val;
    token = token->next;
    return val;
}

// 次のトークンがEOFの場合，真を返す．
bool at_eof() {
    return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる．
Token *new_token(TokenKind kind, Token *cur, char *str, long len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// 入力文字列pが文字列qで始まるかどうかを返す．
bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

// "user_input"をトークナイズしてそれを返す．
Token *tokenize() {
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if (strchr("+-*/()<>;", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        error_at(p, "invalid token");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}
