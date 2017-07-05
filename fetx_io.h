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

#ifndef FETX_IO_H
#define FETX_IO_H

#include "fetx_netlist.h"

/* fetx_io is a wrapper around fetx that represents a modular circuit
 * with abstracted inputs and outputs. */

struct fetx_io {
  struct fetx fx;
  struct fetx_input_node *inputs;
  struct fetx_node **outputs;
  size_t inputs_size;
  size_t outputs_size;
};

void fetx_io_delete(struct fetx_io io);
int fetx_io_init(struct fetx_io *const io, const struct fetx_netlist nl);
void fetx_io_input(struct fetx_io *const io, const size_t input_index,
                   const enum fetx_node_states state);
enum fetx_node_states fetx_io_output(const struct fetx_io io,
                                     const size_t output_index);
void fetx_io_inputs(struct fetx_io *const io,
                    const enum fetx_node_states *const inputs);
void fetx_io_outputs(enum fetx_node_states *const outputs,
                     const struct fetx_io io);
unsigned char fetx_io_resolve(struct fetx_io *const io);

#endif
