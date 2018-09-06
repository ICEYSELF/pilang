#include "clist.h"

typedef struct _node {
    any_t any;
    struct _node *prev, *next;
} node_t;

typedef struct _clistimpl {
    allocator_t alloc;
    deallocator_t dealloc;
    node_t head_node;
} list_impl_t;

void create_list(list_t *list, allocator_t alloc, deallocator_t dealloc) {
    list->impl = (list_impl_t*)alloc(sizeof(list_impl_t));
    list->impl->alloc = alloc;
    list->impl->dealloc = dealloc;

    list->impl->head_node.prev = &(list->impl->head_node);
    list->impl->head_node.next = &(list->impl->head_node);
}

void destroy_list(list_t *list) {
    deallocator_t dealloc = list->impl->dealloc;
    node_t *node = list->impl->head_node.next;
    while (node != (&list->impl->head_node)) {
        node = node->next;
        dealloc(node->prev);
    }
    dealloc(list->impl);
}

void list_push_back(list_t *list, any_t any) {
    list_insert_before(list, list_end(list), any);
}

void list_push_front(list_t *list, any_t any) {
    list_insert_before(list, iter_next(list_begin(list)), any);
}

void list_insert_before(list_t *list, iter_t iter, any_t any) {
    node_t *new_node = (node_t*)list->impl->alloc(sizeof(node_t));
    new_node->any = any;

    node_t *iter_node = (node_t*)iter.opaque;
    new_node->next = iter_node;
    new_node->prev = iter_node->prev;
    iter_node->prev->next = new_node;
    iter_node->next->prev = new_node;
}

void list_remove(list_t *list, iter_t iter) {
    node_t *iter_node = (node_t*)iter.opaque;
    iter_node->prev->next = iter_node->next;
    iter_node->next->prev = iter_node->prev;
    list->impl->dealloc(iter_node);
}

static iter_t create_iter(void* opaque) {
    iter_t iter;
    iter.opaque = opaque;
    return iter;
}

static const_iter_t create_const_iter(const void* opaque) {
    const_iter_t iter;
    iter.opaque = opaque;
    return iter;
}

iter_t list_begin(list_t *list) {
    return create_iter(list->impl->head_node.next);
}

iter_t list_end(list_t *list) {
    return create_iter(&(list->impl->head_node));
}

const_iter_t list_const_begin(const list_t *list) {
    return create_const_iter(list->impl->head_node.next);
}

const_iter_t list_const_end(const list_t *list) {
    return create_const_iter(&(list->impl->head_node));
}

any_t iter_deref(iter_t iter) {
    return ((node_t*)iter.opaque)->any;
}

const_any_t const_iter_deref(const_iter_t iter) {
    return ((const node_t*)iter.opaque)->any;
}

iter_t iter_next(iter_t iter) {
    return create_iter(((node_t*)iter.opaque)->next);
}

iter_t iter_prev(iter_t iter) {
    return create_iter(((node_t*)iter.opaque)->prev);
}

const_iter_t const_iter_next(const_iter_t iter) {
    return create_const_iter(((const node_t*)iter.opaque)->next);
}

const_iter_t const_iter_prev(const_iter_t iter) {
    return create_const_iter(((const node_t*)iter.opaque)->prev);
}
