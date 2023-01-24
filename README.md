<!---
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
--->

# FETX

## Simulation of Symmetrical FET Networks

The API provides methods for dynamically creating circuits or loading them from netlist files.

There are 2 structures that represent a circuit/network. The `fetx_netlist` structure can be directly translated from an input file or can be programmatically generated. The `fetx_io` structure contains run-time data and is initialised from the `fetx_netlist` structure.

`make` in the root directory will build the library.

## Tests

`make test` will compile and run the tests.

## Example Program

Example code using the library can be found in the `examples/` directory. Example netlists can be found in the `netlists/` directory, and vectors to exercise/test them can be found in the `vectors/` directory.

`make example` will compile an example program which takes 0 or 2 arguments. If 0, it programmatically generates an inverter and exercises it, if 2 it reads a netlist from the file specified by the first argument, then exercises it with a vector from the file  specified by the second argument.

```
$ ./bin/fetx_example ./netlists/nand.nl vectors/pg_2in.vct
0100
0101
0100
0110
0111
0101
0111
0110
0100

1
1
1
1
0
1
0
1
```

## Limitations

FETX does not take into account complimentary pairs whose gates are not directly attached to one another. It also does not correctly resolve every circumstance where a FET is indirectly connected to its own gate.

## Netlists

The `fetx_netlist` structure contains a list of FETs and associated inputs and outputs. The inputs, outputs and FET connections are specified as node indices.

```
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

struct fetx_netlist {
  struct fetx_fetlist fl;
  size_t *inputs;
  size_t *outputs;
  size_t nodes_size;
  size_t inputs_size;
  size_t outputs_size;
};
```

### Functions

`void fetx_netlist_delete(struct fetx_netlist nl);`

Deallocates the memory associated with the netlist `nl`.

`enum fetx_errs fetx_netlist_new(struct fetx_netlist *const nl);`

Allocates memory for the netlist `nl` based on it's `inputs_size`, `outputs_size` and `fl.size` members, all of which should be initialised before calling the function.

Returns (a combination of):
* `FETX_ERR_ALLOC` A memory allocation error occurred.
* `FETX_ERR_NONE` Netlist allocated successfully.

`void fetx_netlist_assign_fet(struct fetx_netlist *const nl, const struct fetx_fetlist_fet fet, const size_t index);`

Copies `fet` onto the netlist `nl` at index `index`.

`void fetx_netlist_assign_input(struct fetx_netlist *const nl, const size_t node_index, const size_t index);`

Assigns the node at `node_index` as an input and place it at position `index` in the input array of netlist `nl`.

`void fetx_netlist_assign_output(struct fetx_netlist *const nl, const size_t node_index, const size_t index);`

Assigns the node at `node_index` as an output and place it at position `index` in the output array of netlist `nl`.

`void fetx_netlist_update_nodes_size(struct fetx_netlist *const nl);`

After all the FETs have been assigned, this function can be called to work out the size of the node array, a necessary step before generating the `fetx_io` struct from `nl`. This can be done manually by setting the `nodes_size` member if the number is already known, it should be the largest node index + 1.

`enum fetx_errs fetx_netlist_from_file(struct fetx_netlist *const nl, const char *const pathname);`

Populates the netlist `nl` from the file at `pathname`, see below or the `netlists/` directory for file formats.

Returns (a combination of):
* `FETX_ERR_ALLOC` A memory allocation error occurred.
* `FETX_ERR_IO` An input error occurred.
* `FETX_ERR_FOPEN` Failed to open file.
* `FETX_ERR_FFORMAT` Incorreect file format.
* `FETX_ERR_FCLOSE` Failed to close file.
* `FETX_ERR_NONE` Netlist read successfully.

`enum fetx_errs fetx_netlist_to_file(const struct fetx_netlist nl, const char *const pathname);`

Generates a file `pathanme` from the netlist `nl`, see below or the `netlists/` directory for file formats.

Returns (a combination of):
* `FETX_ERR_IO` An output error occurred.
* `FETX_ERR_FOPEN` Failed to open file.
* `FETX_ERR_FCLOSE` Failed to close file.
* `FETX_ERR_NONE` Netlist written successfully.

`enum fetx_errs fetx_netlist_print(const struct fetx_netlist nl);`

Prints the netlist `nl`.

Returns (a combination of):
* `FETX_ERR_IO` An output error occurred.
* `FETX_ERR_NONE` Netlist written successfully.

## Runtime

The `fetx_io` structure contains the runtime data along with associated inputs and outputs.

```
struct fetx_io {
  struct fetx fx;
  struct fetx_input_node *inputs;
  struct fetx_node **outputs;
  size_t inputs_size;
  size_t outputs_size;
};
```

### Functions

`void fetx_io_delete(struct fetx_io io);`

Deallocates the memory associated with the runtime struct `io`.

`int fetx_io_init(struct fetx_io *const io, const struct fetx_netlist nl);`

Initialises the `fetx_io` struct `io` from the netlist `nl`. After calling this the netlist `nl` can be disposed of.

Returns `-1` if there was a memory allocation error, `0` otherwise.

`void fetx_io_input(struct fetx_io *const io, const size_t input_index, const enum fetx_node_states state);`

Sets the state of the node at index `input_index` in the input array if `io` to state `state`.

`enum fetx_node_states fetx_io_output(const struct fetx_io io, const size_t output_index);`

Gets the state of the node at index `output_index` in the output array of `io`.

`void fetx_io_inputs(struct fetx_io *const io, const enum fetx_node_states *const inputs);`

Sets the state of all the nodes on the input array of `io` to the states in the `inputs` array.

`void fetx_io_outputs(enum fetx_node_states *const outputs,
                     const struct fetx_io io);`

Gets the state of all the nodes on the output array of `io` and writes them to the `outputs` array.

`unsigned char fetx_io_resolve(struct fetx_io *const io);`

Once the inputs have been set to the desired states, this function will attempt to incrementally resolve the network. Note that some networks may oscillate indefinitely and never resolve.

Returns `1` when the network has resolved, otherwise `0`.

## File Formats

### Netlists

All numbers are decimal node indexes, the FET connections are ordered gate, source, drain (although the source and drain are interchangeable).

#### Line Types

* i - input nodes
* o - output nodes
* p - p channel FET
* n - n channel FET

Example, a nand gate:

```
i 0 1 2 3
o 4
p 2 1 4
p 3 1 4
n 2 4 5
n 3 5 0
```

### Vectors

Vectors are 2 dimensional arrays of node states that co-respond to the states of the input or output nodes of a netlist. They can be loaded from a file and are useful for running fixed tests.

State	      		| File Representation
----------------|----------------------------
Low			|	0
High   			|	1
Unstable low 		| 	2
Unstable high 		| 	3
Unstable multiple 	| 	4
Undriven 		| 	5


Each line represents states at a particular time instance.

Example, for exercising a 2 input gate:

```
01 00
01 01
01 00
01 10
01 11
01 01
01 11
01 10
01 00
```
