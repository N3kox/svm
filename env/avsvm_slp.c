//BISTU SE 20 AVSVM PROJECT
//scanner & lex & parser
//author : xzn
//date : 4.20.2020

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define TKSIZE 1000
#define LEN2OP "== != <= >= != += -= *= /= >> << ++ -- && || ->"

int debug;
int fToken;
int fTrace;
int err;
char *Token[TKSIZE];    //Token表
int nToken;             //Token计数
int tix;                //Token指针

#define isword(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
#define isdigit(c) ((c >= '0' && c <= '9'))
int isWordOrNum(int c) { return (isword(c) || isdigit(c)); }


/// Part A: 词法分析
/*
 * myLex
 * 字符串解析仅支持英文+数字
 */
int lex(const char *src) {
    tix = -1;
    nToken = 0;
    char linebuf[256],  //行长限制256
            buf[128],   //buf128
            op2[4];     //双目运算符缓存

    FILE *fp = fopen(src, "r");
    if (fp == NULL) {
        printf("#Error: File %s open error\n", src);
        exit(1);
    }

    while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {   //按行处理，token保存后打印
        for (char *p = linebuf; *p != '\0';) {
            if (*p <= ' ') {  //space越过
                p++;
                continue;
            }
            if (p[0] == '/' && p[1] == '/') //注释结束此行
                break;
            char *pStart = p;   //记录单词首位

            if (*p == '"') {     //双引号:解释两个引号之间内容为字符串token
                for (p++; *p != '\0' && *p != '"'; p++) {
                    if (*p == '\\')
                        p++;
                }
                if (*p++ != '"') {
                    printf("#Error: Cannot find pair quote in line %s", pStart);
                    exit(1);
                }
            } else if (isdigit(*p)) {
                for (p++; *p != '\0' && isdigit(*p); p++);
            } else if (strncmp(pStart, "char*", 5) == 0) { //char* before space
                p += 5;
            } else if (isword(*p)) {
                for (p++; *p != '\0' && isWordOrNum(*p); p++);
            } else {  //双目运算符
                op2[0] = p[0];
                op2[1] = p[1];
                op2[2] = ' ';
                op2[3] = '\0';
                p += strstr(LEN2OP, op2) != NULL ? 2 : 1;
            }

            for (int i = 0; i < p - pStart; i++)
                buf[i] = pStart[i];
            buf[p - pStart] = 0;    //buf装载单词

            if (nToken == TKSIZE) {
                printf("#Error: OOM while executing lex\n");
                exit(1);
            }
            Token[nToken++] = strdup(buf);

            //以(;, {, })三个符号结束时换行输出
            if (fToken)
                printf(strchr(";{}", *buf) != NULL ? "%s\n" : "%s ", buf);
        }
        //break;
    }
    fclose(fp);
    return 0;
}

/// Part B: 名称管理
#define SIZEGLOBAL 1024
#define SIZELOCAL 1024
enum { NM_VAR = 1, NM_FUNC};                //名称类型
enum { AD_DATA = 0, AD_STACK, AD_CODE};     //地址类型 : AD_DATA 静态,常量 ; AD_STACK 形参,局部变量 ; CODE 函数,方法..
typedef struct _Name {
    int type;           // VAR 变量 | FUNC 函数
    char *dataType;     // 数据类型
    char *name;         // 变量或函数名称
    int addrType;       // 地址类型
    intptr_t address;   // 相对地址
} Name;

Name *GName[SIZEGLOBAL];
Name *LName[SIZELOCAL];
int nGlobal, nLocal;

/*
 * domian: 0-global, 1-local
 */
Name *appendName(int domain, int type, char *dataType, char *name, int addrType, intptr_t addr) {
    Name nm = {type, dataType, name, addrType, addr};
    Name *pNew = calloc(1, sizeof(Name));
    if (pNew == NULL) {
        printf("#Error: OOM while executing appendName\n");
        exit(1);
    }
    memcpy(pNew, &nm, sizeof(Name));
    if (domain == 0 && nGlobal < SIZEGLOBAL - 1)
        GName[nGlobal++] = pNew;
    else if (domain == 1 && nLocal < SIZELOCAL - 1)
        LName[nLocal++] = pNew;
    else {
        printf("#Error: parameter OOR\n");
        exit(1);
    }
    return pNew;
}

