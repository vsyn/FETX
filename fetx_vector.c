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

#include "fetx_vector.h"

#include <stdio.h>

static size_t fetx_vector_new_size(const size_t width, const size_t length) {
  /* returns alloc size for the input/output vectors
   * (width * sizeof (enum node_states*)) + (width * length *
   * sizeof (enum node_states))
   */
  size_t alloc_size;
  if (fetx_check_multiply(&alloc_size, length,
                          sizeof(enum fetx_node_states *))) {
    return 0;
  }

  size_t tmp_alloc_size;
  if (fetx_check_multiply(&tmp_alloc_size, width, length)) {
    return 0;
  }
  if (fetx_check_multiply(&tmp_alloc_size, sizeof(enum fetx_node_states),
                          tmp_alloc_size)) {
    return 0;
  }

  alloc_size += tmp_alloc_size;
  if (alloc_size < tmp_alloc_size) {
    return 0;
  }
  return alloc_size;
}

void fetx_vector_delete(struct fetx_vector v) { fetx_dealloc(v.values); }

enum fetx_errs fetx_vector_new(struct fetx_vector *const v) {
  v->values = fetx_alloc(1, fetx_vector_new_size(v->width, v->length));
  if (v->values == 0) {
    return FETX_ERR_ALLOC;
  }
  enum fetx_node_states *states =
      (enum fetx_node_states *)(v->values + v->length);
  size_t time = 0;
  while (time < v->length) {
    v->values[time] = states;
    states += v->width;
    ++time;
  }
  return FETX_ERR_NONE;
}

enum fetx_errs fetx_vector_split(struct fetx_vector *const sub,
                                 const struct fetx_vector v,
                                 const size_t start) {
  sub->values = fetx_alloc(sub->length, sizeof(enum fetx_node_states *));
  if (sub->values == 0) {
    return FETX_ERR_ALLOC;
  }

  size_t time = 0;
  while (time < sub->length) {
    sub->values[time] = v.values[time] + start;
    ++time;
  }
  return FETX_ERR_NONE;
}

enum fetx_errs fetx_vector_sim(struct fetx_sim_res *const res,
                               struct fetx_vector output_vector,
                               const struct fetx_netlist nl,
                               const struct fetx_vector input_vector,
                               const unsigned long int time_limit) {
  if ((input_vector.length != output_vector.length) ||
      (input_vector.width != nl.inputs_size) ||
      (output_vector.width != nl.outputs_size)) {
    return FETX_ERR_PARAM;
  }

  struct fetx_io io;
  if (fetx_io_init(&io, nl) != 0) {
    return FETX_ERR_ALLOC;
  }

  unsigned long int time = 0;
  unsigned long int multiply_driven = 0;

  size_t t = 0;
  while (t < input_vector.length) {
    fetx_io_inputs(&io, input_vector.values[t]);

    while (fetx_io_resolve(&io) == 0) {
      ++time;
      if ((time_limit != 0) && (time > time_limit)) {
        fetx_io_delete(io);
        res->multiply_driven = multiply_driven;
        res->time = time;
        return FETX_ERR_TIMEOUT;
      }
    }

    multiply_driven += fetx_multiple_drive_detect(io.fx);

    fetx_io_outputs(output_vector.values[t], io);
    ++t;
  }

  res->multiply_driven = multiply_driven;
  res->time = time;

  fetx_io_delete(io);
  return FETX_ERR_NONE;
}

static int fetx_vector_file_stride_eol(struct fetx_vector *const v,
                                       const size_t tmp_width) {
  if (tmp_width != 0) {
    if (v->width == 0) {
      v->width = tmp_width;
    } else if (tmp_width != v->width) {
      return -1;
    }
    ++(v->length);
  }
  return 0;
}

static int fetx_vector_file_stride(struct fetx_vector *const v,
                                   FILE *const fd) {
  size_t tmp_width = 0;
  v->length = 0;
  v->width = 0;
  while (1) {
    const int c = fgetc(fd);
    if (c == EOF) {
      return fetx_vector_file_stride_eol(v, tmp_width);
    } else if ((c <= ('0' + FETX_UNDRIVEN)) && (c >= '0')) {
      ++tmp_width;
    } else if (c == '\n') {
      if (fetx_vector_file_stride_eol(v, tmp_width) != 0) {
        return -1;
      }
      tmp_width = 0;
    } else if ((c != ' ') && (c != '\r') && (c != '\t')) {
      return -1;
    }
  }
  return 0;
}

static void fetx_vector_file_proc(struct fetx_vector *const v, FILE *const fd) {
  size_t index = 0;
  size_t time = 0;
  while (1) {
    const int c = fgetc(fd);
    if (c == EOF) {
      break;
    } else if ((c <= ('0' + FETX_UNDRIVEN)) && (c >= '0')) {
      v->values[time][index] = c - '0';
      ++index;
    } else if (c == '\n') {
      index = 0;
      ++time;
    }
  }
  return;
}

enum fetx_errs fetx_vector_from_file(struct fetx_vector *const v,
                                     const char *const pathname) {
  /* get i/o width and length from file */
  FILE *fd = fopen(pathname, "r");
  if (fd == 0) {
    return FETX_ERR_FOPEN;
  }

  /* verify file and get width and length */
  if (fetx_vector_file_stride(v, fd) != 0) {
    return (fclose(fd) != 0) ? FETX_ERR_FFORMAT | FETX_ERR_FCLOSE
                             : FETX_ERR_FFORMAT;
  }

  /* alloc memory for vector */
  if (fetx_vector_new(v) != 0) {
    return FETX_ERR_ALLOC;
  }

  /* initialise vector */
  if (fseek(fd, 0, SEEK_SET) != 0) {
    return (fclose(fd) != 0) ? FETX_ERR_IO | FETX_ERR_FCLOSE : FETX_ERR_IO;
  }

  fetx_vector_file_proc(v, fd);

  if (fclose(fd) != 0) {
    fetx_vector_delete(*v);
    return FETX_ERR_FCLOSE;
  }

  return FETX_ERR_NONE;
}

enum fetx_errs fetx_vector_to_fd(struct fetx_vector v, FILE *const fd) {
  size_t time = 0;
  while (time < v.length) {
    size_t index = 0;
    while (index < v.width) {
      if (fputc(v.values[time][index] + '0', fd) == EOF) {
        return FETX_ERR_IO;
      }
      ++index;
    }
    if (fputc('\n', fd) == EOF) {
      return FETX_ERR_IO;
    }
    ++time;
  }
  return FETX_ERR_NONE;
}

enum fetx_errs fetx_vector_to_file(struct fetx_vector v,
                                   const char *const pathname) {
  FILE *fd = fopen(pathname, "w");
  if (fd == 0) {
    return FETX_ERR_FOPEN;
  }

  enum fetx_errs errs = fetx_vector_to_fd(v, fd);
  if (fclose(fd) != 0) {
    errs |= FETX_ERR_FCLOSE;
  }
  return errs;
}

enum fetx_errs fetx_vector_print(struct fetx_vector v) {
  return fetx_vector_to_fd(v, stdout);
}
