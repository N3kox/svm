//
// Created by xzn on 2020/4/23.
//

#ifndef SVM_STATIC_H
#define SVM_STATIC_H

typedef struct _INSTRUCT {
    int opcode;     // operator
    int type;
    intptr_t val;
} INSTRUCT, *INSPTR;

// all opcodes
enum { PUSH = 0, ENTRY, POP, MOV, ADD, ADDSP, SUB, MUL, DIV, MOD, RET, CALL, JZ, JMP, CMP, LT, GT, LE, GE, EQ, NE, FUNC, LABEL };
char* OPCODE[] = { "push", "entry", "pop", "mov", "add", "addsp", "sub", "mul", "div", "mod", "ret", "call", "jz", "jmp", "cmp", "lt", "gt", "le", "ge", "eq", "ne", "func", "label" };

// all types
enum { NIL, IMM, STR, MEM, REF, SKV, SKR, LOC, SP };
char* TYPE[] = { "nil", "int", "str", "mem", "ref", "stack-val", "stack-ref", "loc", "sp" };

enum { VAL = 0, ADDR };                 // VAL: 内容, ADDR: 地址
enum { ST_FUNC = 0, ST_GVAR };          // ST_FUNC: 函数定义, ST_GVAR: 变量声明

#endif //SVM_STATIC_H
