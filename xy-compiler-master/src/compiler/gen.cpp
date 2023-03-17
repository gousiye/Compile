#include "gen.h"
#include "ast.h"
#include "syntactic.hpp"

#include <exception>

#include <llvm/ADT/Twine.h>
#include <llvm/IR/LegacyPassManager.h>

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>


using namespace std;

LLVMContext TheContext;




/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root) {
  std::cout << "Generating code...\n";

  /* Create the top level interpreter function to call as entry */
  vector<Type*> argTypes;
  FunctionType* ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()),
    makeArrayRef(argTypes), false);
  mainFunction =
    Function::Create(ftype, GlobalValue::InternalLinkage, "main", module);
  BasicBlock* bblock =
    BasicBlock::Create(getGlobalContext(), "entry", mainFunction, 0);
  builder.SetInsertPoint(bblock);
  /* Push a new variable/block context */
  pushBlock(bblock);

  root.codeGen(*this); /* emit bytecode for the toplevel block */
  //这里要先给return再pop，否则如果有if then endif的话，ret void不是在endif块中，而是仍然在main中，这会导致运行错误

  ReturnInst::Create(getGlobalContext(), currentBlock());
  popBlock();


  /* Print the bytecode in a human-readable format
     to see if our program compiled properly
   */
  std::cout << "Code is generated.\n";

#if LLVM_VERSION_MAJOR < 10
  PassManager<Module> pm;
  AnalysisManager<Module> am;
  pm.addPass(PrintModulePass(outs()));
  pm.run(*module, am);
#else
  llvm::legacy::PassManager pm;
  pm.add(createPrintModulePass(outs()));
  pm.run(*(this->module));
#endif
}

/* Executes the AST by running the main function */
GenericValue CodeGenContext::runCode() {
  try {
    std::cout << "Running code:\n";
    ExecutionEngine* ee = EngineBuilder(unique_ptr<Module>(module)).create();
    ee->finalizeObject();
    vector<GenericValue> noargs;
    GenericValue v = ee->runFunction(mainFunction, noargs);
    delete ee;
    return v;
  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
  }
}

/* -- Code Generation -- */

Value* NInteger::codeGen(CodeGenContext& context) {
  std::cout << "Creating integer: " << value << endl;
  return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value, true);
}

Value* NIdentifier::codeGen(CodeGenContext& context) {
  std::cout << "Creating identifier reference: " << name << endl;
  if (context.locals().find(name) == context.locals().end()) {
    std::cout << "undeclared variable " << name << endl;
    std::cout << "Creating variable declaration " << name << endl;
    AllocaInst* alloc = nullptr;
    //增加了float类型
    if (type == TINT)
      alloc = new AllocaInst(Type::getInt64Ty(getGlobalContext()), 0u,
        name.c_str(), context.currentBlock());
    else if (type == TFLOAT)
      alloc = new AllocaInst(Type::getFloatTy(getGlobalContext()), 0u,
        name.c_str(), context.currentBlock());
    context.locals()[name] = alloc;
  }

#if LLVM_VERSION_MAJOR >= 12
  const auto& local = context.locals()[name];
  return new LoadInst(cast<PointerType>(local->getType())->getElementType(),
    local, llvm::Twine(""), false, context.currentBlock());
#else
  return new LoadInst(context.locals()[name], "", false,
    context.currentBlock());
#endif
}

Value* NMethodCall::codeGen(CodeGenContext& context) {
  Function* function = context.module->getFunction(id.name.c_str());
  if (function == NULL) {
    std::cerr << "no such function " << id.name << endl;
  }
  std::vector<Value*> args;
  ExpressionList::const_iterator it;
  for (it = arguments.begin(); it != arguments.end(); it++) {
    args.push_back((**it).codeGen(context));
  }
  CallInst* call = CallInst::Create(function, makeArrayRef(args), "",
    context.currentBlock());
  std::cout << "Creating method call: " << id.name << endl;
  return call;
}

