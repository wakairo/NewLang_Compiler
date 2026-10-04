/* Infrastructure only: no NewLang source semantics or optimizer promises. */
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

#include <stdio.h>
#include <string.h>

int main(void)
{
    int status = 1;
    LLVMContextRef const context = LLVMContextCreate(); /* Owned. */
    if (context == NULL) {
        return 1;
    }
    LLVMModuleRef const module =
        LLVMModuleCreateWithNameInContext("p0", context);
    LLVMBuilderRef const builder = LLVMCreateBuilderInContext(context);
    char *verification = NULL; /* Owned LLVM message; disposed below. */
    char *ir = NULL;
    if (module == NULL || builder == NULL) {
        goto cleanup;
    }
    LLVMTypeRef const integer = LLVMInt32TypeInContext(context); /* Borrowed. */
    LLVMTypeRef const signature = LLVMFunctionType(integer, NULL, 0, 0);
    LLVMValueRef const function =
        LLVMAddFunction(module, "p0_answer", signature);
    LLVMBasicBlockRef const entry =
        LLVMAppendBasicBlockInContext(context, function, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
    LLVMBuildRet(builder, LLVMConstInt(integer, 42, 0));

    if (LLVMVerifyModule(module, LLVMReturnStatusAction, &verification) != 0) {
        fprintf(stderr, "LLVM verification failed: %s\n",
                verification != NULL ? verification : "no message");
        goto cleanup;
    }
    ir = LLVMPrintModuleToString(module);
    if (ir == NULL || strstr(ir, "define i32 @p0_answer()") == NULL ||
        strstr(ir, "ret i32 42") == NULL) {
        goto cleanup;
    }
    status = puts("LLVM C API smoke: verified p0_answer returning i32 42") >= 0
                 ? 0
                 : 1;

cleanup:
    if (ir != NULL) {
        LLVMDisposeMessage(ir);
    }
    if (verification != NULL) {
        LLVMDisposeMessage(verification);
    }
    if (builder != NULL) {
        LLVMDisposeBuilder(builder);
    }
    if (module != NULL) {
        LLVMDisposeModule(module);
    }
    LLVMContextDispose(context);
    return status;
}
