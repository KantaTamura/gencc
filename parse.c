#include "gencc.h"

VarList *scope;
VarList *locals;
VarList *globals;

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

// 変数を名前で検索する．
Var *find_var(Token *tok) {
    for (VarList *vl = scope; vl; vl = vl->next) {
        Var *var = vl->var;
        if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len)) {
            return var;
        }
    }

    return NULL;
}

// ローカル変数を追加する．
Var *push_var(char *name, Type *ty, bool is_local) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = ty;
    var->is_local = is_local;

    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = var;

    if (is_local) {
        vl->next = locals;
        locals = vl;
    } else {
        vl->next = globals;
        globals = vl;
    }

    VarList *sc = calloc(1, sizeof(VarList));
    sc->var = var;
    sc->next = scope;
    scope = sc;

    return var;
}

char *new_label() {
    static int cnt = 0;
    char buf[20];
    sprintf(buf, ".L.data.%d", cnt++);
    return strndup(buf, 20);
}

bool is_function();
Type *basetype();
Type *struct_decl();
Member *struct_member();
void global_var();
Type *read_type_suffix(Type *base);
VarList *read_func_param();
VarList *read_func_params();
Function *function();
Node *declaration();
bool is_typename();
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

// program = (global-var | function)*
Program *program() {
    Function head;
    head.next = NULL;
    Function *cur = &head;
    globals = NULL;

    while (!at_eof()) {
        if (is_function()) {
            cur->next = function();
            cur = cur->next;
        } else {
            global_var();
        }
    }

    Program *prog = calloc(1, sizeof(Program));
    prog->global = globals;
    prog->fns = head.next;
    return prog;
}

bool is_function() {
    Token *tok = token;
    basetype();
    bool isfunc = consume_ident() && consume("(");
    token = tok;
    return isfunc;
}

// global-var = basetype ident suffix ";"
void global_var() {
    Type *ty = basetype();
    char *name = expect_ident();
    ty = read_type_suffix(ty);
    expect(";");
    push_var(name, ty, false);
}

// basetype = ("int" | "char" | struct-dicl) "*"*
Type *basetype() {
    if (!is_typename())
        error_tok(token, "typename expected");

    Type *ty;
    if (consume("char"))
        ty = char_type();
    else if (consume("int"))
        ty = int_type();
    else
        ty = struct_decl();

    while (consume("*"))
        ty = pointer_to(ty);
    return ty;
}

// struct-decl = "struct" ident? "{" struct-member "}"
Type *struct_decl() {
    // Read struct members
    expect("struct");
    expect("{");

    Member head;
    head.next = NULL;
    Member *cur = &head;

    while (!consume("}")) {
        cur->next = struct_member();
        cur = cur->next;
    }

    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_STRUCT;
    ty->members = head.next;

    // Assign offsets within the struct to members.
    int offset = 0;
    for (Member *mem = ty->members; mem; mem = mem->next) {
        mem->offset = offset;
        offset += size_of(mem->ty);
    }

    return ty;
}

// struct-member = basetype ident ("[" num "]")* ";"
Member *struct_member() {
    Member *mem = calloc(1, sizeof(Member));
    mem->ty = basetype();
    mem->name = expect_ident();
    mem->ty = read_type_suffix(mem->ty);
    expect(";");
    return mem;
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
    vl->var = push_var(name, ty, true);
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
    Var *var = push_var(name, ty, true);

    if (consume(";"))
        return new_node(ND_NULL, tok);

    expect("=");
    Node *lhs = new_var(var, tok);
    Node *rhs = expr();
    expect(";");
    Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
    return new_unary(ND_EXPR_STMT, node, tok);
}

// typename = "int" | "char" | "struct"
bool is_typename() {
    return peek("int") || peek("char") || peek("struct");
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

        VarList *sc = scope;
        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }
        scope = sc;

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

    if (is_typename())
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

// postfix = primary ("[" expr "]" | "." ident)*
Node *postfix() {
    Node *node = primary();
    Token *tok;

    for (;;) {
        if ((tok = consume("["))) {
            // x[y] is short for *(x+y)
            Node *exp = new_binary(ND_ADD, node, expr(), tok);
            expect("]");
            node = new_unary(ND_DEREF, exp, tok);
            continue;
        }

        if ((tok = consume("."))) {
            node = new_unary(ND_MEMBER, node, tok);
            node->member_name = expect_ident();
            continue;
        }

        return node;
    }
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

// stmt-expr = "(" "{" stmt stmt* "}" ")"
//
// GNUのC拡張である文の中に式を埋め込める機能です．
Node *stmt_expr(Token *tok) {
    VarList *sc = scope;

    Node *node = new_node(ND_STMT_EXPR, tok);
    node->body = stmt();
    Node *cur = node->body;

    while (!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }
    expect(")");

    scope = sc;

    if (cur->kind != ND_EXPR_STMT)
        error_tok(cur->tok, "stmt expr returning void is not supported");
    *cur = *cur->lhs;
    return node;
}

// primary = num
//         | str
//         | ident func_args?
//         | "(" expr ")"
//         | "sizeof" unary
//         | stmt-expr
Node *primary() {
    // 次のトークンが"("なら，"(" expr ")"のはず
    if (consume("(")) {
        if (consume("{"))
            return stmt_expr(token->next);

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
    if (tok->kind == TK_STR) {
        token = token->next;

        Type *ty = array_of(char_type(), tok->cont_len);
        Var *var = push_var(new_label(), ty, false);
        var->contents = tok->contents;
        var->cont_len = tok->cont_len;
        return new_var(var, tok);
    }

    if (tok->kind != TK_NUM)
        error_tok(tok, "expected expression");
    // そうでなければ数値のはず
    return new_num(expect_number(), tok);
}
