#ifndef SETMODELNAMEANDFUNCTIONPOINTERS_H
#define SETMODELNAMEANDFUNCTIONPOINTERS_H

typedef struct {
    void(*reference_varis) (void);              // Reference-Funk.
    int(*init_test_object) (void);              // Init-Funk.
    void(*cyclic_test_object) (void);           // Zyklische Funk.
    void(*terminate_test_object) (void);         // Beenden-Funk.
} MODEL_FUNCTION_POINTERS;

// This function has it own header. The definition lves inside RemoteMasterModelInterface.c
void SetModelNameAndFunctionPointers(const char *par_Name, int par_Prio, MODEL_FUNCTION_POINTERS *par_FunctionPointers);

#endif // SETMODELNAMEANDFUNCTIONPOINTERS_H
