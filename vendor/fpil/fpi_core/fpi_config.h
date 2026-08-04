#ifndef FPI_CONFIG_H
#define FPI_CONFIG_H

/* Compile-time capacities. Every overflow is reported; nothing is truncated. */
#ifndef FPI_MAX_RULES
#define FPI_MAX_RULES 256
#endif

#ifndef FPI_MAX_TERMS
#define FPI_MAX_TERMS 16384
#endif

#ifndef FPI_MAX_TERMS_PER_SIDE
#define FPI_MAX_TERMS_PER_SIDE 32
#endif

#ifndef FPI_MAX_COND_SYMBOLS
#define FPI_MAX_COND_SYMBOLS 512
#endif

#ifndef FPI_MAX_ACT_SYMBOLS
#define FPI_MAX_ACT_SYMBOLS 512
#endif

#ifndef FPI_REGISTRY_POOL_BYTES
#define FPI_REGISTRY_POOL_BYTES 32768UL
#endif

#ifndef FPI_VALUE_POOL_BYTES
#define FPI_VALUE_POOL_BYTES 131072UL
#endif

#ifndef FPI_MAX_POLYSYMS
#define FPI_MAX_POLYSYMS 256
#endif

#ifndef FPI_POLYSYM_POOL_BYTES
#define FPI_POLYSYM_POOL_BYTES 16384UL
#endif

#ifndef FPI_IDENT_MAX
#define FPI_IDENT_MAX 63
#endif

#ifndef FPI_ERROR_MESSAGE_MAX
#define FPI_ERROR_MESSAGE_MAX 159
#endif

#ifndef FPI_STATIC_CONTEXTS
#define FPI_STATIC_CONTEXTS 1
#endif

#ifndef FPI_STATIC_FILE_BYTES
#define FPI_STATIC_FILE_BYTES 262144UL
#endif

#ifndef FPI_PROGRAM_MAX_CODE
#define FPI_PROGRAM_MAX_CODE 65536
#endif

#ifndef FPI_PROGRAM_MAX_CONSTS
#define FPI_PROGRAM_MAX_CONSTS FPI_MAX_TERMS
#endif

#ifndef FPI_FIXED_FRAC_BITS
#define FPI_FIXED_FRAC_BITS 16
#endif

#define FPI_FIXED_ONE (1L << FPI_FIXED_FRAC_BITS)
#define FPI_FIXED_HALF (FPI_FIXED_ONE >> 1)

#if FPI_FIXED_FRAC_BITS != 16
#error FPIL_v18_requires_Q16_16
#endif

#endif /* FPI_CONFIG_H */
