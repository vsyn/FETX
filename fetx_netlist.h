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

#ifndef FETX_NETLIST_H
#define FETX_NETLIST_H

#include "fetx.h"

enum fetx_errs {
  FETX_ERR_NONE = 0,
  FETX_ERR_PARAM = 1,
  FETX_ERR_ALLOC = 2,
  FETX_ERR_FOPEN = 4,
  FETX_ERR_FCLOSE = 8,
  FETX_ERR_FFORMAT = 16,
  FETX_ERR_IO = 32,
  FETX_ERR_TIMEOUT = 64
};

struct fetx_netlist {
  struct fetx_fetlist fl;
  size_t *inputs;
  size_t *outputs;
  size_t nodes_size;
  size_t inputs_size;
  size_t outputs_size;
};

void fetx_netlist_delete(struct fetx_netlist nl);
enum fetx_errs fetx_netlist_new(struct fetx_netlist *const nl);
void fetx_netlist_assign_fet(struct fetx_netlist *const nl,
                             const struct fetx_fetlist_fet fet,
                             const size_t index);
void fetx_netlist_assign_input(struct fetx_netlist *const nl,
                               const size_t node_index, const size_t index);
void fetx_netlist_assign_output(struct fetx_netlist *const nl,
                                const size_t node_index, const size_t index);
void fetx_netlist_update_nodes_size(struct fetx_netlist *const nl);

enum fetx_errs fetx_netlist_from_file(struct fetx_netlist *const nl,
                                      const char *const pathname);
enum fetx_errs fetx_netlist_to_file(const struct fetx_netlist nl,
                                    const char *const pathname);
enum fetx_errs fetx_netlist_print(const struct fetx_netlist nl);

#endif
