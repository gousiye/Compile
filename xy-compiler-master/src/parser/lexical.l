%{
#include "ast.h"
#include <string>
#include "syntactic.hpp"
#define SAVE_TOKEN  yylval.string = new std::string(yytext, yyleng)
#define TOKEN(t)    (yylval.token = t)

%}

%option noyywrap
%option caseless
%option yylineno

%%


[ \t\n]                 ;
"extern"                return TOKEN(TEXTERN);
"return"                return TOKEN(TRETURN);

"int"                   return TOKEN(TINT);
"float"                 return TOKEN(TFLOAT);
"if"                    return TOKEN(TIF);
"else"                  return TOKEN(TELSE);
"while"                 return TOKEN(TWHILE);

"||"                    return TOKEN(TOR);
"&&"                    return TOKEN(TAND);
"!"                     return TOKEN(TNOT);

[a-zA-Z_][a-zA-Z0-9_]*  SAVE_TOKEN; return TIDENTIFIER;
[0-9]+                  SAVE_TOKEN; return TINTEGER;
[-+]?([0-9]*[.])?[0-9]+([eE][-+]?[0-9]+)?        SAVE_TOKEN;  return TLFOATNUMBER;
">"                     return TOKEN(SGT);
"<"                     return TOKEN(SLT);
">="                    return TOKEN(SGE);
"<="                    return TOKEN(SLE);
";"                     return TOKEN(TSEMICOLON);

"="                     return TOKEN(TEQUAL);
"=="                    return TOKEN(TCEQ);
"!="                    return TOKEN(TCNE);

"("                     return TOKEN(TLPAREN);
")"                     return TOKEN(TRPAREN);
"{"                     return TOKEN(TLBRACE);
"}"                     return TOKEN(TRBRACE);

","                     return TOKEN(TCOMMA);

"+"                     return TOKEN(TPLUS);
"-"                     return TOKEN(TMINUS);
"*"                     return TOKEN(TMUL);
"/"                     return TOKEN(TDIV);

.                       printf("Unknown token!\n"); yyterminate();
"/*"([^*]|\*+[^*/])*\*+"/"    return TOKEN(TDISCRIPTION1);
"//"([^"\n"])*+"\n"       return TOKEN(TDISCRIPTION2);

%%