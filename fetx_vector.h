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

#ifndef FETX_VECTOR_H
#define FETX_VECTOR_H

#include "fetx_io.h"

struct fetx_vector {
  enum fetx_node_states **values;
  size_t width;
  size_t length;
};

struct fetx_sim_res {
  unsigned long int multiply_driven;
  unsigned long int time;
};

void fetx_vector_delete(struct fetx_vector v);
enum fetx_errs fetx_vector_new(struct fetx_vector *const v);
enum fetx_errs fetx_vector_split(struct fetx_vector *const sub,
                                 const struct fetx_vector v,
                                 const size_t start);

enum fetx_errs fetx_vector_sim(struct fetx_sim_res *const res,
                               struct fetx_vector output_vector,
                               const struct fetx_netlist nl,
                               const struct fetx_vector input_vector,
                               const unsigned long int time_limit);

enum fetx_errs fetx_vector_from_file(struct fetx_vector *const v,
                                     const char *const pathname);
enum fetx_errs fetx_vector_to_file(struct fetx_vector v,
                                   const char *const pathname);
enum fetx_errs fetx_vector_print(struct fetx_vector v);

#endif
