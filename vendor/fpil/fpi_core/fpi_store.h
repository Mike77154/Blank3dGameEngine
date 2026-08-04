#ifndef FPI_STORE_H
#define FPI_STORE_H

#include "fpi_polysym.h"

#ifndef FPI_STORE_MAX_ENTRIES
#define FPI_STORE_MAX_ENTRIES 256
#endif
#ifndef FPI_STORE_POOL_BYTES
#define FPI_STORE_POOL_BYTES 16384UL
#endif

typedef struct FPI_StoreEntry {
    int used;
    FPI_U32 name_offset;
    FPI_U32 hash;
    FPI_Fixed value;
} FPI_StoreEntry;

typedef struct FPI_Store {
    char pool[FPI_STORE_POOL_BYTES];
    FPI_U32 pool_used;
    FPI_StoreEntry entries[FPI_STORE_MAX_ENTRIES];
    int count;
} FPI_Store;

void fpi_store_init(FPI_Store* store);
int fpi_store_set(FPI_Store* store, const char* name, FPI_Fixed value, FPI_Error* error);
int fpi_store_get(const FPI_Store* store, const char* name, FPI_Fixed* out_value);

#endif /* FPI_STORE_H */