/*
 * domian: 0-global, 1-local
 */
Name *getNameFromTable(int domain, int type, char *name, int fErr) {
    int nEntry = domain == 0 ? nGlobal : nLocal;
    for (int n = 0; n < nEntry; n++) {
        Name *cur = domain == 0 ? GName[n] : LName[n];
        if (strcmp(cur->name, name) == 0 && (cur->type & type) != 0)
            return cur;
    }
    if (fErr && domain == 0) {
        printf("#Error: %s not declared\n", name);
        exit(1);
    }
    return NULL;
}


Name *getNameFromAllTable(int type, char *name, int fErr) {
    Name *pName = getNameFromTable(1, type, name, fErr);    //search local first
    if (pName != NULL)
        return pName;
    return getNameFromTable(0, type, name, fErr);   //search global
}


/// Part C: 语法分析
/// "*" means 0 or more
/// "+" means 1 or more

/// Identifier ::= identifier
/// PrimaryExpression ::= Identifier | "(" Expr ")" | Constant | FunctionCall
/// MulExpression ::= PrimaryExpression ( ("*" | "/" | "%") PrimaryExpression )*
/// AddExpression ::= MulExpression ( ("+" | "-") MulExpression )*
/// RelationalExpression ::= addExpr [("<" | ">" | "<=" | ">=") addExpr]
/// EqualityExpression ::= RelationalExpr [ ("==" | "!=") RelationalExpr ]
/// expression ::= [Identifier "="] addExpression
/// TypeSpecifier ::= "void" | "char*" | "int"
/// VarDeclarator ::= Identifier
/// VariableDeclaration ::= TypeSpecifier VarDeclarator ["=" Initializer]("," VarDeclarator ["=" Initializer])*
/// CompoundStatement ::= "{" (Statements)* "}"
/// IfStatement ::= "if" "(" Expression ")" Statement [ "else" Statement ]+
/// WhileStatement ::= "while" "(" Expression ")" Statement
/// ReturnStatement ::= "return" [Expression] ";"
/// Statement ::= CompoundStmt | IfStmt | ReturnStmt| Expression ";" | ";"
/// FunctionDefinition ::= TypeSpecifier  VarDeclarator "(" [ VarDeclaration ("," VarDeclaration)* ] ")" CompoundStatement
/// Program ::= (FunctionDefinition | VariableDeclaration ";")*

#include "static.h"
void expression(int mode);
void statement(intptr_t locBreak, intptr_t locContinue);

#define SIZEDATASECTION 1024
char* DataSection[SIZEDATASECTION];     // 变量+字符串管理

int ixData;                             // 管理数据计数
intptr_t baseSpace;                     // 堆栈相对地址

int numLabel = 1;       // 标签序列号
INSTRUCT Inst[1000];    // 中间码表
int nInst = 0;          // 中间码计数
int entryPoint;         // 程序入口（main）

/// 返回label位置并+1
int loc() { return numLabel++; }

/// token和s比较
int is(char* s) {
    if(debug)
        printf("#DEBUG: Token[tix]=<%s>\t\tchar=<%s>\n",Token[tix],s);
    return strcmp(Token[tix], s) == 0;
}

/// 检查token与内容是否匹配，匹配成功token指针位置+1
int ispp(char* s){
    if(is(s)){
        tix++;
        return 1;
    }
    return 0;
}

/// return type check
int isTypeSpecifier(){
    return (is("void") || is("int")) || is("char*");
}

/// 函数定义检查
/// type name "(" ......
int isFunctionDefinition(){
    return *Token[tix+2] == '(';
}

/// 越过s字符处理
void skip(char* s){
    if(!ispp(s)){
        printf("#Error: char'%s' excepted\n",s);
        exit(1);
    }
}

/// 打印中间码
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

/// build instance
INSTRUCT* outInst2(int opcode, int type, intptr_t val){
    INSTRUCT inst = {opcode, type, val};
    return memcpy(&Inst[nInst++], &inst, sizeof(inst));
}

