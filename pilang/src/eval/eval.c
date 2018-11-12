#define EVAL_C
#include "eval.h"

#include "ast.h"
#include "util.h"
#include "y.tab.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

plvalue_t create_onstack(stkobj_t *storage) {
  plvalue_t ret;
  ret.roc = ROC_ONSTACK;
  ret.type = soid2jt(storage->soid);
  ret.value.pvalue = storage;
  return ret;
}

plvalue_t create_onheap(heapobj_t *storage) {
  plvalue_t ret;
  ret.roc = ROC_ONHEAP;
  ret.type = hoid2jt(storage->oid);
  ret.value.pvalue = storage;
  return ret;
}

plvalue_t create_temp() {
  plvalue_t ret;
  ret.roc = ROC_TEMP;
  return ret;
}

jjvalue_t *fetch_storage(plvalue_t *obj) {
  switch (obj->roc) {
  case ROC_TEMP: return &(obj->value);
  case ROC_ONHEAP: {
    heapobj_t *heapobj = (heapobj_t*)(obj->value.pvalue);
    return &(heapobj->value);
  }
  case ROC_ONSTACK: {
    stkobj_t *stkobj = (stkobj_t*)(obj->value.pvalue);
    return &(stkobj->value);
  }
  }
  return NULL;
}

static result_t fetch_int(plvalue_t obj) {
  jjvalue_t *storage = fetch_storage(&obj);
  if (storage == NULL) {
    return failed_result("object does not have storage");
  }

  switch (obj.type) {
  case JT_INT:
    return success_result(*storage);
  case JT_FLOAT: {
    jjvalue_t shell;
    shell.fvalue = (int)(storage->ivalue);
    return success_result(shell);
  }
  case JT_BOOL: {
    jjvalue_t shell;
    shell.ivalue = (int)(storage->bvalue);
    return success_result(shell);
  }
  case JT_STR:
    return failed_result("cannot autocast from Str to Int");
  case JT_LIST:
    return failed_result("cannot autocast from List to Int");
  case JT_UNDEFINED:
    return failed_result("cannot autocast from Nothing to Int");
  }

  UNREAECHABLE
}

static result_t fetch_float(plvalue_t obj) {
  jjvalue_t *storage = fetch_storage(&obj);
  if (storage == NULL) {
    return failed_result("object does not have storage");
  }

  switch (obj.type) {
  case JT_INT: {
    jjvalue_t shell;
    shell.fvalue = (double)(storage->ivalue);
    return success_result(shell);
  }
  case JT_FLOAT: return success_result(*storage);
  case JT_BOOL: {
    jjvalue_t shell;
    shell.fvalue = (float)(storage->fvalue);
    return success_result(shell);
  }
  case JT_STR:
    return failed_result("cannot autocast from Str to Float");
  case JT_LIST:
    return failed_result("cannot autocast from List to Float");
  case JT_UNDEFINED:
    return failed_result("cannot autocast from Nothing to Float");
  }

  UNREAECHABLE
}

static result_t fetch_bool(plvalue_t obj) {
  jjvalue_t *storage = fetch_storage(&obj);
  if (storage == NULL) {
    return failed_result("object does not have storage");
  }

  switch (obj.type) {
  case JT_INT: {
    jjvalue_t shell;
    shell.bvalue = (bool)(storage->ivalue);
    return success_result(shell);
  }
  case JT_FLOAT: {
    jjvalue_t shell;
    shell.bvalue = (int)(storage->fvalue);
    return success_result(shell);
  }
  case JT_STR:
  case JT_LIST: {
    jjvalue_t shell;
    shell.bvalue = true;
    return success_result(shell);
  }
  case JT_UNDEFINED: {
    jjvalue_t shell;
    shell.bvalue = false;
    return success_result(shell);
  }
  }

  UNREAECHABLE
}

static result_t fetch_str(plvalue_t obj) {
  jjvalue_t *storage = fetch_storage(&obj);
  if (storage == NULL) {
    return failed_result("object does not have storage");
  }

  switch (obj.type) {
  case JT_INT: {
    char buffer[128];
    sprintf(buffer, "%" PRId64, storage->ivalue);
    jjvalue_t shell;
    shell.svalue = create_string(buffer);
    return success_result(shell);
  }
  case JT_FLOAT: {
    char buffer[128];
    sprintf(buffer, "%g", storage->fvalue);
    jjvalue_t shell;
    shell.svalue = create_string(buffer);
    return success_result(shell);
  }
  case JT_BOOL: {
    jjvalue_t shell;
    shell.svalue = storage->bvalue ? create_string("True") :
                                     create_string("False");
    return success_result(shell);
  }
  case JT_STR: return success_result(*storage);
  case JT_LIST:
    return failed_result("cannot autocast fron List to Str");
  }

  UNREAECHABLE
}

