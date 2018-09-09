%{

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "mstring.h"
#include "jjvalue.h"
#include "ast.h"

#include "yystype.h"

#include "tree_dump.h"

extern int yylex(void);
extern void yyerror(const char *err);

ast_node_base_t *glob_ast = NULL;

%}

%token TK_FUNCTION TK_TAKES TK_RETURNS TK_BEGIN TK_END TK_IF TK_THEN
%token TK_ELSE TK_WHILE TK_FOR TK_TO 
%token TK_ID
%token TK_NUM_INT TK_NUM_FLOAT TK_STR
%token TK_SYM_COMMA TK_SYM_SEMI TK_SYM_DOT
%token TK_SYM_LBRACKET TK_SYM_RBRACKET
%token TK_ESYM_EQ TK_ESYM_EQEQ TK_ESYM_PLUS TK_ESYM_MINUS TK_ESYM_ASTER
%token TK_ESYM_AMP TK_ESYM_PIPE TK_ESYM_AMPAMP TK_ESYM_PIPEPIPE
%token TK_ESYM_CARET TK_ESYM_SLASH TK_ESYM_PERCENT TK_ESYM_LPAREN 
%token TK_ESYM_RPAREN TK_ESYM_LBRACE TK_ESYM_RBRACE TK_ESYM_LT
%token TK_ESYM_GT TK_ESYM_NOT TK_ESYM_NEQ TK_ESYM_LEQ TK_ESYM_GEQ

%%

program: functions { glob_ast = $1.ast; $$ = $1; } ;

functions: 
  functions function 
  {
    $$.ast = $1.ast;
    ast_list_append($$.ast, $2.ast);
  }
  | { $$.ast = node_list(ANS_FUNCTIONS); } ;

function: 
  TK_FUNCTION TK_ID TK_TAKES id_list TK_RETURNS id_list function_body
  {
    $$.ast = node3_wdata(ANS_FUNCTION, $2.token.val, $4.ast, $6.ast,
                         $7.ast);
  } 
  ;

function_body: 
  TK_BEGIN statements TK_END TK_FUNCTION
  {
    $$.ast = node1(ANS_FUNCTION_BODY, $2.ast);
  }
  ;

statements:
  statements statement
  {
    $$.ast = $1.ast;
    ast_list_append($$.ast, $2.ast);
  }
  | { $$.ast = node_list(ANS_STATEMENTS); }
  ;

statement: expr_statement { $$ = $1; } 
           | if_statement { $$ = $1; } 
           | while_statement { $$ = $1; } 
           | for_statement { $$ = $1; } 
           | empty_statement { $$ = $1; } ;

expr_statement: expr TK_SYM_SEMI { $$ = $1; } ;

if_statement: 
  TK_IF expr TK_THEN statements TK_END TK_IF
  {
    $$.ast = node2(ANS_IF, $2.ast, $4.ast);
  }
  | TK_IF expr TK_THEN statements TK_ELSE statements TK_END TK_IF
  {
    $$.ast = node3(ANS_IF, $2.ast, $4.ast, $6.ast);
  }
  ;

while_statement: TK_WHILE expr TK_THEN statements TK_END TK_WHILE
  {
    $$.ast = node2(ANS_WHILE, $2.ast, $4.ast);
  }
  ;

for_statement: 
  TK_FOR idref_expr TK_ESYM_EQ int_expr TK_TO int_expr TK_THEN 
  statements
  TK_END TK_FOR
  {
    ast_node_base_t *for_head = 
      node3(ANS_FOR_HEAD, $2.ast, $4.ast, $6.ast);
    $$.ast = node2(ANS_FOR, for_head, $8.ast);
  }
  ;

expr: assign_expr { $$ = $1; } ;

assign_expr:
  assign_expr TK_ESYM_EQ logic_expr
  {
    jjvalue_t t;
    t.ivalue = TK_ESYM_EQ;
    $$.ast = node2_wdata(ANS_BINEXPR, t, $1.ast, $3.ast);
  }
  | logic_expr { $$ = $1; } 
  ;

logic_expr:
  logic_expr bin_logicop rel_expr
  {
    jjvalue_t t;
    t.ivalue = $2.token.token_kind;
    $$.ast = node2_wdata(ANS_BINEXPR, t, $1.ast, $3.ast);
  }
  | rel_expr { $$ = $1; }
  ;
  

rel_expr:
  rel_expr bin_relop add_expr
  {
    jjvalue_t t;
    t.ivalue = $2.token.token_kind;
    $$.ast = node2_wdata(ANS_BINEXPR, t, $1.ast, $3.ast);
  }
  | add_expr { $$ = $1; }
  ;