INSTRUCT* outInst(int code){
    return outInst2(code, 0, 0);
}


/// 表达式解析
/// PrimExpression ::= Identifier | "(" Expr ")" | Constant | FunctionCall

/// 函数调用
/// FunctionCall ::= function_name "(" Expression [ "," Expression ] ")"
void funcionCall(){
    int n,
        nParam,         // 参数数量计数
        posParam[20],   // 参数名
        nInstSave,      // instance缓存
        ixDataSave,     // data缓存
        ixNext;         // 记录tix+1

    char* name = Token[tix++];  // functionName
    Name* pName = getNameFromTable(0, NM_FUNC, name, 1);
    skip("(");

    // 参数最后入栈
    // 先求参数位置
    nInstSave = nInst;
    ixDataSave = ixData;

    for (nParam = 0; !is(")"); nParam++){   // 参数解析
        posParam[nParam] = tix;             //各参数位置（tix）存表
        expression(VAL);
        ispp(",");
    }

    ixNext = tix + 1;   // 越过 ")"
    nInst = nInstSave;
    ixData = ixDataSave;    // 丢弃数据

    for(n = nParam; --n >= 0;){
        tix = posParam[n];
        expression(VAL);    // 倒序解释参数并堆叠
    }

    tix = ixNext;
    if(pName->address > 0)
        outInst2(CALL, IMM, pName->address);
    else
        outInst2(CALL, STR, (intptr_t)pName->name);

    outInst2(ADDSP, IMM, nParam);   // 堆栈指针还原
}


void primaryExpression(int mode){   // 顶层解析
    if(isdigit(*Token[tix])){       // 数值常量
        // TODO: atoi - strtol : error control
        outInst2(PUSH, IMM, atoi(Token[tix++]));
    }else if(*Token[tix] == '"'){      // 字符串常量
        outInst2(PUSH, STR, (intptr_t)Token[tix++]);
    }else if(ispp("(")){            // "(" expression ")"
        expression(VAL);
        skip(")");
    }else if(*Token[tix+1] == '('){    // name "(" -> function
        funcionCall();
    }else if(isword(*Token[tix])){  // identifier
        Name *pName = getNameFromAllTable(NM_VAR, Token[tix], 1);
        int type = (pName->addrType == AD_STACK) ? (mode == VAL ? SKV : SKR) : (mode == VAL ? MEM : REF);
        outInst2(PUSH, type, pName->address);
        tix++;
    }else {     // unknown
        printf("#Error:<%s>\n",Token[tix]);
        exit(1);
    }
}


/// MulExp ::= PriExp(( "*" | "/" | "%" ) PriExp )*
void mulExpression(int mode){
    int fMul = 0, fDiv = 0;
    primaryExpression(mode);    // PriExp
    while((fMul = ispp("*")) || (fDiv = ispp("/")) || ispp("%")){
        primaryExpression(mode);
        outInst(fMul ? MUL : fDiv ? DIV : MOD);
    }   // (( "*" | "/" | "%" ) PriExp )
}


/// AddExp ::= MulExp(( "+" | "-") MulExp) *
void addExpression(int mode){
    int fAdd;
    mulExpression(mode);        // MulExp
    while((fAdd = ispp("+")) || ispp("-")){ // ( "+" | "-") MulExp
        mulExpression(mode);
        outInst(fAdd ? ADD : SUB);
    }
}


/// RelationalExpression ::= addExpr [("<" | ">" | "<=" | ">=") addExpr]
void relationalExpression(int mode) {    // 带符号整数
    int fLT=0, fGT=0, fLE=0;
    addExpression(mode);        // addExpr
    if ((fLT = ispp("<")) || (fGT = ispp(">")) || (fLE = ispp("<=")) || ispp(">=")){
        addExpression(mode);
        outInst(fLT ? LT : fGT ? GT : fLE ? LE : GE);
    }   // [("<" | ">" | "<=" | ">=") addExpr]
}


/// EqualityExpression ::= RelationalExpr [ ("==" | "!=") RelationalExpr ]
void equalityExpression(int mode) {
    int fEQ;
    relationalExpression(mode);         // RelationalExpr
    if ((fEQ = ispp("==")) || ispp("!=")) { // [ ("==" | "!=") RelationalExpr ]
        relationalExpression(mode);
        outInst(fEQ ? EQ : NE);
    }
}

