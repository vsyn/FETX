/*
Copyright 2017 Julian Ingram

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */

#include "fetx.h"

static int fetx_check_post_multiply(const size_t q, const size_t a,
                                    const size_t b) {
  return ((b != 0) && ((q / b) != a)) ? -1 : 0;
}

int fetx_check_multiply(size_t *const q, const size_t a, const size_t b) {
  *q = a * b;
  return fetx_check_post_multiply(*q, a, b);
}

void *fetx_alloc(const size_t nmemb, const size_t size) {
  const size_t alloc_size = size * nmemb;
  return (fetx_check_post_multiply(alloc_size, nmemb, size) != 0)
             ? 0
             : malloc(alloc_size);
}

void *fetx_calloc(const size_t nmemb, const size_t size) {
  return calloc(nmemb, size);
}

void fetx_dealloc(void *const ptr) { free(ptr); }

static inline void fetx_dealloc_two(void *const a, void *const b) {
  fetx_dealloc(a);
  fetx_dealloc(b);
}

size_t fetx_fetlist_find_last_node(const struct fetx_fetlist fl) {
  size_t last_node_id = 0;
  size_t i = 0;
  while (i < fl.size) {
    unsigned char c = 0;
    while (c < 3) {
      size_t node_id = fl.fets[i].connections[c];
      if (node_id > last_node_id) {
        last_node_id = node_id;
      }
      ++c;
    }
    ++i;
  }
  return last_node_id;
}

void fetx_fetlist_delete(struct fetx_fetlist fl) {
  if (fl.fets != 0) {
    fetx_dealloc(fl.fets);
  }
}

int fetx_fetlist_new(struct fetx_fetlist *const fl) {
  fl->fets = fetx_alloc(fl->size, sizeof(struct fetx_fetlist_fet));
  return (fl->fets == 0) ? -1 : 0;
}

void fetx_fetlist_assign_fet(struct fetx_fetlist *const fl,
                             const struct fetx_fetlist_fet fet,
                             const size_t index) {
  fl->fets[index] = fet;
}

void fetx_inter_delete(struct fetx_inter fxi) {
  if (fxi.nodes != 0) {
    if (fxi.nodes[0].connections != 0) {
      fetx_dealloc(fxi.nodes[0].connections);
    }
    fetx_dealloc(fxi.nodes);
  }
  if (fxi.fets != 0) {
    fetx_dealloc(fxi.fets);
  }
}

/* \nodes_size must be the id of the highest node + 1, not the number of nodes
 */

int fetx_inter_init_fns(struct fetx_inter *const fxi,
                        const struct fetx_fetlist fl, size_t nodes_size) {
  fxi->nodes_size = nodes_size;
  fxi->fets_size = fl.size;
  /* alloc memory for the counting of node connections */
  struct nodes_alloc_sizes {
    size_t connections;
    size_t control;
  };

  struct nodes_alloc_sizes *alloc_sizes =
      fetx_calloc(sizeof(*alloc_sizes), nodes_size);
  if (alloc_sizes == 0) {
    return -1;
  }

  size_t n = 0;
  while (n < fl.size) {
    ++alloc_sizes[fl.fets[n].connections[0]].control;     /* gate */
    ++alloc_sizes[fl.fets[n].connections[1]].connections; /* source */
    ++alloc_sizes[fl.fets[n].connections[2]].connections; /* drain */
    ++n;
  }

  /* alloc memory for the fetx struct */
  fxi->nodes = fetx_alloc(sizeof(*fxi->nodes), nodes_size);
  if (fxi->nodes == 0) {
    fetx_dealloc(alloc_sizes);
    return -1;
  }

  fxi->fets = fetx_alloc(sizeof(*fxi->fets), fl.size);
  if (fxi->fets == 0) {
    fetx_dealloc_two(alloc_sizes, fxi->nodes);
    return -1;
  }

  if (fetx_check_multiply(&n, fl.size, 3) != 0) {
    fetx_dealloc_two(alloc_sizes, fxi->nodes);
    return -1;
  }
  struct fetx_inter_fet **concon = fetx_alloc(sizeof(*concon), n);
  if (concon == 0) {
    fetx_dealloc_two(alloc_sizes, fxi->nodes);
    fetx_dealloc(fxi->fets);
    return -1;
  }

  n = 0;
  while (n < nodes_size) {
    struct fetx_inter_node *node = fxi->nodes + n;
    node->index = n;
    node->connections = concon;
    node->connections_limit = concon; /* not the final value */
    concon += alloc_sizes[n].connections;
    node->control = concon;
    node->control_limit = concon;
    concon += alloc_sizes[n].control;
    ++n;
  }
  fetx_dealloc(alloc_sizes);

  n = 0;
  while (n < fl.size) {
    struct fetx_inter_fet *fet = fxi->fets + n;
    fet->index = n;
    fet->control = fxi->nodes + fl.fets[n].connections[0];
    *fet->control->control_limit = fxi->fets + n;
    ++fet->control->control_limit;
    fet->connections[0] = fxi->nodes + fl.fets[n].connections[1];
    *fet->connections[0]->connections_limit = fxi->fets + n;
    ++fet->connections[0]->connections_limit;
    fet->connections[1] = fxi->nodes + fl.fets[n].connections[2];
    *fet->connections[1]->connections_limit = fxi->fets + n;
    ++fet->connections[1]->connections_limit;
    fet->type = fl.fets[n].type;
    ++n;
  }
  return 0;
}

