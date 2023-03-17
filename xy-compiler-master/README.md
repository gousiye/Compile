# 项目介绍
本项目基本是基于xy-compiler-master项目改写的，增添的功能有：
- 词法层面：支持了int, float的类型识别，浮点数的识别和注释,比较运算的识别
- 语法层面：支持了 `类型 ` +  `变量名`的声明和定义；支持多个变量同时声明（不过没有对应的ast和中间代码生成）；支持if-then-else-endif, while语句的识别； 支持逻辑运算。通过优先级消除了`移进-规约冲突`和`规约-规约`冲突，文法没有二义性。对文法匹配错误的提示做了优化。
- ast方面，新增了if块和while块的ast类
- 中间代码方面，补充了逻辑二元运算，比较运算；补充了if块和while块的中间代码的生成，同时也能能通过JIT运行。

## 我的项目的配置环境
- flex 2.6
- bison 3.2.1
- llvm 14.0.6

## 项目快速运行
- 在`xy-compiler/xy-compiler-master/build/bin`中通过终端命令`cat demo.xy |./compiler`编译并执行demo.xy，显示中间代码和输出结果。

## 项目的完整构建，运行
- 在`xy-compiler/xy-compiler-master`目录下运行`./run.sh`，会在该目录下生成`build`文件夹，进入`build/bin`中，通过命令`cat demo.xy |./compiler`执行对demo.xy的编译，将会生成对应的中间代码和运行结果。
- 如果初始没有`build`文件夹，第一次运行`./run.sh`，会从头构建整个项目，可能出现`无法访问https://github.com/jarro2783/cxxopts.git/'：Failed to connect to 127.0.0.1 port 7890: 拒绝连接错误`，可能需要翻墙或者换源，国内git访问不是很稳定。
- 如果初始有`build`文件夹的话, 此时已经有了`cxxopts.git`的相关日志，不会报错。但是此时`run.sh`中对应的是build的绝对路径，不同机器上不同绝对路径下`./run.sh`出现路径错误，不能直接./run.sh更新项目的源码，但是可以在`./build/bin/xy-compiler`中正常使用xy-compiler。如果是在本地从头构建的build,可以修改项目源码后./run.sh来更新项目，生成新的xy-compiler。
- 初始是有`build`文件夹


## 项目原本的细节改动
- 在原项目gen.cpp的`generateCode()`中的`ReturnInst::Create(getGlobalContext(), currentBlock());`的位置是错误的，应该在`popBlock();`之前执行，否则`ret void`会出现在`entry`块中，此时如果有跳转语句的话运行会出现段错误。
- 原项目的作用域也有点问题，默认每个baiscBlock是一个作用域，嵌套的block不会继承外层的变量，本项目是通过每个basicBlock先读取到外层的变量添加到自己的变量名map中来解决的。
- if, while的中间代码的生成是完全添加的，在gen.cpp中基本每一步都有详细的注释说明。
- 对`syntactic.y`的分析进行了优化，比原本的项目要更清晰


## 项目存在的问题
- 不能给变量赋负值（原项目自身存在的问题）
- if 和 while 语句不支持嵌套，但是可以无嵌套写多个if,while
- 数据类型的考虑比较少，虽然有float，但中间代码基本还只涉及int类型
- 原项目的结构中默认是有一个main的，如果在demo.xy中自己添加一个main，会变成`main1`，并非真正的`main` 
