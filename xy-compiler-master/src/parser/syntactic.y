%{
    #include "ast.h"
    #include <algorithm>
    #include <queue>
    #include <cstdio>
    #include <cstdlib>
    #include <iostream>
    using namespace std;

    extern int yylex();
    void yyerror(const char *s) { 
      extern char* yytext;
      extern int yylineno;
      printf("\n%s at line %d: '%s'  \n",s,yylineno,yytext);
    }
    NBlock *programBlock;
%}

/* Represents the many different ways we can access our data */


%define parse.error verbose
%union {
Node *node;
    NBlock *block;
    NExpression *expr;
    NStatement *stmt;
    NIdentifier *ident;
    std::vector<NIdentifier*> *varvec;
    std::vector<NExpression*> *exprvec;
    std::string *string;
    int token;
}

/* Define our terminal symbols (tokens). This should
   match our tokens.l lex file. We also define the node type
   they represent.
 */


%token <string> TIDENTIFIER TINTEGER TLFOATNUMBER 
%token <token> TINT TFLOAT TIF TWHILE SGT SLT SGE SLE
%token <token> TOR TAND TNOT  TELSE
%token <token> TPLUS TMINUS TMUL TDIV
%token <token> YUNDIF
%token <token> TCEQ TCNE TEQUAL
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TCOMMA TSEMICOLON
%token <token> TRETURN TEXTERN
%token <node> TDISCRIPTION1 TDISCRIPTION2 First Last

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (NIdentifier*). It makes the compiler happy.
 */
%type <ident> ident 
%type <expr> def_ident expr value_expr numeric
%type <varvec> func_decl_args
%type <exprvec>  call_args  //recursive_ident
%type <block> program stmts block 
%type <stmt> stmt func_decl  extern_decl control while
%type <token> relop form

%left Last

%left TCOMMA
%left TOR
%left TAND
%left TCEQ TCNE
%left SGE SGT SLE SLT
%left TPLUS TMINUS
%left TMUL TDIV
%left TLPAREN TRPAREN TNOT
%left TLBRACE

%left First

%right TEQUAL 



%start program



%%

program:
  stmts { programBlock = $1;  std::cout<<"syntactic parse successfully!"<<std::endl;}
;

stmts: 
  stmt { $$ = new NBlock(); $$->statements.push_back($1); }
| stmts stmt { $1->statements.push_back($2); }
;  

stmt:
  func_decl     {}  
| control       {}
| while     {}
| extern_decl    {}
| expr TSEMICOLON    { $$ = new NExpressionStatement(*$1); }
| TRETURN value_expr TSEMICOLON   { $$ = new NReturnStatement(*$2); }
/*
-- | TFORM TIDENTIFIER TEQUAL numeric recursive_ident TSEMICOLON 
-- | TFORM TIDENTIFIER TEQUAL ident recursive_ident TSEMICOLON 
-- | TFORM TIDENTIFIER TEQUAL ident TLPAREN call_args TRPAREN  recursive_ident TSEMICOLON 
-- | TFORM TIDENTIFIER TEQUAL ident TLPAREN TRPAREN  recursive_ident TSEMICOLON 
-- | TFORM TIDENTIFIER  recursive_ident TSEMICOLON 
*/
;

form:
  TINT
| TFLOAT
;

block:
  TLBRACE stmts TRBRACE  {$$ = $2;}
| TLBRACE TRBRACE    {$$ = new NBlock();}
;

extern_decl:
  TEXTERN ident TLPAREN func_decl_args TRPAREN 
  { $$ = new NExternDeclaration(*$2, *$4); delete $4; }
;

func_decl:
  form ident TLPAREN func_decl_args TRPAREN block 
  { $$ = new NFunctionDeclaration($1, *$2, *$4, *$6); delete $4; }
| form ident TLPAREN TRPAREN block  
  { $$ = new NFunctionDeclaration($1, *$2, *$5);}
;

func_decl_args:
  form ident
  { $$ = new VariableList(); NIdentifier* varible = new NIdentifier(*$2);
  varible->type = $1;  
    $$->push_back(varible); }
| func_decl_args TCOMMA form TIDENTIFIER   
  {
    NIdentifier* varible = new NIdentifier(*$4);
    varible->type = $3;
    $1->push_back(varible);
  }
; 


//if 对应的文法，可以选择有else，if,else后面可以只跟一条语句，或者花括号的块