int fetx_inter_init(struct fetx_inter *const fxi,
                    const struct fetx_fetlist fl) {
  return fetx_inter_init_fns(fxi, fl, fetx_fetlist_find_last_node(fl) + 1);
}

void fetx_delete(struct fetx fx) {
  if (fx.nodes != 0) {
    fetx_dealloc(fx.nodes);
  }
  if (fx.fets != 0) {
    fetx_dealloc(fx.fets);
  }
}

int fetx_init(struct fetx *const fx, const struct fetx_inter fxi) {
  fx->nodes = fetx_alloc(sizeof(*fx->nodes), fxi.nodes_size);
  if (fx->nodes == 0) {
    return -1;
  }
  fx->nodes_limit = fx->nodes + fxi.nodes_size;

  struct fetx_node *node = fx->nodes;
  while (node < fx->nodes_limit) {
    node->index = node - fx->nodes;
    node->state_counts[FETX_LOW] = 0;
    node->state_counts[FETX_HIGH] = 0;
    node->state_counts[FETX_UNSTABLE_LOW] = 0;
    node->state_counts[FETX_UNSTABLE_HIGH] = 0;
    node->control = 0;
    node->is_input = 0;
    node->flag = 0;
    ++node;
  }

  fx->fets = fetx_alloc(sizeof(*fx->fets), fxi.fets_size);
  if (fx->fets == 0) {
    return -1;
  }
  fx->fets_limit = fx->fets + fxi.fets_size;

  struct fetx_fet *fet = fx->fets;
  while (fet < fx->fets_limit) {
    size_t index = fet - fx->fets;
    const struct fetx_inter_fet inter_fet = fxi.fets[index];
    struct fetx_node *control_node = &fx->nodes[inter_fet.control->index];
    fet->index = index;
    fet->control = control_node;
    fet->next_control = control_node->control;
    control_node->control = fet;
    fet->next_listed = 0;
    fet->state = FETX_UNSTABLE;
    fet->type = inter_fet.type;
    fet->links = 0;
    fet->is_listed = 0;
    ++fet;
  }

  fx->fets_update = 0;
  fx->input_nodes_update = 0;
  return 0;
}

static struct fetx_inter_node
fetx_get_connected_node(const struct fetx_inter_node node,
                        const struct fetx_inter_fet fet) {
  return *fet.connections[(node.index == fet.connections[0]->index) ? 1 : 0];
}

void fetx_input_delete(struct fetx_input_node path) {
  struct fetx_input_node *output = path.outputs;
  while (output != 0) {
    fetx_input_delete(*output);
    struct fetx_input_node *next_output = output->next_output;
    fetx_dealloc(output);
    output = next_output;
  }
}