Value* NBinaryOperator::codeGen(CodeGenContext& context) {
  std::cout << "Creating binary operation " << op << endl;
  Instruction::BinaryOps instr;
  Instruction::OtherOps icmp;
  //这里增加了逻辑运算和比较运算
  CmpInst::Predicate pred;
  switch (op) {
  case TPLUS:    //加
    instr = Instruction::Add;
    goto math;
  case TMINUS:     //减
    instr = Instruction::Sub;
    goto math;
  case TMUL:     //乘
    instr = Instruction::Mul;
    goto math;
  case TDIV:    //除
    instr = Instruction::SDiv;
    goto math;
  case TAND:    //和
    instr = Instruction::And;
    goto math;
  case TOR:    //或
    instr = Instruction::And;
    goto math;

    //大于这些不是二元运算，是算在其它运算中的
  case SGT:    //大于
    icmp = Instruction::ICmp;
    pred = CmpInst::Predicate::ICMP_SGT;
    goto logic;
  case SGE:    //大于等于
    icmp = Instruction::ICmp;
    pred = CmpInst::Predicate::ICMP_SGE;
    goto logic;
  case SLT:    //大于
    icmp = Instruction::ICmp;
    pred = CmpInst::Predicate::ICMP_SLT;
    goto logic;
  case SLE:    //小于等于
    icmp = Instruction::ICmp;
    pred = CmpInst::Predicate::ICMP_SLE;
    goto logic;
  case TCEQ:    //等于
    icmp = Instruction::ICmp;
    pred = CmpInst::Predicate::ICMP_EQ;
    goto logic;
  case TCNE:    //不等于
    icmp = Instruction::ICmp;
    pred = CmpInst::Predicate::ICMP_NE;
    goto logic;
  }
  return NULL;
math:
  return BinaryOperator::Create(instr, lhs.codeGen(context),
    rhs.codeGen(context), "",
    context.currentBlock());
  //如果是逻辑运算返回逻辑运算的Vlaue*
logic:
  return CmpInst::Create(icmp, pred, lhs.codeGen(context),
    rhs.codeGen(context), "",
    context.currentBlock());
}



Value* NAssignment::codeGen(CodeGenContext& context) {
  std::cout << "Creating assignment for " << lhs.name << endl;
  if (context.locals().count(lhs.name) == 0) {
    std::cout << "undeclared variable " << lhs.name << endl;
    std::cout << "Creating variable declaration " << lhs.name << endl;
    AllocaInst* alloc =
      new AllocaInst(Type::getInt64Ty(getGlobalContext()), 0u,
        lhs.name.c_str(), context.currentBlock());
    context.locals()[lhs.name] = alloc;
  }

  return new StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false,
    context.currentBlock());

  // const auto& local = context.locals()[lhs.name];
  // return new LoadInst(cast<PointerType>(local->getType())->getElementType(),
  //   local, llvm::Twine(""), false, context.currentBlock());
}

Value* NBlock::codeGen(CodeGenContext& context) {
  StatementList::const_iterator it;
  Value* last = NULL;
  for (it = statements.begin(); it != statements.end(); it++) {
    std::cout << "Generating code for " << typeid(**it).name() << endl;
    last = (**it).codeGen(context);
  }
  std::cout << "Creating block" << endl;
  return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context) {
  std::cout << "Generating code for " << typeid(expression).name() << endl;
  return expression.codeGen(context);
}

Value* NReturnStatement::codeGen(CodeGenContext& context) {
  std::cout << "Generating return code for " << typeid(expression).name()
    << endl;
  Value* returnValue = expression.codeGen(context);
  context.setCurrentReturnValue(returnValue);
  return returnValue;
}

Value* NExternDeclaration::codeGen(CodeGenContext& context) {
  vector<Type*> argTypes;
  VariableList::const_iterator it;
  for (it = arguments.begin(); it != arguments.end(); it++) {
    argTypes.push_back(Type::getInt64Ty(getGlobalContext()));
  }
  FunctionType* ftype = FunctionType::get(Type::getInt64Ty(getGlobalContext()),
    makeArrayRef(argTypes), false);
  Function* function = Function::Create(ftype, GlobalValue::ExternalLinkage,
    id.name.c_str(), context.module);

  return function;
}

Value* NFunctionDeclaration::codeGen(CodeGenContext& context) {
  vector<Type*> argTypes;
  VariableList::const_iterator it;
  for (it = arguments.begin(); it != arguments.end(); it++) {
    argTypes.push_back(Type::getInt64Ty(getGlobalContext()));
  }
  FunctionType* ftype = FunctionType::get(Type::getInt64Ty(getGlobalContext()),
    makeArrayRef(argTypes), false);
  Function* function = Function::Create(ftype, GlobalValue::InternalLinkage,
    id.name.c_str(), context.module);
  BasicBlock* bblock =
    BasicBlock::Create(getGlobalContext(), "entry", function, 0);
  context.builder.SetInsertPoint(bblock);
  context.pushBlock(bblock);


  Function::arg_iterator argsValues = function->arg_begin();
  Value* argumentValue;

  for (it = arguments.begin(); it != arguments.end(); it++) {
    (**it).codeGen(context);

    argumentValue = &*argsValues++;
    argumentValue->setName((*it)->name.c_str());
    StoreInst* inst = new StoreInst(
      argumentValue, context.locals()[(*it)->name], false, bblock);
  }

  block.codeGen(context);
  ReturnInst::Create(getGlobalContext(), context.getCurrentReturnValue(),
    bblock);

  context.popBlock();

  std::cout << "Creating function: " << id.name << endl;
  return function;
}