static result_t fetch_list(plvalue_t obj) {
  jjvalue_t *storage = fetch_storage(&obj);
  if (storage == NULL) {
    return failed_result("object does not have storage");
  }

  switch (obj.type) {
  case JT_INT:
    return failed_result("cannot autocast from Int to List");
  case JT_FLOAT:
    return failed_result("cannot autocast from Float to List");
  case JT_BOOL:
    return failed_result("cannot autocast from Bool to List");
  case JT_STR:
    return failed_result("cannot autocast from Str to List");
  case JT_LIST:
    return success_result(*storage);
  }

  UNREAECHABLE
}

static void asgn_attach_typeinfo(plvalue_t *obj, int16_t type) {
  obj->type = type;
  switch (obj->roc) {
    case ROC_ONSTACK: {
      stkobj_t *stkobj = (stkobj_t*)obj->value.pvalue;
      stkobj->soid = jt2soid(type);
      break;
    }
    case ROC_ONHEAP: {
      heapobj_t *heapobj = (heapobj_t*)obj->value.pvalue;
      heapobj->oid = jt2hoid(type);
      break;
    }
    default:
    UNREAECHABLE
  }
}

static void storage_precleanup(plvalue_t *obj) {
  if (obj->roc == ROC_ONHEAP) {
    destroy_object((heapobj_t*)(obj->value.pvalue));
  }
}

#define EITHER_IS(VALUETYPE, LHS, RHS) \
  ((LHS).type == VALUETYPE || (RHS).type == VALUETYPE)

static int64_t int_failsafe(result_t maybe) {
  return maybe.success ? maybe.value.ivalue : 0;
}

static double float_failsafe(result_t maybe) {
  return maybe.success ? maybe.value.fvalue : 0.0;
}

static strhdl_t str_failsafe(result_t maybe) {
  return maybe.success ? maybe.value.svalue
                       : create_string("undefined");
}

static plvalue_t auto_deref(plvalue_t maybe_ref) {
  if (maybe_ref.type != JT_REF) {
    return maybe_ref;
  }
  heapobj_t *referred =
    (heapobj_t*)(fetch_storage(&maybe_ref)->pvalue);
  return create_onheap(referred);
}

plvalue_t algebraic_calc(plvalue_t lhs, plvalue_t rhs,
                         algebraic_function_t alf) {
  lhs = auto_deref(lhs);
  rhs = auto_deref(rhs);

  if (alf == ALF_ADD) {
    if (EITHER_IS(JT_STR, lhs, rhs)) {
      const char* strl = get_string(str_failsafe(fetch_str(lhs)));
      const char* strr = get_string(str_failsafe(fetch_str(rhs)));
      char *temp = NEWN(char, strlen(strl) + strlen(strr) + 1);
      strcpy(temp, strl);
      strcat(temp, strr);
      int newstr = create_string(temp);
      free(temp);

      plvalue_t ret = create_temp();
      ret.type = JT_STR;
      ret.value.svalue = newstr;
      return ret;
    }
  }

  if (EITHER_IS(JT_FLOAT, lhs, rhs) && alf != ALF_MOD) {
    double f1 = float_failsafe(fetch_float(lhs));
    double f2 = float_failsafe(fetch_float(rhs));

    plvalue_t ret = create_temp();
    ret.type = JT_FLOAT;
    switch (alf) {
    case ALF_ADD: ret.value.fvalue = f1 + f2; break;
    case ALF_SUB: ret.value.fvalue = f1 - f2; break;
    case ALF_MUL: ret.value.fvalue = f1 * f2; break;
    case ALF_DIV: ret.value.fvalue = f1 / f2; break;
    default: UNREAECHABLE
    }
    return ret;
  }
  else if (EITHER_IS(JT_INT, lhs, rhs)) {
    int64_t i1 = int_failsafe(fetch_int(lhs));
    int64_t i2 = int_failsafe(fetch_int(rhs));

    plvalue_t ret = create_temp();
    ret.type = JT_INT;
    switch (alf) {
    case ALF_ADD: ret.value.ivalue = i1 + i2; break;
    case ALF_SUB: ret.value.ivalue = i1 - i2; break;
    case ALF_MUL: ret.value.ivalue = i1 * i2; break;
    case ALF_DIV: ret.value.ivalue = i1 / i2; break;
    case ALF_MOD: ret.value.ivalue = i1 % i2; break;
    }
    return ret;
  }
  else {
    plvalue_t ret = create_temp();
    ret.type = JT_UNDEFINED;
    return ret;
  }
}