static int fetx_input_init_rec(struct fetx_input_node *const path,
                               struct fetx *const fx,
                               const struct fetx_inter_node inter_node) {

  struct fetx_node *const node = fx->nodes + inter_node.index;
  node->flag = 1;

  struct fetx_inter_fet **inter_fet_itt = inter_node.connections;
  while (inter_fet_itt != inter_node.connections_limit) {
    /* for each connection */
    const struct fetx_inter_fet inter_fet = **inter_fet_itt;
    const struct fetx_inter_node connected_inter_node =
        fetx_get_connected_node(inter_node, inter_fet);
    struct fetx_node *connected_node = fx->nodes + connected_inter_node.index;
    /* check for permanent comp pairs and FETs connected to their own gate */
    struct fetx_input_node *el = path;
    while ((el->link.input != 0) &&
           ((el->link.fet->control->index != inter_fet.control->index) ||
            (el->link.fet->type == inter_fet.type)) &&
           (el->node->index != inter_fet.control->index)) {
      el = el->link.input;
    }
    /* check if node is already on the path */
    if ((el->link.input == 0) && (connected_node->flag == 0)) {
      /* add to outputs */
      struct fetx_input_node *const new_path = fetx_alloc(sizeof(*new_path), 1);
      if (new_path == 0) {
        return -1;
      }

      new_path->next_output = path->outputs;
      path->outputs = new_path;

      /* add link to FET */
      struct fetx_fet *const fet = fx->fets + inter_fet.index;
      new_path->link.next = fet->links;
      fet->links = &new_path->link;

      /* add to path */
      new_path->node = connected_node;
      new_path->state = FETX_UNDRIVEN;
      new_path->link.input = path;
      new_path->link.output = new_path;
      new_path->link.fet = fet;
      new_path->next_listed = 0;
      new_path->is_listed = 0;
      new_path->outputs = 0;

      if (fetx_input_init_rec(new_path, fx, connected_inter_node) != 0) {
        fetx_dealloc(new_path);
        path->outputs = 0;
        return -1;
      }
    }
    ++inter_fet_itt;
  }
  node->flag = 0;
  return 0;
}

int fetx_input_init(struct fetx_input_node *const path, struct fetx *const fx,
                    const struct fetx_inter_node inter_node) {
  struct fetx_node *const node = fx->nodes + inter_node.index;
  node->is_input = 1;
  path->node = node;
  path->state = FETX_UNDRIVEN;
  path->link.input = 0;
  path->outputs = 0;
  path->next_output = 0;
  path->is_listed = 0;

  return fetx_input_init_rec(path, fx, inter_node);
}

static int fetx_node_state_test(const size_t *const state_counts) {
  return ((state_counts[0] != 0) || (state_counts[2] != 0)) ? -1 : 0;
}

enum fetx_node_states fetx_node_state_get(const struct fetx_node node) {
  if (node.state_counts[FETX_LOW] != 0) {
    return (fetx_node_state_test(node.state_counts + FETX_HIGH) != 0)
               ? FETX_UNSTABLE_MULTIPLE
               : FETX_LOW;
  } else if (node.state_counts[FETX_HIGH] != 0) {
    return (fetx_node_state_test(node.state_counts + FETX_LOW) != 0)
               ? FETX_UNSTABLE_MULTIPLE
               : FETX_HIGH;
  } else if (node.state_counts[FETX_UNSTABLE_LOW] != 0) {
    return (node.state_counts[FETX_UNSTABLE_HIGH] != 0) ? FETX_UNSTABLE_MULTIPLE
                                                        : FETX_UNSTABLE_LOW;
  }
  return (node.state_counts[FETX_UNSTABLE_HIGH] != 0) ? FETX_UNSTABLE_HIGH
                                                      : FETX_UNDRIVEN;
}

static enum fetx_fet_states fetx_fet_state_get(const struct fetx_fet fet) {
  const enum fetx_node_states control_state = fetx_node_state_get(*fet.control);
  enum fetx_fet_states state = FETX_UNSTABLE;
  if (fet.type == FETX_FET_N) {
    if (control_state == FETX_LOW) {
      state = FETX_OPEN;
    } else if (control_state == FETX_HIGH) {
      state = FETX_CLOSED;
    }
  } else {
    if (control_state == FETX_HIGH) {
      state = FETX_OPEN;
    } else if (control_state == FETX_LOW) {
      state = FETX_CLOSED;
    }
  }
  return state;
}

static void
fetx_input_node_add_to_list(struct fetx *const fx,
                            struct fetx_input_node *const input_node) {
  if (input_node->is_listed == 0) {
    input_node->next_listed = fx->input_nodes_update;
    fx->input_nodes_update = input_node;
    input_node->is_listed = 1;
  }
}

static void fetx_fet_add_to_list(struct fetx *const fx,
                                 struct fetx_fet *const fet) {
  if (fet->is_listed == 0) {
    fet->next_listed = fx->fets_update;
    fx->fets_update = fet;
    fet->is_listed = 1;
  }
}