add_expr: 
  add_expr bin_addop mul_expr
  {
    jjvalue_t t;
    t.ivalue = $2.token.token_kind;
    $$.ast = node2_wdata(ANS_BINEXPR, t, $1.ast, $3.ast); 
  }
  | mul_expr { $$ = $1; } 
  ;

mul_expr:
  mul_expr bin_mulop unary_expr
  {
    jjvalue_t t;
    t.ivalue = $2.token.token_kind;
    $$.ast = node2_wdata(ANS_BINEXPR, t, $1.ast, $3.ast);
  }
  | unary_expr { $$ = $1; }
  ;

unary_expr:
  atom_expr
  // { $$.ast = node2(ANS_UNARYEXPR, $1.ast, $2.ast); }
  { $$.ast = $1.ast; }
  ;
/*
unary_op_chain:
  unary_op_chain unary_op
  {
    jjvalue_t t;
    t.ivalue = $2.token.token_kind;
    ast_node_base_t *tnode = node1_wdata(ANS_UNARYOP, t, $1.ast);
    ast_list_append($$.ast, tnode);
  }
  | { $$.ast = node_list(ANS_UNARYOPS); };
*/

bin_logicop: TK_ESYM_AMPAMP { $$ = $1; }
             | TK_ESYM_PIPEPIPE { $$ = $1; } 
             ;

bin_relop: TK_ESYM_LT { $$ = $1; }
           | TK_ESYM_GT { $$ = $1; }
           | TK_ESYM_EQEQ { $$ = $1; }
           | TK_ESYM_GEQ { $$ = $1; }
           | TK_ESYM_LEQ { $$ = $1; }
           | TK_ESYM_NEQ { $$ = $1; } 
           ;

bin_mulop: TK_ESYM_ASTER { $$ = $1; }
           | TK_ESYM_SLASH { $$ = $1; }
           | TK_ESYM_PERCENT { $$ = $1; } 
           ;

bin_addop: TK_ESYM_PLUS { $$ = $1; } 
           | TK_ESYM_MINUS { $$ = $1; } 
           ;
/*
unary_op: TK_ESYM_PLUS { $$ = $1; }
          | TK_ESYM_MINUS { $$ = $1; }
          | TK_ESYM_NOT { $$ = $1; }
          | TK_ESYM_CARET { $$ = $1; } 
          ;
*/

atom_expr: int_expr { $$ = $1; } 
           | float_expr { $$ = $1; } 
           | idref_expr { $$ = $1; }
           | TK_SYM_LBRACKET expr TK_SYM_RBRACKET { $$ = $2; }
           | func_call_expr { $$ = $1; }
           ;

int_expr:
  TK_NUM_INT { $$.ast = leaf_wdata(ANS_INTVAL, $1.token.val); } ;

float_expr:
  TK_NUM_FLOAT { $$.ast = leaf_wdata(ANS_FLOATVAL, $1.token.val); } ;

idref_expr:
  TK_ID { $$.ast = leaf_wdata(ANS_IDREF, $1.token.val); } ;

func_call_expr:
  idref_expr TK_SYM_LBRACKET semi_sep_list TK_SYM_RBRACKET
  { $$.ast = node2(ANS_FUNC_CALL, $1.ast, $3.ast); } ;

empty_statement: TK_SYM_SEMI { $$.ast = leaf(ANS_NULL); } ;

id_list: 
  id_list idref_expr
  {
    $$.ast = $1.ast;
    ast_list_append($$.ast, $2.ast);
  }
  | 
  {
    $$.ast = node_list(ANS_IDS); 
  }
  ;

semi_sep_list: 
  semi_sep_list TK_SYM_SEMI expr
  {
    $$.ast = $1.ast;
    ast_list_append($$.ast, $3.ast);
  }
  | expr 
  {
    $$.ast = node_list(ANS_SEMI_SEP_LIST);
    ast_list_append($$.ast, $1.ast);
  }
  | { $$.ast = node_list(ANS_SEMI_SEP_LIST); }
  ;

%%

void yyerror(const char *err) {
  fprintf(stderr, "at line %d, col %d: error: %s\n", yylval.token.row,
          yylval.token.col, err);
}

int main(int argc, char *argv[]) {
  extern FILE *fp_lex_in;
  if (argc == 2) {
    fp_lex_in = fopen(argv[1], "r");
    if (fp_lex_in == NULL) {
      fprintf(stderr, "Cannot open file %s\n", argv[1]);
      exit(-1);
    }
  }
  else if (argc == 1) {
    fp_lex_in = stdin;
  }
  else {
    fprintf(stderr, "Too many arguments\n");
  }
  yyparse();
  if (fp_lex_in != stdin) {
    fclose(fp_lex_in);
  }

  printf("glob_ast = %p\n", glob_ast);
  tree_dump(glob_ast);
}
