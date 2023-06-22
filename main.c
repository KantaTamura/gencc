#include "gencc.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        error("%s: invalid number of arguments", argv[0]);
    }

    // トークナイズしてパースする
    user_input = argv[1];
    token = tokenize();
    Program *prog = program();
    add_type(prog);

    // offsetを計算
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        long offset = 0;
        for (VarList *vl = fn->locals; vl; vl = vl->next) {
            Var *var = vl->var;
            offset += size_of(var->ty);
            var->offset = offset;
        }
        fn->stack_size = offset;
    }

    codegen(prog);
    return 0;
}