/// 赋值
void assign() {
    primaryExpression(ADDR);     // var
    skip("=");
    addExpression(VAL);         // val
    outInst(MOV);               // Mem[st1] = st0
}


/// expression ::= [Identifier "="] EqualityExpression
void expression(int mode){
    if(strcmp(Token[tix+1], "=") == 0)  // [Identifier "="]
        assign();
    else
        equalityExpression(mode);       // addExpression
}


/*
 * 语法分析
 */

/// 数据类型识别
/// TypeSpecifier ::= "void" | "char*" | "int"
char* typeSpecifier(){
    if(!isTypeSpecifier()){
        printf("#Error: Unsupported specifier <%s>\n",Token[tix]);
        exit(1);
    }
    return Token[tix++];  //返回此数据类型
}

/// 变量名或函数名称
/// VarDeclarator ::= Identifier
char* varDeclarator(){
    return Token[tix++];
}

/// VariableDeclaration ::= TypeSpecifier VarDeclarator ["=" Initializer]  ("," VarDeclarator ["=" Initializer])*
/// var like : [ int | char* ] name1 = var1, name2 = var2, ...... ;
/// void 类型未提出变量类型
/// ";" 由 program() 处理
void variableDeclaration(int status){
    char* varType = typeSpecifier();
    do{
        char* varName = varDeclarator();
        if(status == ST_FUNC){  // 函数内定义局部变量
            appendName(1, NM_VAR, varType, varName, AD_STACK, --baseSpace);
            if(is("=")){
                tix--;
                assign();
            }
        }else{                  // 函数外定义全局变量
            appendName(0, NM_VAR, varType, varName, AD_DATA, ixData);
            if(ispp("="))
                DataSection[ixData++] = Token[tix++];
        }
    }while (ispp(","));
}


/// 复合语句
/// CompoundStatement ::= "{" (Statements)* "}"
void compoundStatement(intptr_t locBreak, intptr_t locContinue){
    // 由于只写了quote检查，no bracket pair类型的语法错误只能通过tix<nToken检验
    for(skip("{"); tix<nToken && isTypeSpecifier(); skip(";"))
        variableDeclaration(ST_FUNC);
    while(!ispp("}"))
        statement(locBreak, locContinue);
}


/// IfStatement ::= "if" "(" Expression ")" Statement [ "else" Statement ]
void ifStatement(intptr_t locBreak, intptr_t locContinue){
    intptr_t locElse, locEnd;
    skip("(");
    expression(VAL);
    skip(")");
    outInst2(JZ, IMM, locElse = loc());
    statement(locBreak, locContinue);
    if(is("else"))
        outInst2(JMP, IMM, locEnd = loc());
    outInst2(LABEL, IMM, locElse);
    if(ispp("else")){
        statement(locBreak, locContinue);
        outInst2(LABEL, IMM, locEnd);
    }
}


/// WhileStatement ::= "while" "(" Expression ")" Statement
void whileStatement(intptr_t locBreak, intptr_t locContinue){
    intptr_t locExpr, locNext;
    outInst2(LABEL, IMM, locExpr = loc());
    skip("(");
    expression(VAL);
    skip(")");
    outInst2(JZ, IMM, locNext = loc());
    statement(locNext, locExpr);
    outInst2(JMP, IMM, locExpr);
    outInst2(LABEL, IMM, locNext);
}

/// ReturnStatement ::= "return" [Expression] ";"
void returnStatement() {
    if (!is(";"))
        expression(VAL);
    skip(";");
    outInst(RET);
}

/// Statement ::= CompoundStmt | IfStmt | ReturnStmt| Expression ";" | ";"
void statement(intptr_t locBreak, intptr_t locContinue) {
    if (ispp(";")) ; // null statement
    else if (is("{")) compoundStatement(locBreak, locContinue);
    else if (ispp("if")) ifStatement(locBreak, locContinue);
    else if (ispp("while")) whileStatement(locBreak, locContinue);
    else if (ispp("return")) returnStatement();
    else { expression(VAL); skip(";"); }
}

