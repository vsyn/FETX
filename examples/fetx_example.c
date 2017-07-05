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

#include "../fetx_vector.h"

#include <stdio.h>

int fetx_example_inverter(void) {
  struct fetx_fetlist_fet fets[] = {
      {.type = FETX_FET_P, .connections = {2, 1, 3}},
      {.type = FETX_FET_N, .connections = {2, 3, 0}}};

  struct fetx_netlist nl;
  nl.fl.size = (sizeof(fets) / sizeof(fets[0]));
  nl.inputs_size = 3;
  nl.outputs_size = 1;

  enum fetx_errs errs = fetx_netlist_new(&nl);
  if (errs != FETX_ERR_NONE) {
    puts("Failed to allocate netlist");
    return -1;
  }

  /* assign FETs */
  size_t i = 0;
  while (i < nl.fl.size) {
    fetx_netlist_assign_fet(&nl, fets[i], i);
    ++i;
  }

  fetx_netlist_update_nodes_size(&nl);

  /* assign IO */
  i = 0;
  while (i < nl.inputs_size) {
    fetx_netlist_assign_input(&nl, i, i);
    ++i;
  }
  size_t j = 0;
  while (j < nl.outputs_size) {
    fetx_netlist_assign_output(&nl, i, j);
    ++i;
    ++j;
  }

  /* compile */
  struct fetx_io io;
  errs = fetx_io_init(&io, nl);
  fetx_netlist_delete(nl);
  if (errs != FETX_ERR_NONE) {
    return -1;
  }

  puts("Simulating inverter");

  /* set Vcc and clear ground */
  fetx_io_input(&io, 0, FETX_LOW);
  fetx_io_input(&io, 1, FETX_HIGH);

  /* clear input */
  fetx_io_input(&io, 2, FETX_LOW);
  while (fetx_io_resolve(&io) == 0) {
  }
  printf("Cleared input, output: %u\n", fetx_io_output(io, 0));
  /* set input */
  fetx_io_input(&io, 2, FETX_HIGH);
  while (fetx_io_resolve(&io) == 0) {
  }
  printf("Set input, output: %u\n", fetx_io_output(io, 0));

  fetx_io_delete(io);
  return 0;
}

int fetx_example_from_file(const char *const netlist_pathname,
                           const char *const vector_pathname) {

  struct fetx_netlist nl;
  enum fetx_errs errs = fetx_netlist_from_file(&nl, netlist_pathname);
  if (errs != FETX_ERR_NONE) {
    printf("Failed to read netlist file %d\n", errs);
    return -1;
  }

  struct fetx_vector input_vec;
  errs = fetx_vector_from_file(&input_vec, vector_pathname);
  if (errs != FETX_ERR_NONE) {
    printf("Failed to read vector file %d\n", errs);
    fetx_netlist_delete(nl);
    return -1;
  }

  /* check width of vector */
  if (input_vec.width != nl.inputs_size) {
    puts("Netlist io does not match vector");
    fetx_netlist_delete(nl);
    fetx_vector_delete(input_vec);
    return -1;
  }

  struct fetx_vector output_vec = {.width = nl.outputs_size,
                                   .length = input_vec.length};

  /* allocate vector to store output states */
  if (fetx_vector_new(&output_vec) != 0) {
    puts("Failed to allocate output vector");
    fetx_netlist_delete(nl);
    fetx_vector_delete(input_vec);
    return -1;
  }

  /* simulate */
  struct fetx_sim_res res;
  errs = fetx_vector_sim(&res, output_vec, nl, input_vec, 0);
  if (errs != FETX_ERR_NONE) {
    printf("Simulation failed: %u\n", errs);
    fetx_netlist_delete(nl);
    fetx_vector_delete(input_vec);
    fetx_vector_delete(output_vec);
    return -1;
  }

  fetx_vector_print(input_vec);
  puts("");
  fetx_vector_print(output_vec);

  /* clean up */
  fetx_netlist_delete(nl);
  fetx_vector_delete(input_vec);
  fetx_vector_delete(output_vec);
  return 0;
}

int main(int argc, char **argv) {
  switch (argc) {
  case 1:
    return fetx_example_inverter();
  case 3:
    return fetx_example_from_file(argv[1], argv[2]);
  default:
    puts("Incorrect number of arguments. fetx_example takes 0 or 2 "
         "arguments\n1: The netlist pathname\n2: The test vector pathname\n");
    return -1;
  }
  return 0;
}