plvalue_t assign(plvalue_t lhs, plvalue_t rhs) {
  if (lhs.roc != ROC_ONSTACK && lhs.roc != ROC_ONHEAP) {
    return lhs;
  }

  if (lhs.roc == ROC_ONSTACK) {
    lhs = auto_deref(lhs);
  }

  if (lhs.roc == ROC_ONHEAP) {
    rhs = auto_deref(rhs);
  }

  jjvalue_t *rhs_storage = fetch_storage(&rhs),
            *lhs_storage = fetch_storage(&lhs);

  if (lhs_storage == NULL) {
    return lhs;
  }

  storage_precleanup(&lhs);
  asgn_attach_typeinfo(&lhs, rhs.type);
  *lhs_storage = *rhs_storage;
  return lhs;
}

plvalue_t eval_literal_expr(ast_leaf_wdata_t *node) {
  plvalue_t ret = create_temp();
  switch (node->node_sema_info) {
  case ANS_INTVAL:   ret.type = JT_INT;   break;
  case ANS_FLOATVAL: ret.type = JT_FLOAT; break;
  case ANS_STR:      ret.type = JT_STR;   break;
  case ANS_BOOLVAL:  ret.type = JT_BOOL;  break;
  }
  ret.value = node->value;
  return ret;
}

plvalue_t eval_idref_expr(ast_leaf_wdata_t *node, stack_t *stack) {
  return create_onstack(stack_get(stack, node->value.svalue));
}

plvalue_t eval_binexpr(ast_dchild_wdata_t *node, stack_t *stack) {
  plvalue_t lhs = eval_expr(node->children[0], stack);
  plvalue_t rhs = eval_expr(node->children[1], stack);
  if (node->value.ivalue == TK_ESYM_EQ) {
    return assign(lhs, rhs);
  }

  algebraic_function_t alf;
  switch (node->value.ivalue) {
  case TK_ESYM_PLUS:    alf = ALF_ADD; break;
  case TK_ESYM_MINUS:   alf = ALF_SUB; break;
  case TK_ESYM_ASTER:   alf = ALF_MUL; break;
  case TK_ESYM_SLASH:   alf = ALF_DIV; break;
  case TK_ESYM_PERCENT: alf = ALF_MOD; break;
  default: {
      UNREAECHABLE
      plvalue_t failure = create_temp();
      failure.type = JT_UNDEFINED;
      return failure;
    }
  }
  return algebraic_calc(lhs, rhs, alf);
}

plvalue_t eval_expr(ast_node_base_t *node, stack_t *stack) {
  switch (node->node_sema_info) {
  case ANS_BINEXPR:
    return eval_binexpr((ast_dchild_wdata_t*)node, stack);
  case ANS_IDREF:
    return eval_idref_expr((ast_leaf_wdata_t*)node, stack);
  case ANS_INTVAL: case ANS_FLOATVAL: case ANS_BOOLVAL: case ANS_STR:
    return eval_literal_expr((ast_leaf_wdata_t*)node);
  }
  UNREAECHABLE;
  plvalue_t failure = create_temp();
  failure.type = JT_UNDEFINED;
  return failure;
}

void eval_stmt(ast_node_base_t *stmt, stack_t *stack) {
  // TODO
  (void)stmt;
  (void)stack;
}

void eval_func_body(ast_list_t *body, stack_t *stack) {
  list_t stmts = body->list;
  for (iter_t it = list_begin(&stmts);
       !iter_eq(it, list_end(&stmts));
       it = iter_next(it)) {
    eval_stmt((ast_node_base_t*)iter_deref(it), stack);
  }
}

static void callfunc(ast_tchild_wdata_t *func, list_t args,
                     list_t rets, stack_t *stack) {
  stack_enter_frame(stack);
  ast_list_t *param_list_node = (ast_list_t*)func->children[0];
  list_t param_list = param_list_node->list;
  for (iter_t it1 = list_begin(&param_list),
              it2 = list_begin(&args);
       !iter_eq(it1, list_end(&param_list))
       && !iter_eq(it2, list_end(&args));
       it1 = iter_next(it1), it2 = iter_next(it2)) {
    ast_leaf_wdata_t *param_idref_node =
      (ast_leaf_wdata_t*)iter_deref(it1);
    assign(eval_idref_expr(param_idref_node, stack),
           *(plvalue_t*)iter_deref(it2));
  }

  eval_func_body((ast_list_t*)(func->children[2]), stack);

  ast_list_t *rets_list_node = (ast_list_t*)func->children[1];
  list_t rets_list = rets_list_node->list;
  for (iter_t it = list_begin(&rets_list);
       !iter_eq(it, list_end(&rets_list));
       it = iter_next(it)) {
    plvalue_t *t = NEW(plvalue_t);
    *t = create_temp();
    ast_leaf_wdata_t *ret_idref_node = (ast_leaf_wdata_t*)iter_deref(it);
    assign(*t, eval_idref_expr(ret_idref_node, stack));
    list_push_back(&rets, t);
  }

  stack_exit_frame(stack);
}

void eval_ast(ast_node_base_t *program) {
  (void)program;
}