//添加的if-then-else-endif块

Value* NIF::codeGen(CodeGenContext& context) {
  cout << "Generating if statement" << endl;
  Value* cond_value = this->condition.codeGen(context);
  // //条件不能为空
  if (!cond_value)
    return nullptr;
  //   获取到当前if所在的函数
  Function* theFunction = context.currentBlock()->getParent();
  //  true_block, false_block对应的代码块，true_block指父函数，else块, merge块因为不是先处理的，没有父函数
  BasicBlock* thenBB = BasicBlock::Create(getGlobalContext(), "then", theFunction, 0);
  BasicBlock* falseBB = BasicBlock::Create(getGlobalContext(), "else");

  //  if 结束后的合并分支块
  BasicBlock* mergeBB = BasicBlock::Create(getGlobalContext(), "endif");

  // 记录这个if-then-endif块的上一级的块中已经声明的那些变量，上一级的变量在这里应该也能找到
  std::map<std::string, Value*> temp_memo = context.locals();

  //如果有else
  if (false_block)
    //创建中间代码的跳转转移指令，为真是then块，为假是else块
    BranchInst* jmp = BranchInst::Create(thenBB, falseBB, cond_value, context.currentBlock());
  else
    //中间代码的跳转转移指令，为真是then块，为假是merge块
    BranchInst* jmp = BranchInst::Create(thenBB, mergeBB, cond_value, context.currentBlock());

  //现在处理的块是then块
  context.pushBlock(thenBB);
  //将父级的变量传递给then，保证作用域正确
  context.locals() = temp_memo;
  //递归调用生成then块的中间代码
  Value* then_val = this->true_block->codeGen(context);
  Value* else_val = nullptr;
  //then块结束跳转到merge块
  BranchInst::Create(mergeBB, thenBB);
  //then块处理完毕
  context.popBlock();

  //如果有else块，处理else块
  if (false_block) {
    //设置else块的父函数
    theFunction->getBasicBlockList().push_back(falseBB);
    //这里的步骤同then块同样
    context.pushBlock(falseBB);
    context.locals() = temp_memo;
    else_val = this->false_block->codeGen(context);
    BranchInst::Create(mergeBB, falseBB);
    context.popBlock();
  }
  //设置merge块的父函数
  theFunction->getBasicBlockList().push_back(mergeBB);
  //后面的中间代码都是在merge块中生成，处理的是merge块
  context.pushBlock(mergeBB);
  //merge块的变量继承上级代码块
  context.locals() = temp_memo;
  return nullptr;
}

//while的中间代码
Value* NWHILE::codeGen(CodeGenContext& context) {
  cout << "Generating while statement" << endl;

  std::map<std::string, Value*> temp_memo = context.locals();

  Function* theFunction = context.currentBlock()->getParent();
  //这里cond, loop, endloop三个块
  BasicBlock* condBB = BasicBlock::Create(getGlobalContext(), "cond", theFunction, 0);
  BasicBlock* loopBB = BasicBlock::Create(getGlobalContext(), "loop");
  BasicBlock* endBB = BasicBlock::Create(getGlobalContext(), "endloop");
  context.pushBlock(condBB);
  context.locals() = temp_memo;
  Value* cond_value = this->condition.codeGen(context);
  BranchInst* jmp = BranchInst::Create(loopBB, endBB, cond_value, condBB);
  context.popBlock();

  //主块要进入cond块
  BranchInst::Create(condBB, context.currentBlock());

  //循环体不为空
  if (block == nullptr)
    return nullptr;


  //类似if的中间代码生成
  theFunction->getBasicBlockList().push_back(loopBB);
  context.pushBlock(loopBB);
  context.locals() = temp_memo;
  Value* loop_val = this->block->codeGen(context);
  BranchInst::Create(condBB, loopBB);
  context.popBlock();

  theFunction->getBasicBlockList().push_back(endBB);
  context.pushBlock(endBB);
  context.locals() = temp_memo;
  return nullptr;
}












