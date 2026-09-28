# ICCAD 2023 CAD Contest – Problem A: Multi-bit Large-scale Boolean Matching

A SAT-based Boolean matcher written for the ICCAD 2023 CAD Contest, Problem A
(Multi-bit Large-scale Boolean Matching, sponsored by Cadence).

Given two combinational gate-level Verilog netlists and their bus declarations,
the tool searches for an input/output port correspondence (with optional
negation and constant binding) under which the two circuits become functionally
equivalent, and tries to maximize the number of matched output ports within a
time budget.

## Approach

The pipeline is:

1. **Parse & build AIGs.** Each Verilog file is read with ABC, structurally
   hashed (`strash`) and written out as an AIGER file
   (`src/parser.cpp`). The AIGs are loaded into the in-house `AIG` class
   (`src/AIG.cpp`), which also computes structural / functional support
   (via ABC) and NP-symmetry information used for pruning.

2. **Two-step matching** (`src/TwoStep*.cpp`). This is the main solver and
   runs as a nested search:

   * **Output solver** (`src/TwoStepOutput.cpp`) – incrementally picks the
     next output pair `(f_i, g_j)` using heuristics (support size, output bus
     signatures, output groups with identical characteristics) and keeps a
     backtrack stack. When the two circuits have a different number of
     outputs it enables *projection* (several outputs of one circuit may map
     to one output of the other).
   * **Input solver** (`src/TwoStepInput.cpp`) – for the current set of
     output pairs `R`, it reduces both AIGs to the cones of the chosen
     outputs and encodes the input mapping space as a CNF: one Boolean
     variable per `(x_i, y_j)`, `(x_i', y_j)`, `(0, y_j)`, `(1, y_j)`
     choice, with at-most-one / cardinality constraints, functional-support
     pruning, symmetry constraints and (optionally) bus constraints.
     The CNF is solved with CaDiCaL; every candidate mapping is checked with
     a miter (`src/TwoStepMiter.cpp`). A counterexample is turned into a
     blocking clause (using a non-redundant subset of the differing inputs)
     that shrinks the mapping space, and the loop repeats – a CEGAR-style
     refinement.
   * Every time a full input match is found for `R`, the result is written
     to the output file if it beats the best score so far
     (`parseOutput` in `src/parser.cpp`). The search continues until all
     outputs are matched or the time budget expires.

3. **Multiple passes.** `src/main.cpp` runs the two-step matcher up to three
   times with different bus settings (`busType` 0 → bus constraints on,
   1 → input bus off, 2 → all bus constraints off) so that a wrong or
   inconsistent bus declaration cannot block a solution.

An earlier simulation-based approach (`LargeScale` / `Partition`,
`src/largeScale.cpp`, `src/Partition.cpp`) that refines input/output
partitions by random simulation is still in the tree but is disabled in
`main.cpp`.

## Repository layout

```
.
├── Makefile              # builds ./main and the third-party libraries
├── src/
│   ├── main.cpp          # entry point
│   ├── parser.*          # contest input / output format, Verilog -> AIG via ABC
│   ├── AIG.*             # AIG data structure, support / symmetry analysis
│   ├── CNF.*             # CNF container + CaDiCaL wrapper
│   ├── aigtocnf.*        # AIGER -> CNF (Tseitin)
│   ├── abcTool.*         # helpers that call into libabc
│   ├── TwoStep.h/.cpp    # top-level two-step matching loop
│   ├── TwoStepOutput.cpp # output-pair selection / backtracking
│   ├── TwoStepInput.cpp  # input mapping CNF, bus clauses, space reduction
│   ├── TwoStepMiter.cpp  # miter construction and counterexample handling
│   ├── largeScale.*      # legacy simulation-based matcher (disabled)
│   ├── Partition.*       # legacy partition refinement (disabled)
│   └── utility.*         # timing / statistics / ABC command helpers
├── lib/
│   ├── abc/              # git submodule: ABC (fork)
│   ├── cadical/          # git submodule: CaDiCaL SAT solver
│   └── aiger/            # git submodule: AIGER library
├── test/                 # GoogleTest unit tests (AIG, Partition, LargeScale)
├── results/              # result files of contest cases (reference only)
├── circuit_1.v, circuit_2.v, input.txt   # sample case
└── LICENSE               # MIT
```

## Requirements

* Linux, `g++` with C++17 support, GNU `make`
* `gcc` (for `lib/aiger/aiger.c`)
* GoogleTest (`libgtest`) – only for the unit tests in `test/`

## Build

Clone with submodules:

```bash
git clone --recurse-submodules <repo-url>
cd ICCAD-2023-Multi-bit-Large-scale-Boolean-Matching
# or, if already cloned:
git submodule update --init --recursive
```

Build everything (third-party libraries are built once and copied into
`lib/`):

```bash
make -j
```

This produces the `main` executable. `make clean` removes the objects and the
binary; the prebuilt `lib/*.a` files are kept. `make objs` compiles only the
object files (used by the test build).

## Usage

```bash
./main <input.txt> <output.txt>
```

`make run` runs `./main input.txt output.txt` on the sample case and deletes
the intermediate files (`*.aig`, `*.aag`, `*.cnf`, `stdoutOutput.txt`,
`optimizeAIG/`) that are generated in the working directory.

The program keeps writing the best result found so far to `<output.txt>`,
so it can be stopped at any time. The time budget is `maxRunTime` in
`src/TwoStep.h`.

### Input format

```
<circuit_1.v>
<number of buses in circuit 1>
<bus size> <port> <port> ...
...
<circuit_2.v>
<number of buses in circuit 2>
<bus size> <port> <port> ...
...
```

See `input.txt` for a complete example. Paths are resolved relative to the
current working directory.

### Output format

```
INGROUP
1 + <circuit-1 input>
2 + <circuit-2 input>
2 - <circuit-2 input>      # '-' = matched with negation
END
OUTGROUP
1 + <circuit-1 output>
2 + <circuit-2 output>
END
CONSTGROUP
+ <circuit-2 input>        # bound to constant (see parseOutput in src/parser.cpp)
- <circuit-2 input>
END
```

## Tests

Unit tests live in `test/` and use GoogleTest:

```bash
make test        # or: cd test && make
```

Each `test/*_test.cpp` is compiled, linked against the project objects and
the third-party libraries, and executed.

## Configuration knobs

Most tuning is done through constants in the source:

| Where | What |
|-------|------|
| `src/TwoStep.h` – `maxRunTime` | overall time budget (ms) |
| `src/TwoStep.h` – `enableInputBus`, `enableOutputBus` | default bus-constraint switches |
| `src/TwoStep.h` – `verbose` | debug printing |
| `src/utility.h` – `simplify` | ABC script applied after `strash` |
| `Makefile` – `DEFINE` | `-DDBG` enables self-checks, `-DINF` enables progress output |

## Acknowledgements

* [ABC](https://github.com/berkeley-abc/abc) – logic synthesis and AIG
  manipulation
* [CaDiCaL](https://github.com/arminbiere/cadical) – SAT solver used for the
  mapping space and miters
* [AIGER](https://github.com/arminbiere/aiger) – AIGER file I/O

## License

This project is released under the MIT License, see `LICENSE`. The third-party
libraries in `lib/` keep their own licenses.
