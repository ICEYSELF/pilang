#include "ast.h"

#include <stdlib.h>

#define NEW(TYPE) (TYPE*)malloc(sizeof(TYPE))

ast_node_base_t *leaf(ast_node_sema_t sema_info) {
  ast_leaf_t *ret = NEW(ast_leaf_t);
  ret->node_sema_info = sema_info;
  ret->node_kind = ANK_LEAF;
  return (ast_node_base_t*)ret;
}

ast_node_base_t *leaf_wdata(ast_node_sema_t sema_info, jjvalue_t data) {
  ast_leaf_wdata_t *ret = NEW(ast_leaf_wdata_t);
  ret->node_kind = ANK_LEAF_WDATA;
  ret->node_sema_info = sema_info;
  ret->data = data;
  return (ast_node_base_t*)ret;
}

ast_node_base_t *node1(ast_node_sema_t sema_info, 
                       ast_node_base_t *child) {
  ast_schild_t *ret = NEW(ast_schild_t);
  ret->node_kind = ANK_SINGLE_CHILD;
  ret->node_sema_info = sema_info;
  ret->child = child;
  return (ast_node_base_t*)ret;
}

ast_node_base_t *node1_wdata(ast_node_sema_t sema_info, jjvalue_t data,
                             ast_node_base_t *child) {
  ast_schild_wdata_t *ret = NEW(ast_schild_wdata_t);
  ret->node_kind = ANK_SINGLE_CHILD_WDATA;
  ret->node_sema_info = sema_info;
  ret->data = data;
  ret->child = child;
  return (ast_node_base_t*)ret;
}

ast_node_base_t *node2(ast_node_sema_t sema_info, 
                       ast_node_base_t *lchild, 
                       ast_node_base_t *rchild) {
  ast_dchild_t *ret = NEW(ast_dchild_t);
  ret->node_kind = ANK_DUAL_CHILD;
  ret->node_sema_info = sema_info;
  ret->children[0] = lchild;
  ret->children[1] = rchild;
  return (ast_node_base_t*)ret;
}

ast_node_base_t *node2_wdata(ast_node_sema_t sema_info, jjvalue_t data,
                             ast_node_base_t *lchild,
                             ast_node_base_t *rchild) {
  ast_dchild_wdata_t *ret = NEW(ast_dchild_wdata_t);
  ret->node_kind = ANK_DUAL_CHILD_WDATA;
  ret->node_sema_info = sema_info;
  ret->data = data;
  ret->children[0] = lchild;
  ret->children[1] = rchild;
  return (ast_node_base_t*)ret;
}

ast_node_base_t *node3(ast_node_sema_t sema_info, 
                       ast_node_base_t *child0, 
                       ast_node_base_t *child1,
                       ast_node_base_t *child2) {
  ast_tchild_t *ret = NEW(ast_tchild_t);
  ret->node_kind = ANK_TRIPLE_CHILD;
  ret->node_sema_info = sema_info;
  ret->children[0] = child0;
  ret->children[1] = child1;
  ret->children[2] = child2;
  return (ast_node_base_t*)ret;
}

ast_node_base_t *node3_wdata(ast_node_sema_t sema_info, jjvalue_t data,
                             ast_node_base_t *child0,
                             ast_node_base_t *child1, 
                             ast_node_base_t *child2) {
  ast_tchild_wdata_t *ret = NEW(ast_tchild_wdata_t);
  ret->node_kind = ANK_TRIPLE_CHILD_WDATA;
  ret->node_sema_info = sema_info;
  ret->data = data;
  ret->children[0] = child0;
  ret->children[1] = child1;
  ret->children[2] = child2;
  return (ast_node_base_t*)ret;
}
