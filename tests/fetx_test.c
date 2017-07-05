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
#include <time.h>

int vector_compare(const struct fetx_vector a, const struct fetx_vector b) {
  if ((a.length != b.length) || (a.width != b.width)) {
    return -1;
  }
  size_t time = 0;
  while (time < a.length) {
    size_t index = 0;
    while (index < a.width) {
      if (a.values[time][index] != b.values[time][index]) {
        return 1;
      }
      ++index;
    }
    ++time;
  }
  return 0;
}

int fetx_test(const char *const netlist_pathname,
              const char *const vector_pathname,
              unsigned long int multiply_driven, unsigned long int time_limit) {

  struct fetx_netlist nl;
  int ret = fetx_netlist_from_file(&nl, netlist_pathname);
  if (ret != 0) {
    printf("Failed to read netlist file %d\n", ret);
    return -1;
  }

  struct fetx_vector vec;
  ret = fetx_vector_from_file(&vec, vector_pathname);
  if (ret != 0) {
    printf("Failed to read vector file %d\n", ret);
    fetx_netlist_delete(nl);
    return -1;
  }

  /* check width of vector */
  if (vec.width != (nl.inputs_size + nl.outputs_size)) {
    puts("Netlist io does not match vector");
    fetx_netlist_delete(nl);
    fetx_vector_delete(vec);
    return -1;
  }

  struct fetx_vector input_vec = {
      .values = vec.values, .width = nl.inputs_size, .length = vec.length};
  struct fetx_vector correct_vec = {.width = vec.width - nl.inputs_size,
                                    .length = vec.length};

  if (fetx_vector_split(&correct_vec, vec, nl.inputs_size) != 0) {
    fetx_netlist_delete(nl);
    fetx_vector_delete(vec);
  }

  struct fetx_vector output_vec = {.width = correct_vec.width,
                                   .length = vec.length};

  if (fetx_vector_new(&output_vec) != 0) {
    /* clean up */
    fetx_netlist_delete(nl);
    fetx_vector_delete(vec);
    fetx_vector_delete(correct_vec);
    return -1;
  }

  /* simulate */

  struct fetx_sim_res res;
  enum fetx_errs errs =
      fetx_vector_sim(&res, output_vec, nl, input_vec, time_limit);
  if (errs != FETX_ERR_NONE) {
    printf("Simulation failed: %u\n", errs);
    fetx_netlist_delete(nl);
    fetx_vector_delete(output_vec);
    fetx_vector_delete(vec);
    fetx_vector_delete(correct_vec);
    return -1;
  }

  if (res.multiply_driven != multiply_driven) {
    puts("Expected:");
    fetx_vector_print(correct_vec);
    puts("Actual:");
    fetx_vector_print(output_vec);
    printf("Simulation failed: %lu multiply driven nodes detected\n",
           res.multiply_driven);
    fetx_netlist_delete(nl);
    fetx_vector_delete(output_vec);
    fetx_vector_delete(vec);
    fetx_vector_delete(correct_vec);
    return -1;
  }

  if (vector_compare(output_vec, correct_vec) != 0) {
    puts("Expected:");
    fetx_vector_print(correct_vec);
    puts("Actual:");
    fetx_vector_print(output_vec);
    puts("Simulation failed: Actual outputs do not match expected outputs");
    fetx_netlist_delete(nl);
    fetx_vector_delete(output_vec);
    fetx_vector_delete(vec);
    fetx_vector_delete(correct_vec);
    return -1;
  }

  printf("Test passed: %s %s\n", netlist_pathname, vector_pathname);

  /* clean up */
  fetx_netlist_delete(nl);
  fetx_vector_delete(output_vec);
  fetx_vector_delete(vec);
  fetx_vector_delete(correct_vec);
  return 0;
}

int main(int argc, char **argv) {
  unsigned long int multiply_driven = 0;
  switch (argc) {
  case 4:
    break;
  case 5:
    multiply_driven = strtol(argv[4], 0, 0);
    break;
  default:
    puts("Incorrect number of arguments. fetx-test takes 3 or 4 arguments\n"
         "1: The netlist pathname\n2: The test vector pathname\n"
         "3: The limit on time before the circuit resolves, in time instances\n"
         "4: The number of times inputs should be recorded as multiply driven "
         "(defaults to 0)");
    return -1;
  }

  fetx_test(argv[1], argv[2], multiply_driven, strtol(argv[3], 0, 0));

  return 0;
}
