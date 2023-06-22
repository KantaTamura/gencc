#include "gencc.h"

VarList *locals;

// 新しいノードを作成して，kindを設定する．
Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

// 2つの葉を持つノードを生成する．
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

// 1つの葉を持つノードを生成する．
Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = expr;
    return node;
}

// 数値を持つ葉を生成する．
Node *new_num(long val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

// 変数を持つ葉を生成する．
Node *new_var(Var *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

// ローカル変数を名前で検索する．
Var *find_var(Token *tok) {
    for (VarList *vl = locals; vl; vl = vl->next) {
        Var *var = vl->var;
        if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len)) {
            return var;
        }
    }
    return NULL;
}

// ローカル変数を追加する．
Var *push_var(char *name, Type *ty) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = ty;

    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = var;
    vl->next = locals;
    locals = vl;
    return var;
}

Type *basetype();
Type *read_type_suffix(Type *base);
VarList *read_func_param();
VarList *read_func_params();
Function *function();
Node *declaration();
Node *stmt();
Node *read_expr_stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *postfix();
Node *func_args();
Node *primary();

// program = function*
Function *program() {
    Function head;
    head.next = NULL;
    Function *cur = &head;

    while (!at_eof()) {
        cur->next = function();
        cur = cur->next;
    }

    return head.next;
}

// basetype = "int" "*"*
Type *basetype() {
    expect("int");
    Type *ty = int_type();
    while (consume("*"))
        ty = pointer_to(ty);
    return ty;
}

// suffix = ( "[" num "]" )*
Type *read_type_suffix(Type *base) {
    if (!consume("["))
        return base;

    long size = expect_number();
    expect("]");
    base = read_type_suffix(base);
    return array_of(base, size);
}

// param = basetype ident suffix?
VarList *read_func_param() {
    Type *ty = basetype();
    char *name = expect_ident();
    ty = read_type_suffix(ty);

    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = push_var(name, ty);
    return vl;
}

// params = "(" ( param ( "," param )* )? ")"
VarList *read_func_params() {
    if (consume(")"))
        return NULL;

    VarList *head = read_func_param();
    VarList *cur = head;

    while (!consume(")")) {
        expect(",");
        cur->next = read_func_param();
        cur = cur->next;
    }

    return head;
}

// function = basetype ident "(" params? ")" "{" stmt* "}"
Function *function() {
    locals = NULL;

    Function *fn = calloc(1, sizeof(Function));
    basetype();
    fn->name = expect_ident();
    expect("(");
    fn->params = read_func_params();
    expect("{");

    Node head;
    head.next = NULL;
    Node *cur = &head;

    while (!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    fn->node = head.next;
    fn->locals = locals;
    return fn;
}

// declaration = basetype ident suffix? ("=" expr)? ";"
Node *declaration() {
    Token *tok = token;
    Type *ty = basetype();
    char *name = expect_ident();
    ty = read_type_suffix(ty);
    Var *var = push_var(name, ty);

    if (consume(";"))
        return new_node(ND_NULL, tok);

    expect("=");
    Node *lhs = new_var(var, tok);
    Node *rhs = expr();
    expect(";");
    Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
    return new_unary(ND_EXPR_STMT, node, tok);
}

// stmt = expr ";"
//      | declaration
//      | "{" stmt* "}"
//      | "if" "(" expr ")" stmt ( "else" stmt )?
//      | "while" "(" expr ")" stmt
//      | "for" "( expr? ";" expr? ";" expr? ")" stmt
//      | "return" expr ";"
Node *stmt() {
    Token *tok;
    if ((tok = consume("{"))) {
        Node *node = new_node(ND_BLOCK, tok);

        Node head;
        head.next = NULL;
        Node *cur = &head;
        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }

        node->body = head.next;
        return node;
    }

    if ((tok = consume("return"))) {
        Node *node = new_unary(ND_RETURN, expr(), tok);
        expect(";");
        return node;
    }

    if ((tok = consume("if"))) {
        Node *node = new_node(ND_IF, tok);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume("else")) {
            node->els = stmt();
        }
        return node;
    }

    if ((tok =consume("while"))) {
        Node *node = new_node(ND_WHILE, tok);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        return node;
    }

    if ((tok = consume("for"))) {
        Node *node = new_node(ND_FOR, tok);
        expect("(");
        if (!consume(";")) {
            node->init = read_expr_stmt();
            expect(";");
        }
        if (!consume(";")) {
            node->cond = expr();
            expect(";");
        }
        if (!consume(")")) {
            node->inc = read_expr_stmt();
            expect(")");
        }
        node->then = stmt();
        return node;
    }

    if ((tok = peek("int")))
        return declaration();

    Node *node = read_expr_stmt();
    expect(";");
    return node;
}

