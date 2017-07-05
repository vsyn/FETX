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

#include "fetx_io.h"

#include <stdio.h>

void fetx_io_delete(struct fetx_io io) {
  if (io.inputs != 0) {
    size_t i = 0;
    while (i < io.inputs_size) {
      fetx_input_delete(io.inputs[i]);
      ++i;
    }
    fetx_dealloc(io.inputs);
    io.inputs = 0;
  }
  if (io.outputs != 0) {
    fetx_dealloc(io.outputs);
    io.outputs = 0;
  }
  fetx_delete(io.fx);
}

int fetx_io_init(struct fetx_io *const io, const struct fetx_netlist nl) {
  io->inputs = 0;
  io->outputs = 0;
  /* generate intermediate */
  struct fetx_inter fxi;
  if (fetx_inter_init_fns(&fxi, nl.fl, nl.nodes_size) != 0) {
    return -1;
  }
  /* generate runtime data */
  if (fetx_init(&io->fx, fxi) != 0) {
    fetx_inter_delete(fxi);
    return -1;
  }

  /* fill inputs arr in io struct */
  io->inputs = fetx_alloc(sizeof(*io->inputs), nl.inputs_size);
  if (io->inputs == 0) {
    fetx_inter_delete(fxi);
    fetx_delete(io->fx);
    return -1;
  }

  size_t i = 0;
  while (i < nl.inputs_size) {
    if (fetx_input_init(io->inputs + i, &io->fx, fxi.nodes[nl.inputs[i]]) !=
        0) {
      fetx_inter_delete(fxi);
      io->inputs_size = i;
      fetx_io_delete(*io);
      return -1;
    }
    ++i;
  }
  io->inputs_size = i;

  fetx_inter_delete(fxi);

  /* fill outputs arr in io struct */
  io->outputs = fetx_alloc(sizeof(*io->outputs), nl.outputs_size);
  if (io->outputs == 0) {
    fetx_io_delete(*io);
    return -1;
  }

  i = 0;
  while (i < nl.outputs_size) {
    io->outputs[i] = io->fx.nodes + nl.outputs[i];
    ++i;
  }
  io->outputs_size = i;
  return 0;
}

void fetx_io_input(struct fetx_io *const io, const size_t input_index,
                   const enum fetx_node_states state) {
  fetx_input_state_set(&io->fx, io->inputs + input_index, state);
}

enum fetx_node_states fetx_io_output(const struct fetx_io io,
                                     const size_t output_index) {
  return fetx_node_state_get(*io.outputs[output_index]);
}

void fetx_io_inputs(struct fetx_io *const io,
                    const enum fetx_node_states *const inputs) {
  size_t i = 0;
  while (i < io->inputs_size) {
    fetx_io_input(io, i, inputs[i]);
    ++i;
  }
}

void fetx_io_outputs(enum fetx_node_states *const outputs,
                     const struct fetx_io io) {
  size_t i = 0;
  while (i < io.outputs_size) {
    outputs[i] = fetx_io_output(io, i);
    ++i;
  }
}

unsigned char fetx_io_resolve(struct fetx_io *const io) {
  return fetx_resolve(&io->fx);
}