static enum fetx_node_states fetx_link_get_output(const struct fetx_link link) {
  enum fetx_node_states input_state = link.input->state;
  const struct fetx_fet fet = *link.fet;
  if (fet.type == FETX_FET_N) {
    if ((fet.state == FETX_OPEN) || (input_state == FETX_HIGH) ||
        (input_state == FETX_UNSTABLE_HIGH)) {
      input_state = FETX_UNDRIVEN;
    } else if ((fet.state == FETX_UNSTABLE) && (input_state == FETX_LOW)) {
      input_state = FETX_UNSTABLE_LOW;
    }
  } else {
    if ((fet.state == FETX_OPEN) || (input_state == FETX_LOW) ||
        (input_state == FETX_UNSTABLE_LOW)) {
      input_state = FETX_UNDRIVEN;
    } else if ((fet.state == FETX_UNSTABLE) && (input_state == FETX_HIGH)) {
      input_state = FETX_UNSTABLE_HIGH;
    }
  }
  return input_state;
}

static void fetx_input_node_update(struct fetx *const fx,
                                   struct fetx_input_node *const node);

/* lists the output node instead of updating it */

static void fetx_fet_change_state(struct fetx *const fx,
                                  struct fetx_fet *const fet,
                                  const enum fetx_fet_states state) {
  if (state != fet->state) {
    /* update state */
    fet->state = state;
    /* update output nodes */
    struct fetx_link *link = fet->links;
    while (link != 0) {
      fetx_input_node_add_to_list(fx, link->output);
      link = link->next;
    }
  }
}

/* updates a FET and removes the listed flag */

static void fetx_fet_update(struct fetx *const fx, struct fetx_fet *const fet) {
  enum fetx_fet_states new_state = fetx_fet_state_get(*fet);
  fetx_fet_change_state(fx, fet, new_state);
  fet->is_listed = 0;
}

static void fetx_node_update_input(struct fetx_node *const node,
                                   enum fetx_node_states old_state,
                                   enum fetx_node_states new_state) {
  if (old_state < (sizeof(node->state_counts) / sizeof(*node->state_counts))) {
    --node->state_counts[old_state];
  }
  if (new_state < (sizeof(node->state_counts) / sizeof(*node->state_counts))) {
    ++node->state_counts[new_state];
  }
}

void fetx_input_state_set(struct fetx *const fx,
                          struct fetx_input_node *const input_node,
                          const enum fetx_node_states new_state) {
  if (new_state != input_node->state) {
    fetx_node_update_input(input_node->node, input_node->state, new_state);
    input_node->state = new_state;

    /* update nodes */
    struct fetx_input_node *output = input_node->outputs;
    while (output != 0) {
      fetx_input_node_update(fx, output->link.output);
      output = output->next_output;
    }
    /* update FETs */
    struct fetx_fet *control = input_node->node->control;
    while (control != 0) {
      fetx_fet_add_to_list(fx, control);
      control = control->next_control;
    }
  }
}

static void fetx_input_node_update(struct fetx *const fx,
                                   struct fetx_input_node *const node) {
  fetx_input_state_set(fx, node, fetx_link_get_output(node->link));
}

static void fetx_fets_update(struct fetx *const fx) {
  struct fetx_fet *fet = fx->fets_update;
  while (fet != 0) {
    fetx_fet_update(fx, fet);
    fet->is_listed = 0;
    fet = fet->next_listed;
  }
  fx->fets_update = 0;
}

static void fetx_input_nodes_update(struct fetx *const fx) {
  struct fetx_input_node *input_node = fx->input_nodes_update;
  while (input_node != 0) {
    fetx_input_node_update(fx, input_node);
    input_node->is_listed = 0;
    input_node = input_node->next_listed;
  }
  fx->input_nodes_update = 0;
}

unsigned char fetx_resolve(struct fetx *const fx) {
  fetx_fets_update(fx);
  fetx_input_nodes_update(fx);
  return (fx->fets_update == 0) ? 1 : 0;
}

size_t fetx_multiple_drive_detect(const struct fetx fx) {
  size_t res = 0;
  struct fetx_node *node = fx.nodes;
  while (node != fx.nodes_limit) {
    if (fetx_node_state_get(*node) == FETX_UNSTABLE_MULTIPLE) {
      ++res;
    }
    ++node;
  }
  return res;
}
