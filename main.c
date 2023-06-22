#include "gencc.h"

// ファイルの内容を読み込んで返す
char *read_file(char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp)
        error("cannot open %s: %s", path, strerror(errno));

    int filemax = 10 * 1024 * 1024;
    char *buf = malloc(filemax);
    int size = fread(buf, 1, filemax - 2, fp);
    if (!feof(fp))
        error("%s: file too large");

    if (size == 0 || buf[size - 1] != '\n')
        buf[size++] = '\n';
    buf[size] = '\0';
    return buf;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        error("%s: invalid number of arguments", argv[0]);
    }

    // トークナイズしてパースする
    filename = argv[1];
    user_input = read_file(filename);
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