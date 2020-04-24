//
// Created by xzn on 2020/4/23.
//

// load & execute

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "static.h"

#define MAXMEM 1024

/// load from file

int fTrace = 0;
int entryPoint;
int nInst;
int ixData;
int param;
INSTRUCT Inst[MAXMEM];
char* DataSection[MAXMEM];


intptr_t mem[MAXMEM];       // mem
intptr_t sp,                // 堆栈指针
         bp,                // 基准指针
         pc;                // pc
/// load ends
char* ps = "putstr";
char* pn = "putnum";

intptr_t pos;               // mem索引
intptr_t location[1024];    // 标签地址排列

void error(char *errMes){
    printf("#Error : %s\n",errMes);
    exit(1);
}

void push(intptr_t val) {
    if (sp <= pos)
        error("stack overflow!");
    mem[--sp] = val;
}


intptr_t pop() {
    if (sp >= MAXMEM)
        error("stack empty!");
    return mem[sp++];
}


intptr_t getStr(char *str) {
    int n = strlen(str);
    if (str[n-1] == '"')
        str[n-1] = '\0';
    return (intptr_t)str + 1;
}


void printInst(int n, INSTRUCT* pI){
    printf("%3d: ", n);
    printf("%s ", OPCODE[pI->opcode]);
    if (pI->type == NIL) {
        //NIL print over
    } else if (pI->type == STR) {
        printf("%s ", TYPE[pI->type]);
        printf("%s", (const char*)pI->val);
    } else {
        printf("%s ", TYPE[pI->type]);
        printf("%ld", pI->val);
    }
    printf("\n");
}


int execute() {
    pos = 0;
    intptr_t addr, rtn;

    for (int n = 0; n < nInst; n++) {
        if (Inst[n].opcode == LABEL)
            location[Inst[n].val] = n;
    }
    for (int n = 0; n < ixData; n++)
        mem[pos++] = atoi(DataSection[n]);

    sp = MAXMEM;        // 堆栈指针初始化
    pc = entryPoint;
    push(param);
    push(-1);
    while (pc < nInst) {
        if (fTrace) printInst(pc, &Inst[pc]);
        int type = Inst[pc].type;
        intptr_t val  = Inst[pc].val;
        switch (Inst[pc++].opcode) {
            case FUNC:  break;
            case LABEL: break;
            case ENTRY:  // 函数入口
                push(bp);
                bp = sp;
                break;
            case ADDSP:
                rtn = pop();        // 函数返回值位于堆栈顶部
                sp += val;          // 删参数
                push(rtn);          // 重新在堆栈顶部设置函数返回值
                break;
            case PUSH:
                if (type == SKV) push(mem[bp + val]);           // push Local
                else if (type == SKR) push(bp + val);           // push Local Addr
                else if (type == STR) push(getStr((char*)val));     // push 字符串地址
                else if (type == IMM) push(val);
                else{ printf("exec.PUSH: %d", type); exit(1); }
                break;
            case MOV:
                val  = pop();
                addr = pop();
                mem[addr] = val;
                break;
            case ADD: mem[sp+1] += mem[sp]; sp++; break;
            case SUB: mem[sp+1] -= mem[sp]; sp++; break;
            case MUL: mem[sp+1] *= mem[sp]; sp++; break;
            case DIV: mem[sp+1] /= mem[sp]; sp++; break;
            case MOD: mem[sp+1] %= mem[sp]; sp++; break;
            case GT:  mem[sp+1] = (mem[sp+1] >  mem[sp]); sp++; break;
            case GE:  mem[sp+1] = (mem[sp+1] >= mem[sp]); sp++; break;
            case LT:  mem[sp+1] = (mem[sp+1] <  mem[sp]); sp++; break;
            case LE:  mem[sp+1] = (mem[sp+1] <= mem[sp]); sp++; break;
            case EQ:  mem[sp+1] = (mem[sp+1] == mem[sp]); sp++; break;
            case NE:  mem[sp+1] = (mem[sp+1] != mem[sp]); sp++; break;
            case JZ:  if (mem[sp++] == 0) pc = location[val]; break;    // jz xxx
            case JMP: pc = location[val]; break;
            case CMP: mem[sp+1] -= mem[sp]; sp++; break;
            case CALL:
                if (type == STR) {  // putstr | putnum
                    const char* fn = (const char*)val;
                    if (strcmp("putstr", fn) == 0) {
                        rtn = printf("%s\n", (const char*)mem[sp]);
                    } else if (strcmp("putnum", fn) == 0) {
                        rtn = printf("%d\n", mem[sp]);
                    }
                    push(rtn);
                } else {
                    push(pc);           // 保存返回地址
                    pc = location[val]; // pc设置函数起始位置
                }
                break;
            case RET:       // 返回函数
                rtn = pop();        // 返回值
                sp = bp;            // 堆栈指针恢复位置
                bp = pop();         // 基准指针恢复位置
                pc = pop();         // pc设置下一个命令位置
                if (pc == -1)
                    return rtn;
                push(rtn);          // 函数返回堆栈顶
                break;
            default:{
                printf("unknown opcode: %d\n", Inst[pc-1].opcode);
                exit(1);
            }

        }
    }
    return 1;
}


