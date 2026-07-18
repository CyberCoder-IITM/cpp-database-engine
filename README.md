# cpp-database-engine

A relational database engine written from scratch in C++20 — storage layer (LRU buffer pool, heap-file page
storage, B+Tree index) plus a small query layer (relational operators and a histogram-based cardinality
estimator) on top.

## Overview

This project covers both the storage and query layers of a database system: how pages move between disk and
memory, how tuples are packed into fixed-size pages, how a B+Tree index keeps sorted data quickly searchable, and
how relational operators and selectivity estimation are built on top of that storage layer. It's built around a
small set of composable pieces rather than a single monolithic class:

- **BufferPool** — an LRU cache of in-memory pages, responsible for eviction, dirty-page tracking, and flushing
  writes back to disk.
- **Database** — a registry that owns and looks up the on-disk files (`DbFile` implementations) by name.
- **Tuple** — a typed row representation, serialized to and from fixed-size page slots.
- **HeapFile / HeapPage** — unordered on-disk storage: pages of slotted tuples, with insert/delete/iteration.
- **BTreeFile / IndexPage / LeafPage** — an ordered on-disk index: internal pages route by key, leaf pages hold
  sorted tuples, with page splitting on overflow.
- **Query operators** (`projection`, `filter`, `join`, `aggregate`) — relational operators that read from one
  `DbFile` and write into another: field projection, predicate filtering, nested-loop join, and grouped
  SUM/AVG/MIN/MAX/COUNT aggregation.
- **ColumnStats** — a fixed-width histogram over an integer column, used to estimate the cardinality of a
  selection predicate for query planning.

## Status

- **Done:** buffer pool (LRU eviction, dirty tracking, flush-to-disk), the `Database` file registry, heap-file
  storage (tuple serialization, page-level insert/delete, full-file iteration), the B+Tree index (`BTreeFile`,
  `IndexPage`, `LeafPage` — sorted insertion, node splitting, and leaf traversal), the query operators
  (`projection`, `filter`, `join`, `aggregate`), and the histogram-based cardinality estimator (`ColumnStats`).
  Compiles cleanly end to end, including all test files; the full `ctest` suite has not yet been run against this
  build.

## Documentation

Design notes for each component live under `docs/`:

- [Buffer Pool & Database](docs/pa0.md)
- [Tuple & Heap File Storage](docs/pa1.md)
- [B+Tree Index](docs/pa2.md)
- [Query Operators](docs/pa3.md)
- [Cardinality Estimation](docs/pa4.md)

## Building

### Environment setup

**Linux / WSL:**

```bash
sudo apt update
sudo apt upgrade -y
sudo apt install -y g++ build-essential gdb make cmake valgrind
```

**macOS:**

```zsh
clang --version   # check if already installed
xcode-select --install   # if not
```

### Recommended IDEs

- [CLion](https://www.jetbrains.com/clion/) — open the folder and load the CMake project directly.
- [VS Code](https://code.visualstudio.com/) — install the
  [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) and
  [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) extensions.

### Build and test from the terminal

```sh
git clone https://github.com/CyberCoder-IITM/cpp-database-engine.git
cd cpp-database-engine
mkdir build && cd build
cmake ..
make
ctest
```
