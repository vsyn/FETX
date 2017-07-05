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

#include "fetx_netlist.h"

#include <stdio.h>

void fetx_netlist_delete(struct fetx_netlist nl) {
  fetx_fetlist_delete(nl.fl);
  if (nl.inputs != 0) {
    fetx_dealloc(nl.inputs);
  }
  if (nl.outputs != 0) {
    fetx_dealloc(nl.outputs);
  }
}

enum fetx_errs fetx_netlist_new(struct fetx_netlist *const nl) {
  nl->inputs = fetx_alloc(sizeof(*nl->inputs), nl->inputs_size);
  if (nl->inputs == 0) {
    return FETX_ERR_ALLOC;
  }
  nl->outputs = fetx_alloc(sizeof(*nl->outputs), nl->outputs_size);
  if (nl->outputs == 0) {
    return FETX_ERR_ALLOC;
  }
  return (fetx_fetlist_new(&nl->fl) != 0) ? FETX_ERR_ALLOC : FETX_ERR_NONE;
}

void fetx_netlist_assign_fet(struct fetx_netlist *const nl,
                             const struct fetx_fetlist_fet fet,
                             const size_t index) {
  nl->fl.fets[index] = fet;
}

void fetx_netlist_assign_input(struct fetx_netlist *const nl,
                               const size_t node_index, const size_t index) {
  nl->inputs[index] = node_index;
}

void fetx_netlist_assign_output(struct fetx_netlist *const nl,
                                const size_t node_index, const size_t index) {
  nl->outputs[index] = node_index;
}

void fetx_netlist_update_nodes_size(struct fetx_netlist *const nl) {
  nl->nodes_size = fetx_fetlist_find_last_node(nl->fl) + 1;
}

enum fetx_netlist_line_type {
  fetx_netlist_line_unknown,
  fetx_netlist_line_inputs,
  fetx_netlist_line_outputs,
  fetx_netlist_line_fet
};

/* reset the sizes in \nl after allocating and re-use them as an index */

static int fetx_netlist_file_stride(struct fetx_netlist *const nl,
                                    FILE *const fd) {
  enum fetx_netlist_line_type type = fetx_netlist_line_unknown;
  unsigned char nw = 0;
  unsigned char count = 0;

  while (1) {
    const int c = fgetc(fd);
    if ((c >= '0') && (c <= '9')) {
      nw = 1;
    } else {
      /* read value in */
      if (nw != 0) {
        if (type == fetx_netlist_line_fet) {
          if (count == 2) {
            ++nl->fl.size;
            count = 0;
          } else {
            ++count;
          }
        } else if (type == fetx_netlist_line_inputs) {
          ++nl->inputs_size;
        } else if (type == fetx_netlist_line_outputs) {
          ++nl->outputs_size;
        } else {
          return -1;
        }
        nw = 0;
      }
      if (c == EOF) {
        break;
      } else if ((c != ' ') && (c != '\t') && (c != '\n') && (c != '\r')) {
        if (count != 0) {
          return -1;
        }
        if ((c == 'p') || (c == 'n')) {
          type = fetx_netlist_line_fet;
        } else if (c == 'i') {
          type = fetx_netlist_line_inputs;
        } else if (c == 'o') {
          type = fetx_netlist_line_outputs;
        } else {
          return -1;
        }
      }
    }
  }
  return 0;
}

static int fetx_netlist_file_proc(struct fetx_netlist *const nl,
                                  FILE *const fd) {
  enum fetx_netlist_line_type type = fetx_netlist_line_unknown;
  enum fetx_fet_types fet_type;
  unsigned char nw = 0;
  unsigned char count = 0;
  size_t value = 0;

  while (1) {
    int c = fgetc(fd);
    if ((c >= '0') && (c <= '9')) {
      nw = 1;
      if (fetx_check_multiply(&value, 10, value) != 0) {
        return -1;
      }
      c -= '0';
      value += c;
      if (value < (unsigned int)c) {
        return -1;
      }
    } else {
      /* read value in */
      if (nw != 0) {
        if (type == fetx_netlist_line_fet) {
          nl->fl.fets[nl->fl.size].connections[count] = value;
          if (count == 2) {
            nl->fl.fets[nl->fl.size].type = fet_type;
            ++nl->fl.size;
            count = 0;
          } else {
            ++count;
          }
        } else if (type == fetx_netlist_line_inputs) {
          nl->inputs[nl->inputs_size] = value;
          ++nl->inputs_size;
        } else if (type == fetx_netlist_line_outputs) {
          nl->outputs[nl->outputs_size] = value;
          ++nl->outputs_size;
        } else {
          return -1;
        }
        nw = 0;
        value = 0;
      }

      if (c == EOF) {
        break;
      } else if ((c != ' ') && (c != '\t') && (c != '\n') && (c != '\r')) {
        if (c == 'p') {
          fet_type = FETX_FET_P;
          type = fetx_netlist_line_fet;
        } else if (c == 'n') {
          fet_type = FETX_FET_N;
          type = fetx_netlist_line_fet;
        } else if (c == 'i') {
          type = fetx_netlist_line_inputs;
        } else if (c == 'o') {
          type = fetx_netlist_line_outputs;
        } else {
          return -1;
        }
      }
    }
  }
  return 0;
}

