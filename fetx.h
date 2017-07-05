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

#ifndef FETX_H
#define FETX_H

#include <stdlib.h>

enum fetx_fet_states { FETX_OPEN = 0, FETX_CLOSED, FETX_UNSTABLE };

enum fetx_node_states {
  FETX_LOW = 0,
  FETX_HIGH,
  FETX_UNSTABLE_LOW,
  FETX_UNSTABLE_HIGH,
  FETX_UNSTABLE_MULTIPLE,
  FETX_UNDRIVEN
};

enum fetx_fet_types {
  FETX_FET_N = 0,
  FETX_FET_P,
};

struct fetx_fetlist_fet {
  /* gate source drain, gate is connections[0] */
  size_t connections[3];
  enum fetx_fet_types type;
};

struct fetx_fetlist {
  struct fetx_fetlist_fet *fets;
  size_t size;
};

struct fetx_inter_node {
  size_t index;
  struct fetx_inter_fet **connections;
  struct fetx_inter_fet **connections_limit;
  struct fetx_inter_fet **control;
  struct fetx_inter_fet **control_limit;
};

struct fetx_inter_fet {
  size_t index;
  /* the source and drain of the symmetrical FET */
  struct fetx_inter_node *connections[2];
  struct fetx_inter_node *control;
  enum fetx_fet_types type;
};

struct fetx_inter {
  struct fetx_inter_node *nodes;
  struct fetx_inter_fet *fets;
  size_t nodes_size;
  size_t fets_size;
};

struct fetx_input_node;

struct fetx_node {
  size_t index;
  size_t state_counts[4]; /* indexed with enum fetx_node_states */
  struct fetx_fet *control;
  unsigned int is_input : 1;
  unsigned int flag : 1;
};

struct fetx_fet {
  size_t index;
  struct fetx_node *control;
  struct fetx_fet *next_control;
  struct fetx_fet *next_listed;
  struct fetx_link *links;
  enum fetx_fet_states state;
  enum fetx_fet_types type;
  unsigned int is_listed : 1;
};

struct fetx_link {
  struct fetx_fet *fet;
  struct fetx_input_node *input;
  struct fetx_input_node *output;
  struct fetx_link *next;
};

struct fetx_input_node {
  struct fetx_node *node;
  struct fetx_link link;
  struct fetx_input_node *outputs;
  struct fetx_input_node *next_output;
  struct fetx_input_node *next_listed;
  enum fetx_node_states state;
  unsigned int is_listed : 1;
};

struct fetx_node_arr {
  struct fetx_node **elements;
  struct fetx_node **limit;
};

/* holds the nodes used for FET control, also the lists used at runtime */

struct fetx {
  struct fetx_node *nodes;
  struct fetx_node *nodes_limit;
  struct fetx_fet *fets;
  struct fetx_fet *fets_limit;
  struct fetx_fet *fets_update;
  struct fetx_input_node *input_nodes_update;
};

/* shared util */

int fetx_check_multiply(size_t *const q, const size_t a, const size_t b);
void *fetx_alloc(const size_t nmemb, const size_t size);
void *fetx_calloc(const size_t nmemb, const size_t size);
void fetx_dealloc(void *const ptr);

/* FET list */

size_t fetx_fetlist_find_last_node(const struct fetx_fetlist fl);
void fetx_fetlist_delete(struct fetx_fetlist fl);
int fetx_fetlist_new(struct fetx_fetlist *const fl);
void fetx_fetlist_assign_fet(struct fetx_fetlist *const fl,
                             const struct fetx_fetlist_fet fet,
                             const size_t index);

/* intermediate representation */

void fetx_inter_delete(struct fetx_inter fxi);
int fetx_inter_init_fns(struct fetx_inter *const fxi,
                        const struct fetx_fetlist fl, size_t nodes_size);
int fetx_inter_init(struct fetx_inter *const fxi, const struct fetx_fetlist fl);

/* runtime data */

void fetx_delete(struct fetx fx);
int fetx_init(struct fetx *const fx, const struct fetx_inter fxi);

void fetx_input_delete(struct fetx_input_node path);
int fetx_input_init(struct fetx_input_node *const path, struct fetx *const fx,
                    const struct fetx_inter_node inter_node);

/* runtime */

enum fetx_node_states fetx_node_state_get(const struct fetx_node node);

void fetx_input_state_set(struct fetx *const fx,
                          struct fetx_input_node *const input_node,
                          const enum fetx_node_states new_state);
/* returns 1 if the circuit has resolved, 0 otherwise */
unsigned char fetx_resolve(struct fetx *const fx);
size_t fetx_multiple_drive_detect(const struct fetx fx);

#endif