/// FunctionDefinition ::= TypeSpecifier  VarDeclarator  "(" [ VarDeclaration ("," VarDeclaration)* ] ")" CompoundStatement
void functionDefinition(){
    int locFunc = loc();                //函数标识号(numberLabel自动++）
    char* varType = typeSpecifier();    //return数据类型表示
    char* varName = varDeclarator();    //函数名称
    nLocal = 0;                         //本地变量表计数

    outInst2(FUNC, STR, (intptr_t)varName);
    outInst2(LABEL, IMM, locFunc);
    if(debug){
        printf("#DEBUG: varType <%s>\n",varType);
        printf("#DEBUG: varName <%s>\n",varName);
    }
    if (strcmp(varName, "main") == 0){
        if(debug) printf("#DEBUG: Main entry <%d>\n",nInst);
        entryPoint = nInst;
    }

    appendName(0, NM_FUNC, varType, varName, AD_CODE, locFunc);
    baseSpace = 2;
    outInst(ENTRY); //相当于push ebp; mov ebp,esp
    INSTRUCT* pSub = outInst2(ADDSP, IMM, 0);

    for (skip("("); !ispp(")"); ispp(",")) {
        varType = typeSpecifier();
        varName = varDeclarator();
        appendName(1, NM_VAR, varType, varName, AD_STACK, baseSpace++);
    }

    baseSpace = 0;  //本地变量
    compoundStatement(0, 0);    //函数主体
    pSub->val = baseSpace;  //变更pSub imm值
    if(Inst[nInst-1].opcode != RET)     //固定配置return
        outInst(RET);
}


/// 顶层语法分析
/// Program ::= (FunctionDefinition | VariableDeclaration ";")*
int program(){
    err = 0;
    nInst = 0;
    entryPoint = -1;    //main函数检测
    numLabel = 1;
    for (tix = 0; tix < nToken;) {
        if (isFunctionDefinition()) {
            functionDefinition(); // 函数定义
        } else {
            variableDeclaration(ST_GVAR); // 变量定义
            skip(";");
        }
        if (err)
            return err;
    }
    if(debug) printf("#DEBUG: entryPoint<%d>\n",entryPoint);
    if (entryPoint < 0){
        printf("#Error: no main function\n");   // main函数未找到
        exit(1);
    }
    return 0;
}

/// parser初始化变量计数，putstr和putnum输出方法
int parser() {
    nGlobal = nLocal = 0;
    appendName(0, NM_FUNC, "void", "putstr", AD_CODE, 0);
    appendName(0, NM_FUNC, "void", "putnum", AD_CODE, 0);
    return program();
}


typedef unsigned char *byte_pointer;
// 二进制输出
void formatOutput(FILE* file, byte_pointer start, int size){
    int ttlSize = size * 8;
    int buf[ttlSize];
    memset(buf, 0, sizeof(buf));
    for(int i = 0; i < size; i++){
        for(int j = 0; j < 8; j++){
            buf[ttlSize - (i*8) - j - 1] = (start[i] >> j) & 0x1;
        }
    }
    for(int i = 0; i < ttlSize; i++){
        fprintf(file, "%d", buf[i]);
    }

    putc(' ', file);

}

// 字符串逐字写入
void formatOutputString(FILE* file,const char* str,int len){
    int buf[8];

    for(int i = 0;i<len;i++){
        if(debug)
            printf("#DEBUG: buf:");
        
        for(int j = 0;j < 8; j++)
            buf[7-j] = (str[i] >> j) & 0x1;

        for(int p = 0; p < 8; p++){
            fprintf(file, "%d", buf[p]);
            if(debug)
                printf("%d", buf[p]);
        }
        if(debug)
            printf("\n");
        putc(' ',file);
    }
}