enum fetx_errs fetx_netlist_from_file(struct fetx_netlist *const nl,
                                      const char *const pathname) {
  FILE *const fd = fopen(pathname, "r");
  if (fd == 0) {
    return FETX_ERR_FOPEN;
  }

  nl->fl.size = 0;
  nl->inputs_size = 0;
  nl->outputs_size = 0;

  if (fetx_netlist_file_stride(nl, fd) != 0) {
    return (fclose(fd) != 0) ? FETX_ERR_FFORMAT | FETX_ERR_FCLOSE
                             : FETX_ERR_FFORMAT;
  }

  enum fetx_errs errs = fetx_netlist_new(nl);
  if (errs != FETX_ERR_NONE) {
    return (fclose(fd) != 0) ? errs | FETX_ERR_FCLOSE : errs;
  }

  if (fseek(fd, 0, SEEK_SET) != 0) {
    return (fclose(fd) != 0) ? FETX_ERR_IO | FETX_ERR_FCLOSE : FETX_ERR_IO;
  }

  nl->fl.size = 0;
  nl->inputs_size = 0;
  nl->outputs_size = 0;

  if (fetx_netlist_file_proc(nl, fd) != 0) {
    return (fclose(fd) != 0) ? FETX_ERR_FFORMAT | FETX_ERR_FCLOSE
                             : FETX_ERR_FFORMAT;
  }

  if (fclose(fd) != 0) {
    return FETX_ERR_FCLOSE;
  }

  fetx_netlist_update_nodes_size(nl);
  return FETX_ERR_NONE;
}

enum fetx_errs fetx_netlist_to_fd(const struct fetx_netlist nl,
                                  FILE *const fd) {
  /* inputs */
  if (fprintf(fd, "i") < 0) {
    return FETX_ERR_IO;
  }

  size_t f = 0;
  while (f < nl.inputs_size) {
    if (fprintf(fd, " %llu", (unsigned long long int)nl.inputs[f]) < 0) {
      return FETX_ERR_IO;
    }
    ++f;
  }

  /* outputs */
  if (fprintf(fd, "\no") < 0) {
    return FETX_ERR_IO;
  }

  f = 0;
  while (f < nl.outputs_size) {
    if (fprintf(fd, " %llu", (unsigned long long int)nl.outputs[f]) < 0) {
      return FETX_ERR_IO;
    }
    ++f;
  }

  if (fprintf(fd, "\n") < 0) {
    return FETX_ERR_IO;
  }

  f = 0;
  while (f < nl.fl.size) {
    if (fprintf(fd, "%c %llu %llu %llu\n",
                (nl.fl.fets[f].type == 0) ? 'n' : 'p',
                (long long unsigned int)nl.fl.fets[f].connections[0],
                (long long unsigned int)nl.fl.fets[f].connections[1],
                (long long unsigned int)nl.fl.fets[f].connections[2]) < 0) {
      return FETX_ERR_IO;
    }
    ++f;
  }
  return FETX_ERR_NONE;
}

enum fetx_errs fetx_netlist_to_file(const struct fetx_netlist nl,
                                    const char *const pathname) {
  FILE *const fd = fopen(pathname, "w");
  if (fd == 0) {
    return FETX_ERR_FOPEN;
  }

  enum fetx_errs errs = fetx_netlist_to_fd(nl, fd);

  if (fclose(fd) != 0) {
    errs |= FETX_ERR_FCLOSE;
  }
  return errs;
}

enum fetx_errs fetx_netlist_print(const struct fetx_netlist nl) {
  return fetx_netlist_to_fd(nl, stdout);
}
