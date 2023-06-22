#include "gencc.h"

char *filename;
char *user_input;
Token *token;

// エラーを報告する
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラー箇所を報告する
void verror_at(char *loc, char *fmt, va_list ap) {
    // find a line containing 'loc'
    char *line = loc;
    while (user_input < line && line[-1] != '\n')
        line--;

    char *end = loc;
    while (*end != '\n')
        end++;

    // get a line number
    int line_num = 1;
    for (char *p = user_input; p < line; p++)
        if (*p == '\n')
            line_num++;

    // print out the line
    int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    // show the error message
    long pos = loc - line + indent;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", (int)pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (tok)
        verror_at(tok->str, fmt, ap);

    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 次のトークンが期待している記号のときは，そのトークンを返す
Token *peek(char *s) {
    if (token->kind != TK_RESERVED || strlen(s) != token->len || memcmp(token->str, s, token->len) != 0)
        return NULL;
    return token;
}

// 次のトークンが期待している記号のときは，トークンを1つ読み進めて真を返す．
Token *consume(char *s) {
    if (!peek(s))
        return NULL;
    Token *t = token;
    token = token->next;
    return t;
}

// 次のトークンが識別子の場合，トークンを1つ読み進めてそのトークンを返す．
Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return NULL;
    }

    Token *t = token;
    token = token->next;
    return t;
}

// 次のトークンが期待している記号のときは，トークンを1つ読み進める．
// それ以外の場合にはエラーを報告する．
void expect(char *s) {
    if (!peek(s))
        error_tok(token, "expected \"%s\"", s);

    token = token->next;
}

// 次のトークンが数値の場合，トークンを1つ読み進めてその数値を返す．
// それ以外の場合にはエラーを報告する．
long expect_number() {
    if (token->kind != TK_NUM) {
        error_tok(token, "not a number");
    }

    long val = token->val;
    token = token->next;
    return val;
}

char *expect_ident() {
    if (token->kind != TK_IDENT)
        error_tok(token, "expected an identifier");

    char *s = strndup(token->str, token->len);
    token = token->next;
    return s;
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

char *startswith_keyword(char *p) {
    // keyword
    static char *kw[] = { "return", "if", "else", "while", "for", "int",
                          "sizeof", "char" };

    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        size_t len = strlen(kw[i]);
        if (startswith(p, kw[i]) && !isalnum(p[len]))
            return kw[i];
    }

    // multi-letter puncture
    static char *ops[] = { "==", "!=", "<=", ">="};

    for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++) {
        if (startswith(p, ops[i]))
            return ops[i];
    }

    return NULL;
}

// 指定した長さの文字列をコピーする
char *strndup(char *p, long len) {
    char *buf = malloc(len + 1);
    strncpy(buf, p, len);
    buf[len] = '\0';
    return buf;
}

// [a-z] or [A-Z] or '_' ならtrueを返す
bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// [a-z] or [A-Z] or [0-9] or '_' ならtrueを返す
bool is_alnum(char c) {
    return is_alpha(c) || ('0' <= c && c <= '9');
}

char get_escape_char(char c) {
    switch(c) {
        case 'a': return '\a';
        case 'b': return '\b';
        case 't': return '\t';
        case 'n': return '\n';
        case 'v': return '\v';
        case 'f': return '\f';
        case 'r': return '\r';
        case 'e': return 27;
        case '0': return 0;
        default: return c;
    }
}

Token *read_string_literal(Token *cur, char *start) {
    char *p = start + 1;
    char buf[1024];
    long len = 0;

    for (;;) {
        if (len == sizeof(buf))
            error_at(start, "string literal too large");
        if (*p == '\0')
            error_at(start, "unclosed string literal");
        if (*p == '"')
            break;

        if (*p == '\\') {
            p++;
            buf[len++] = get_escape_char(*p++);
        } else {
            buf[len++] = *p++;
        }
    }

    Token *tok = new_token(TK_STR, cur, start, p - start + 1);
    tok->contents = malloc(len + 1);
    memcpy(tok->contents, buf, len);
    tok->contents[len] = '\0';
    tok->cont_len = len + 1;
    return tok;
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

        char *kw = startswith_keyword(p);
        if (kw) {
            size_t len = strlen(kw);
            cur = new_token(TK_RESERVED, cur, p, (long)len);
            p += len;
            continue;
        }

        if (strchr("+-*/()<>;={},&[]", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // 文字列リテラル
        if (*p == '"') {
            cur = read_string_literal(cur, p);
            p += cur->len;
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        if (is_alpha(*p)) {
            char *q = p++;
            while (is_alnum(*p)) {
                p++;
            }
            cur = new_token(TK_IDENT, cur, q, p - q);
            continue;
        }

        error_at(p, "invalid token");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}