// 写入文件
int writeImage(char* fileName,int param){
    FILE* file = fopen(fileName, "w");
    INSPTR insptr;

    int intSize = sizeof(int);
    int opSize = sizeof(insptr->opcode);
    int typeSize = sizeof(insptr->type);
    int valSize = sizeof(insptr->val);

    // #1 int entryPoint
    formatOutput(file, (byte_pointer)&entryPoint, intSize);

    // #2 int nInst
    formatOutput(file, (byte_pointer)&nInst, intSize);

    // #3 Inst
    for(int i = 0; i<nInst; i++){
        formatOutput(file, (byte_pointer)&Inst[i].opcode, opSize);
        formatOutput(file, (byte_pointer)&Inst[i].type, typeSize);
        if(Inst[i].type == STR){

            //printf("%s\n",(char*)Inst[i].val);
            if(Inst[i].opcode == PUSH){
                //字符串临时输出，不放于DataSection
                fputs("00 ",file);
                char* str = (char*)Inst[i].val;
                int strSize = strlen(str);  // without '\0'

                formatOutput(file, (byte_pointer)&strSize, intSize);    //字符串长度
                formatOutputString(file, str, strSize); //字符串内容

            }else if(strcmp("putstr",(char*)Inst[i].val) == 0){
                //printf("putstr\n");
                int len = strlen("putstr");
                fputs("00 ",file);   //native
                formatOutput(file, (byte_pointer)&len, intSize);
                formatOutputString(file, "putstr", len);
            }else if(strcmp("putnum",(char*)Inst[i].val) == 0){
                //printf("putnum\n");
                int len = strlen("putnum");
                fputs("00 ",file);   //native
                formatOutput(file, (byte_pointer)&len, intSize);
                formatOutputString(file, "putnum", len);
            }else{
                fputs("01 ",file);
                formatOutput(file, (byte_pointer)&Inst[i].val, valSize);
            }
        }else{
            fputs("01 ",file);   //native
            formatOutput(file, (byte_pointer)&Inst[i].val, valSize);
        }

    }

    // #4 int ixData
    formatOutput(file, (byte_pointer)&ixData, intSize);

    for(int i = 0; i<ixData; i++){
        // #5 sizeof(DataSection[i])
        int dsSize = strlen(DataSection[i]);  // without '\0'
        formatOutput(file, (byte_pointer)&dsSize, intSize);

        // #6 DataSection
        // dsSize 未对齐
        formatOutputString(file, DataSection[i], dsSize);
    }

    // #7 param
    formatOutput(file, (byte_pointer)&param, intSize);

    fclose(file);
    return 0;
}


/* main
 * src : src file
 * -d : debug
 * -to : print token
 * -tr : print trace
 * -o : +output file name
 */
int main(int argc, char *argv[]) {
    char *src = NULL;
    char *outFile = NULL;
    int param = 0;

    for (int n = 1; n < argc; n++) {
        if (strcmp(argv[n], "-d") == 0){
            debug = 1;
            printf("#AVSVM: Debug mode on!\n");
        }else if(strcmp(argv[n], "-to") == 0){
            fToken = 1;
            printf("#AVSVM: TokenPrint mode on!\n");
        }else if(strcmp(argv[n], "-tr") == 0){
            fTrace = 1;
            printf("#AVSVM: Trace mode on!\n");
        }else if(strcmp(argv[n], "-o") == 0){
            if(n + 1 < argc){
                outFile = argv[++n];
            }else{
                printf("#AVSVM: No output file!\n");
                exit(1);
            }
        }else if (src == NULL)
            src = argv[n];
        else
            param = atoi(argv[n]);
    }

    if (!src) {
        printf("#AVSVM: Usage: avsvm_slp [-o outputFile][-d][-to][-tr] src.c\n");
        return -1;
    }

    if(debug) printf("#DEBUG: Param:%d\n",param);

    lex(src);
    if(debug) printf("#DEBUG: lex end\n");

    parser();
    if(debug){
        printf("#DEBUG: parser end\n");
        printf("#DEBUG: Instance Count <%d>\n",nInst);
    }
    if (fTrace){
        printf("#Trace:\n");
        for (int n = 0; n < nInst; n++) {
            printInst(n, &Inst[n]);
        }
    }

    //strdup内存泄漏处理
    for(int i = 0; i<nToken; i++){
        free(Token[i]);
    }
    if(outFile!=NULL && writeImage(outFile, param)){
        printf("#Error: fail in writing %s\n",outFile);
    }

    return 0;
}
