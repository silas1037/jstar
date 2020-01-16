#include "jstar.h"

#include "jsrparse/lex.h"
#include "jsrparse/parser.h"

#include "linenoise.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define JSTARPATH "JSTARPATH"

static JStarVM *vm;

static void header() {
    printf("J* Version %s\n", JSTAR_VERSION_STRING);
    printf("%s on %s\n", JSTAR_COMPILER, JSTAR_PLATFORM);
}

// Little hack to enable adding a tab in linenoise
static void completion(const char *buf, linenoiseCompletions *lc) {
    char indented[1025];
    snprintf(indented, sizeof(indented), "%s    ", buf);
    linenoiseAddCompletion(lc, indented);
}

static int countBlocks(const char *line) {
    Lexer lex;
    Token tok;

    initLexer(&lex, line);
    nextToken(&lex, &tok);

    int depth = 0;
    while(tok.type != TOK_EOF && tok.type != TOK_NEWLINE) {
        switch(tok.type) {
        case TOK_LCURLY:
        case TOK_BEGIN:
        case TOK_CLASS:
        case TOK_THEN:
        case TOK_WITH:
        case TOK_FUN:
        case TOK_TRY:
        case TOK_DO:
            depth++;
            break;
        case TOK_RCURLY:
        case TOK_ELIF:
        case TOK_END:
            depth--;
            break;
        default:
            break;
        }
        nextToken(&lex, &tok);
    }
    return depth;
}

static void addPrintIfExpr(JStarBuffer *sb) {
    Expr *e;
    // If the line is a (correctly formed) expression
    if((e = parseExpression(NULL, sb->data)) != NULL) {
        freeExpr(e);
        // assign the result of the expression to `_`
        jsrBufferPrependstr(sb, "var _ = ");
        jsrBufferAppendChar(sb, '\n');
        // print `_` if not null
        jsrBufferAppendstr(sb, "if _ != null then print(_) end");
    }
}

static void dorepl() {
    header();
    linenoiseSetCompletionCallback(completion);

    JStarBuffer src;
    jsrBufferInit(vm, &src);

    char *line;
    while((line = linenoise("J*>> ")) != NULL) {
        linenoiseHistoryAdd(line);
        int depth = countBlocks(line);
        jsrBufferAppendstr(&src, line);
        free(line);

        while(depth > 0 && (line = linenoise(".... ")) != NULL) {
            linenoiseHistoryAdd(line);
            jsrBufferAppendChar(&src, '\n');
            depth += countBlocks(line);
            jsrBufferAppendstr(&src, line);
            free(line);
        }

        addPrintIfExpr(&src);
        jsrEvaluate(vm, "<stdin>", src.data);
        jsrBufferClear(&src);
    }

    jsrBufferFree(&src);
    linenoiseHistoryFree();
}

static void initImportPaths(char **paths, int count) {
    for(int i = 0; i < count; i++) {
        jsrAddImportPath(vm, paths[i]);
    }

    const char *jstarPath = getenv(JSTARPATH);
    if(jstarPath == NULL) return;

    JStarBuffer path;
    jsrBufferInit(vm, &path);

    size_t last = 0;
    size_t pathLen = strlen(jstarPath);
    for(size_t i = 0; i < pathLen; i++) {
        if(jstarPath[i] == ':') {
            jsrBufferAppend(&path, jstarPath + last, i - last);
            jsrAddImportPath(vm, path.data);
            jsrBufferClear(&path);
            last = i + 1;
        }
    }

    jsrBufferAppend(&path, jstarPath + last, pathLen - last);
    jsrAddImportPath(vm, path.data);
    jsrBufferFree(&path);
}

int main(int argc, const char **argv) {
    vm = jsrNewVM();

    EvalResult res = VM_EVAL_SUCCESS;
    if(argc == 1) {
        initImportPaths(NULL, 0);
        dorepl();
    } else {
        // set command line args for use in scripts
        jsrInitCommandLineArgs(vm, argc - 2, argv + 2);

        // set base import path to script's directory
        char *directory = strrchr(argv[1], '/');
        if(directory != NULL) {
            size_t length = directory - argv[1] + 1;
            char *path = calloc(length + 1, 1);
            memcpy(path, argv[1], length);
            initImportPaths(&path, 1);
            free(path);
        }

        // read file and evaluate
        char *src = jsrReadFile(argv[1]);
        if(src == NULL) {
            fprintf(stderr, "Error reading input file ");
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }

        res = jsrEvaluate(vm, argv[1], src);
        free(src);
    }

    jsrFreeVM(vm);
    return res;
}
