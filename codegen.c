#include "gencc.h"

int label_seq = 0;
char *argreg[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
char *funcname;

void gen(Node *node);

void gen_addr(Node *node) {
    switch (node->kind) {
        case ND_VAR:
            printf("    mov rax, rbp\n");
            printf("    sub rax, %ld\n", node->var->offset);
            printf("    push rax\n");
            return;
        case ND_DEREF:
            gen(node->lhs);
            return;
        default:
            break;
    }

    error_tok(node->tok, "not a variable");
}

void load() {
    printf("    pop rax\n");
    printf("    mov rax, [rax]\n");
    printf("    push rax\n");
}

void store() {
    printf("    pop rdi\n");
    printf("    pop rax\n");
    printf("    mov [rax], rdi\n");
    printf("    push rdi\n");
}

void gen_lval(Node *node) {
    if (node->ty->kind == TY_ARRAY)
        error_tok(node->tok, "not an lvalue");
    gen_addr(node);
}

void gen(Node *node) {
    switch (node->kind) {
        case ND_NULL:
            return;
        case ND_IF: {
            int seq = label_seq++;
            gen(node->cond);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            if (node->els) {
                printf("    je .Lelse%d\n", seq);
                gen(node->then);
                printf("    jmp .Lend%d\n", seq);
                printf(".Lelse%d:\n", seq);
                gen(node->els);
                printf(".Lend%d:\n", seq);
            } else {
                printf("    je .Lend%d\n", seq);
                gen(node->then);
                printf(".Lend%d:\n", seq);
            }
            return;
        }
        case ND_WHILE: {
            int seq = label_seq++;
            printf(".Lbegin%d:\n", seq);
            gen(node->cond);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .Lend%d\n", seq);
            gen(node->then);
            printf("    jmp .Lbegin%d\n", seq);
            printf(".Lend%d:\n", seq);
            return;
        }
        case ND_FOR: {
            int seq = label_seq++;
            if (node->init)
                gen(node->init);
            printf(".Lbegin%d:\n", seq);
            if (node->cond) {
                gen(node->cond);
                printf("    pop rax\n");
                printf("    cmp rax, 0\n");
                printf("    je .Lend%d\n", seq);
            }
            gen(node->then);
            if (node->inc)
                gen(node->inc);
            printf("    jmp .Lbegin%d\n", seq);
            printf(".Lend%d:\n", seq);
            return;
        }
        case ND_FUNCALL: {
            int nargs = 0;
            for (Node *arg = node->args; arg; arg = arg->next) {
                gen(arg);
                nargs++;
            }

            for (int i = nargs - 1; i >= 0; i--)
                printf("    pop %s\n", argreg[i]);

            // 関数呼び出しをする前にRSPが16の倍数になっている必要がある．
            // push, popは8バイトRSPをずらすため，それを補正する
            int seq = label_seq++;
            printf("    mov rax, rsp\n");       // rax <= rsp
            printf("    and rax, 15\n");        // rax <= rax & 0b1111
            printf("    jnz .Lcall%d\n", seq);  // rax is not 0 => jump .Lcall
            // if ( rsp 下位4bit = 0b0000 )
            printf("    mov rax, 0\n");
            printf("    call %s\n", node->funcname);
            printf("    jmp .Lend%d\n", seq);
            // if ( rsp が16の倍数でない )
            printf("    .Lcall%d:\n", seq);
            printf("    sub rsp, 8\n");         // rsp <= rsp - 8 : 16の倍数へ
            printf("    mov rax, 0\n");
            printf("    call %s\n", node->funcname);
            printf("    add rsp, 8\n");         // rsp <= rsp + 8 : もとに戻す

            printf(".Lend%d:\n", seq);
            printf("    push rax\n");
            return;
        }
        case ND_EXPR_STMT:
            gen(node->lhs);
            printf("    add rsp, 8\n"); // genの末尾で使用しない値がpopされているので削除する
        case ND_BLOCK: {
            for (Node *n = node->body; n; n = n->next) {
                gen(n);
            }
            return;
        }
        case ND_RETURN:
            gen(node->lhs);
            printf("    pop rax\n");
            printf("    jmp .Lreturn.%s\n", funcname);
            return;
        default:
            break;
    }

    switch (node->kind) {
        case ND_ADDR:
            gen_addr(node->lhs);
            return;
        case ND_DEREF:
            gen(node->lhs);
            if (node->ty->kind != TY_ARRAY)
                load();
            return;
        case ND_NUM:
            printf("    push %ld\n", node->val);
            return;
        case ND_VAR:
            gen_addr(node);
            if (node->ty->kind != TY_ARRAY)
                load();
            return;
        case ND_ASSIGN:
            gen_lval(node->lhs);
            gen(node->rhs);
            store();
            return;
        default:
            break;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            if (node->ty->base)
                printf("    imul rdi, %ld\n", size_of(node->ty->base)); // ptr + x => (ptr + x*8)
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            if (node->ty->base)
                printf("    imul rdi, %ld\n", size_of(node->ty->base)); // ptr - x => (ptr - x*8)
            printf("    sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("    imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("    cqo\n");
            printf("    idiv rdi\n");
            break;
        case ND_EQ:
            printf("    cmp rax, rdi\n");
            printf("    sete al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_NE:
            printf("    cmp rax, rdi\n");
            printf("    setne al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_LT:
            printf("    cmp rax, rdi\n");
            printf("    setl al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_LE:
            printf("    cmp rax, rdi\n");
            printf("    setle al\n");
            printf("    movzb rax, al\n");
            break;
        default:
            // unreachable
            error_tok(node->tok, "invalid node");
    }

    printf("    push rax\n");
}

void codegen(Function *prog) {
    printf(".intel_syntax noprefix\n");

    for (Function *fn = prog; fn; fn = fn->next) {
        printf(".global %s\n", fn->name);
        printf("%s:\n", fn->name);
        funcname = fn->name;

        // prologue
        // 変数26個分の領域を確保する
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %ld\n", fn->stack_size);

        // 関数の引数をスタックにプッシュ
        int i = 0;
        for (VarList *vl = fn->params; vl; vl = vl->next) {
            Var *var = vl->var;
            printf("    mov [rbp-%ld], %s\n", var->offset, argreg[i++]);
        }

        // 抽象構文木を下りながらコード生成
        for (Node *n = fn->node; n; n = n->next) {
            gen(n);
        }

        // epilogue
        // ND_RETURN ノードからジャンプする
        printf(".Lreturn.%s:\n", funcname);
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
    }
}