// スタックにゴミを残さないように追加
Node *read_expr_stmt() {
    Token *tok = token;
    return new_unary(ND_EXPR_STMT, expr(), tok);
}

// expr = assign
Node *expr() {
    return assign();
}

// assign = equality ("=" assign)?
Node *assign() {
    Node *node = equality();
    Token *tok;
    if ((tok = consume("="))) {
        node = new_binary(ND_ASSIGN, node, assign(), tok);
    }
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
    Node *node = relational();
    Token *tok;

    for (;;) {
        if ((tok = consume("=="))) {
            node = new_binary(ND_EQ, node, relational(), tok);
        } else if ((tok = consume("!="))) {
            node = new_binary(ND_NE, node, relational(), tok);
        } else {
            return node;
        }
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
    Node *node = add();
    Token *tok;

    for (;;) {
        if ((tok = consume("<"))) {
            node = new_binary(ND_LT, node, add(), tok);
        } else if ((tok = consume("<="))) {
            node = new_binary(ND_LE, node, add(), tok);
        } else if ((tok = consume(">"))) {
            node = new_binary(ND_LT, add(), node, tok);
        } else if ((tok = consume(">="))) {
            node = new_binary(ND_LE, add(), node, tok);
        } else {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
    Node *node = mul();
    Token *tok;

    for (;;) {
        if ((tok = consume("+"))) {
            node = new_binary(ND_ADD, node, mul(), tok);
        } else if ((tok = consume("-"))) {
            node = new_binary(ND_SUB, node, mul(), tok);
        } else {
            return node;
        }
    }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
    Node *node = unary();
    Token *tok;

    for (;;) {
        if ((tok = consume("*"))) {
            node = new_binary(ND_MUL, node, unary(), tok);
        } else if ((tok = consume("/"))) {
            node = new_binary(ND_DIV, node, unary(), tok);
        } else {
            return node;
        }
    }
}

// unary = ("+" | "-")? unary
//       | ("*" | "&") unary
//       | postfix
Node *unary() {
    Token *tok;
    if (consume("+")) {
        return unary();
    }

    if ((tok = consume("-"))) {
        return new_binary(ND_SUB, new_num(0, tok), unary(), tok);
    }

    if ((tok = consume("*"))) {
        return new_unary(ND_DEREF, unary(), tok);
    }

    if ((tok = consume("&"))) {
        return new_unary(ND_ADDR, unary(), tok);
    }

    return postfix();
}

// postfix = primary ("[" expr "]")*
Node *postfix() {
    Node *node = primary();
    Token *tok;

    while ((tok = consume("["))) {
        // x[y] => *(x+y)
        Node *exp = new_binary(ND_ADD, node, expr(), tok);
        expect("]");
        node = new_unary(ND_DEREF, exp, tok);
    }
    return node;
}

// func_args = "(" ( assign ( "," assign )* )? ")"
Node *func_args() {
    if (consume(")"))
        return NULL;

    Node *head = assign();
    Node *cur = head;
    while (consume(",")) {
        cur->next = assign();
        cur = cur->next;
    }

    expect(")");
    return head;
}

// primary = num
//         | ident func_args?
//         | "(" expr ")"
//         | "sizeof" unary
Node *primary() {
    // 次のトークンが"("なら，"(" expr ")"のはず
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok;
    if ((tok = consume_ident())) {
        if (consume("(")) {
            Node *node = new_node(ND_FUNCALL, tok);
            node->funcname = strndup(tok->str, tok->len);
            node->args = func_args();
            return node;
        }

        Var *var = find_var(tok);
        if (!var) {
            error_tok(tok, "undefined variable");
        }
        return new_var(var, tok);
    }

    if ((tok = consume("sizeof"))) {
        return new_unary(ND_SIZEOF, unary(), tok);
    }

    tok = token;
    if (tok->kind != TK_NUM)
        error_tok(tok, "expected expression");
    // そうでなければ数値のはず
    return new_num(expect_number(), tok);
}
