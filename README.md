# libds: generic data structures in C

A small static library (`libds.a`) with three core containers, written in C11
with no dependencies. All three are generic: they store elements of any type
by value.

| Container | Header | Key ideas |
|---|---|---|
| Dynamic array | `ds_vector.h` | `realloc`, geometric growth, amortized O(1) push, binary search |
| Doubly linked list | `ds_list.h` | Pointer manipulation, O(1) insert/remove at a known node, in-place reverse |
| Hash map | `ds_hashmap.h` | FNV-1a hashing, separate chaining, power-of-two table, resize at load 0.75 |

## Complexity

| Operation | Vector | List | Hash map |
|---|---|---|---|
| Push back / front | O(1) amortized / O(n) | O(1) / O(1) | – |
| Pop back / front | O(1) / O(n) | O(1) / O(1) | – |
| Index access | O(1) | O(n/2) (walks from the nearer end) | – |
| Insert / erase at index | O(n) | O(n/2) to find, O(1) to link | – |
| Search | O(n), O(log n) if sorted | O(n) | O(1) average |
| Put / get / remove | – | – | O(1) average |

## Quick example

```c
#include "ds_hashmap.h"
#include "ds_vector.h"

ds_hashmap *ages = hm_create(sizeof(int), 0);
int age = 30;
hm_put(ages, "alice", &age);
int *a = hm_get(ages, "alice");          /* -> 30 */

ds_vector *v = vec_create(sizeof(double), 0);
double d = 3.14;
vec_push_back(v, &d);
double first = *(double *)vec_at(v, 0);

vec_destroy(v);
hm_destroy(ages);
```

Every fallible call returns a `ds_status` (`DS_OK`, `DS_ERR_NOMEM`,
`DS_ERR_RANGE`, ...). `ds_strerror()` turns it into a message.

## Build and run

Windows (MinGW):
```
mingw32-make test     # unit tests for all three containers
mingw32-make run      # word-frequency demo on examples/sample.txt
```

Linux / macOS:
```
make test
make run
make asan             # tests under AddressSanitizer + UBSan
```

Demo output (`examples/word_freq.c` uses all three containers):
```
180 words, 98 unique

Top 10:
   1. the             23
   2. a               11
   3. and             8
   ...
Last 5 words: list and a hash map

Hash map: 512 buckets, load factor 0.19, longest chain 2
```

## Implementation notes

### Vector
- Capacity doubles when full. n pushes cost fewer than 2n element copies in
  total, so push is amortized O(1). The tests check that a million pushes
  cause at most ~20 reallocations.
- `realloc` failure leaves the vector untouched; `capacity * elem_size` is
  checked for overflow before allocating.
- **Self-aliasing is handled.** `vec_push_back(v, vec_at(v, 0))` is safe even
  when the push reallocates the buffer the argument points into, and
  `vec_insert` accounts for the source shifting during `memmove`.
- `vec_bsearch` is a lower-bound search (returns the first of equal elements)
  and uses `lo + (hi - lo) / 2` to avoid overflow.

### Doubly linked list
- The element is stored inline after the node header in one allocation, using a
  C11 flexible array member of `max_align_t` so any element type is aligned.
- `list_at` walks from whichever end is closer.
- `list_reverse` swaps `prev`/`next` in every node, then swaps head and tail.
  It runs in O(n) with no allocation.
- Tested against a plain array with 50,000 random operations, walking the list
  both forwards and backwards to verify every link.

### Hash map
- **FNV-1a** 32-bit hash, verified against the published test vectors.
- **Power-of-two bucket count**, so `hash % n` becomes `hash & (n - 1)`.
- **Resize at load factor 0.75.** Entries cache their hash, so resizing relinks
  nodes without rehashing keys or copying data.
- **One allocation per entry**: `[next | hash | value | key\0]`. The value comes
  first, so it is aligned for any type.
- **Pointer-to-pointer lookup** (`find_link`) means insert and remove need no
  special case for the first entry in a bucket.
- Iteration lets you remove the entry just returned. `val_size = 0` turns the
  map into a hash set.

## Layout

```
include/   public headers (ds_common.h, ds_vector.h, ds_list.h, ds_hashmap.h)
src/       implementations
tests/     unit tests + tiny test framework (test.h)
examples/  word_freq.c demo + sample.txt
```
