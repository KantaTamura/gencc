#include "gencc.h"

int offsets = 0;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // トークナイズしてパースする
    user_input = argv[1];
    token = tokenize();
    Node *node = program();

    int offset = 0;
    for (Var *var = locals; var; var = var->next) {
        offset += 8;
        var->offset = offset;
    }
    offsets = offset;

    codegen(node);
    return 0;
}