typedef long int myuint;    //防止intptr_t溢出int

// Bina to Demi
myuint B2D(char* buf){
    int len = strlen(buf), i = 0;
    int rev = buf[0] == '1' ? 1 : 0;
    char cc[len];
    myuint res = 0;

    strcpy(cc, buf);
    if(rev)
        for(int q=0;q<len;q++)
            cc[q] = cc[q] == '0' ? '1' : '0';

    while(cc[i] == '0')
        i++;

    char nBuf[len-i];
    strncpy(nBuf,cc+i,len-i);
    nBuf[len-i] = '\0';

    for(int j = 0;j<strlen(nBuf);j++){
        res <<= 1;
        res += nBuf[j] == '1' ? 1 : 0;
    }
    if(rev)
        res = 0 - (res + 1);
    return res;
}


char buf[127];
void readBi(FILE* file){
    fscanf(file, "%s", buf);
}

void loadImage(char* fileName){
    // #1 int entryPoint
    // #2 int nInst
    // #3 Inst
    //    int Inst[x].opcode
    //    int Inst[x].type
    //    int Inst[x].val
    // #4 int ixData
    // #5 sizeof(DataSection[i])
    // #6 DataSection
    // #7 param

    FILE* file = fopen(fileName, "r");
    INSPTR insptr;
    int intSize = sizeof(int);
    int opSize = sizeof(insptr->opcode);
    int typeSize = sizeof(insptr->type);
    int valSize = sizeof(insptr->val);
    int flag = -1;

    readBi(file);
    entryPoint = (int)B2D(buf);

    readBi(file);
    nInst = (int)B2D(buf);
    for(int n = 0;n<nInst;n++){
        readBi(file);
        Inst[n].opcode = B2D(buf);
        readBi(file);
        Inst[n].type = B2D(buf);

        readBi(file);
        if(Inst[n].type == STR && (flag = (int)B2D(buf)) == 0){ //native
            readBi(file);
            int nativeLen = (int)B2D(buf);
            char nativeName[nativeLen];
            for(int m = 0;m < nativeLen;m++){
                readBi(file);
                nativeName[m] = (char)B2D(buf);
            }
            nativeName[nativeLen] = '\0';
            if(strcmp(nativeName, ps) == 0)
                Inst[n].val = (intptr_t)ps;
            else if(strcmp(nativeName, pn) == 0)
                Inst[n].val = (intptr_t)pn;
            else{
                printf("unknown native name %s\n",nativeName);
                exit(1);
            }
        }else{
            readBi(file);
            Inst[n].val = (intptr_t )B2D(buf);
        }
    }

    readBi(file);
    ixData = (int)B2D(buf);

    for(int n = 0;n<ixData;n++){
        readBi(file);
        int dsLen = B2D(buf);
        char ds[dsLen];
        for(int j = 0; j<dsLen; j++){
            readBi(file);
            ds[j] = (char)B2D(buf);
        }
        ds[dsLen] = '\0';
        DataSection[n] = ds;
    }

    readBi(file);
    param = (int)B2D(buf);

    fclose(file);

}

int main(int argc, char *argv[]) {
    char* fileName;

    if(argc == 1)
        printf("#AVSVM: Usage: avsvm_le src.o\n");

    fileName = argv[1];
    loadImage(fileName);

    execute();
    //printf("end\n");
    return 0;
}