control:
 TIF TLPAREN expr TRPAREN block {$$ = new NIF(*$3, *$5);}
|TIF TLPAREN expr TRPAREN stmt {
  NBlock * stat = new NBlock();
  stat->statements.push_back($5);
  $$ = new NIF(*$3, *stat);}
| TIF TLPAREN expr TRPAREN block TELSE block {$$ = new NIF(*$3, *$5,*$7);}
| TIF TLPAREN expr TRPAREN block TELSE stmt{
  NBlock * stat = new NBlock();
  stat->statements.push_back($7);
  $$ = new NIF(*$3, *$5, *stat);} 
| TIF TLPAREN expr TRPAREN stmt TELSE block{
  NBlock * stat = new NBlock();
  stat->statements.push_back($5);
  $$ = new NIF(*$3, *stat, *$7);}
|TIF TLPAREN expr TRPAREN stmt TELSE stmt{
  NBlock * stat1 = new NBlock();
  stat1->statements.push_back($5);
  NBlock * stat2 = new NBlock();
  stat2->statements.push_back($7);
  $$ = new NIF(*$3, *stat1, *stat2);}
;

while:
 TWHILE TLPAREN expr TRPAREN block {$$ = new NWHILE(*$3, *$5);}
|TWHILE TLPAREN expr TRPAREN stmt {
     NBlock * stat = new NBlock();
     stat->statements.push_back($5);
     $$ = new NWHILE(*$3, *stat);
}
;


// int a,b = 2, c = sum(2,4)这种功能只在syntactic层面实现了，后面的ast和中间代码都没有涉及，这里给注释了
/*
-- recursive_ident:  
--    TCOMMA ident 
-- |  TCOMMA ident recursive_ident 
-- |  TCOMMA def_ident  
-- |  TCOMMA def_ident recursive_ident  
*/
 
ident:
  TIDENTIFIER    { $$ = new NIdentifier(*$1); delete $1; }
;

numeric:
  TINTEGER     { $$ = new NInteger(atol($1->c_str())); delete $1; }
| TLFOATNUMBER    { $$ = new NFloat(atol($1->c_str())); delete $1; }
;

//把原来的表达式拆成了值表达式和定义表达式

expr:
value_expr %prec Last
| def_ident
;

//把大于小于这些拆开了，方便中间代码的生成
relop:
SGT
| SGE
| SLT
| SLE
;

value_expr:
ident TLPAREN call_args TRPAREN {$$ = new NMethodCall(*$1,*$3);}
| ident TLPAREN TRPAREN {$$ = new NMethodCall(*$1);}
| ident {$$ = $1;}
| numeric {$$ = $1;}
| value_expr TMUL value_expr   { $$ = new NBinaryOperator(*$1, $2, *$3); }
| value_expr TDIV value_expr   { $$ = new NBinaryOperator(*$1, $2, *$3); }
| value_expr TPLUS value_expr  { $$ = new NBinaryOperator(*$1, $2, *$3); }
| value_expr TMINUS value_expr  { $$ = new NBinaryOperator(*$1, $2, *$3); }
| value_expr TCEQ value_expr  %prec First  { $$ = new NBinaryOperator(*$1, $2, *$3); }
| value_expr TCNE value_expr  %prec First   { $$ = new NBinaryOperator(*$1, $2, *$3); }
| value_expr relop value_expr  %prec First  { $$ = new NBinaryOperator(*$1, $2, *$3); }
| value_expr TAND value_expr  %prec First   { $$ = new NBinaryOperator(*$1, $2, *$3); }
| value_expr TOR value_expr  %prec First   { $$ = new NBinaryOperator(*$1, $2, *$3); }
| TNOT value_expr {$$ = new NUnaryOperator ($1, *$2);}
| TLPAREN value_expr TRPAREN  {$$ = $2;}
;

def_ident :
  ident TEQUAL expr  {$$ = new NAssignment(*$1,*$3);}
| form TIDENTIFIER {$$ = new NIdentifier($1, *$2); } 
| form TIDENTIFIER TEQUAL value_expr
  {  NIdentifier *varible = new NIdentifier($1,*$2);
     $$ = new NAssignment(*varible, *$4); }
;


call_args:
expr { $$ = new ExpressionList(); $$->push_back($1); }
| call_args TCOMMA expr  { $1->push_back($3); }



%%