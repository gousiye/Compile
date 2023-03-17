#include <iostream>
#include <string>
#include <vector>

#include <llvm/IR/Value.h>

using namespace std;

class CodeGenContext;
class NStatement;
class NExpression;
class NIdentifier;

typedef std::vector<NStatement*> StatementList;
typedef std::vector<NExpression*> ExpressionList;
typedef std::vector<NIdentifier*> VariableList;

//基础节点
class Node {
public:
    virtual llvm::Value* codeGen(CodeGenContext& context) { return NULL; }
};

//一个表达式
class NExpression: public Node {};

//一条语句
class NStatement: public Node {};

//整数
class NInteger: public NExpression {
public:
    long long value;
    NInteger(long long value): value(value) {}
    virtual llvm::Value* codeGen(CodeGenContext& context);
};

//float
class NFloat: public NExpression {
public:
    double value;
    NFloat(double value): value(value) {}
    virtual llvm::Value* codeGen(CodeGenContext& context) { return NULL; }
};


//标识符
class NIdentifier: public NExpression {
public:
    int type = -10;
    std::string name;
    NIdentifier(int& type, const std::string& name): name(name), type(type) {}
    NIdentifier(const std::string& name): name(name) {}
    //NIdentifier(const NIdentifier& other) { *this = other; }
    virtual llvm::Value* codeGen(CodeGenContext& context);
};

//方法调用，包括函数名和参数列表
class NMethodCall: public NExpression {
public:
    const NIdentifier& id;
    ExpressionList arguments;
    NMethodCall(const NIdentifier& id, ExpressionList& arguments)
        : id(id), arguments(arguments) {}
    NMethodCall(const NIdentifier& id): id(id) {}
    virtual llvm::Value* codeGen(CodeGenContext& context);
};


//一元运算符
class NUnaryOperator: public NExpression {
public:
    int op;
    NExpression& rhs;
    NUnaryOperator(int op, NExpression& rhs)
        : rhs(rhs), op(op) {}
    virtual llvm::Value* codeGen(CodeGenContext& context) { return NULL; }
};



//二元运算符
class NBinaryOperator: public NExpression {
public:
    int op;
    NExpression& lhs;
    NExpression& rhs;
    NBinaryOperator(NExpression& lhs, int op, NExpression& rhs)
        : lhs(lhs), rhs(rhs), op(op) {}
    virtual llvm::Value* codeGen(CodeGenContext& context);
};

// //逻辑运算符 中间代码可以同上面一起生成，不用拆分
// class NLogisticOperator: public NExpression {
// public:
//     int op;
//     NExpression& lhs;
//     NExpression& rhs;
//     NBinaryOperator(NExpression& lhs, int op, NExpression& rhs)
//         : lhs(lhs), rhs(rhs), op(op) {}
//     //virtual llvm::Value* codeGen(CodeGenContext& context);
// };

//二元关系运算符
class NRelopOperator: public NExpression {
public:
    int op;
    NExpression& lhs;
    NExpression& rhs;
    NRelopOperator(NExpression& lhs, int op, NExpression& rhs)
        : lhs(lhs), rhs(rhs), op(op) {}
    virtual llvm::Value* codeGen(CodeGenContext& context);
};


//赋值运算符
class NAssignment: public NExpression {
public:
    NIdentifier& lhs;
    NExpression& rhs;
    NAssignment(NIdentifier& lhs, NExpression& rhs): lhs(lhs), rhs(rhs) {}
    virtual llvm::Value* codeGen(CodeGenContext& context);
};

//代码块
class NBlock: public NExpression {
public:
    //有多个语句
    StatementList statements;
    NBlock() {}
    virtual llvm::Value* codeGen(CodeGenContext& context);
};

//表达式语句
class NExpressionStatement: public NStatement {
public:
    NExpression& expression;
    NExpressionStatement(NExpression& expression): expression(expression) {}
    virtual llvm::Value* codeGen(CodeGenContext& context);
};

//return语句
class NReturnStatement: public NStatement {
public:
    NExpression& expression;
    NReturnStatement(NExpression& expression): expression(expression) {}
    virtual llvm::Value* codeGen(CodeGenContext& context);
};

//extern 声明
class NExternDeclaration: public NStatement {
public:
    const NIdentifier& id;
    VariableList arguments;
    NExternDeclaration(const NIdentifier& id, const VariableList& arguments)
        : id(id), arguments(arguments) {}
    virtual llvm::Value* codeGen(CodeGenContext& context);
};

//函数定义
class NFunctionDeclaration: public NStatement {
public:
    const int& type;
    const NIdentifier& id;
    VariableList arguments;
    NBlock& block;
    NFunctionDeclaration(const int& type, const NIdentifier& id, const VariableList& arguments,
        NBlock& block)
        :type(type), id(id), arguments(arguments), block(block) {}
    NFunctionDeclaration(const int& type, const NIdentifier& id, NBlock& block)
        :type(type), id(id), block(block) {}
    virtual llvm::Value* codeGen(CodeGenContext& context);
};

//IF控制语句
class NIF: public NStatement {
public:
    NBlock* true_block = nullptr;
    NBlock* false_block = nullptr;
    NExpression& condition;

    // 条件是必要的，false_block是选择可有的
    NIF(NExpression& expression, NBlock& true_block)
        :condition(expression), true_block(&true_block) {}

    NIF(NExpression& expression, NBlock& true_block, NBlock& false_block)
        :condition(expression), true_block(&true_block), false_block(&false_block) {}

    virtual llvm::Value* codeGen(CodeGenContext& context);
};



//While语句
class NWHILE: public NStatement {
public:
    NBlock* block = nullptr;
    NExpression& condition;

    // 条件是必要的
    NWHILE(NExpression& expression, NBlock& block)
        :condition(expression), block(&block) {}
    virtual llvm::Value* codeGen(CodeGenContext& context);
};


