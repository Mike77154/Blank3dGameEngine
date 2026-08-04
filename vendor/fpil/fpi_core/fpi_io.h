#ifndef FPI_IO_H
#define FPI_IO_H

#include "fpi_error.h"

int fpi_io_read_file(const char* filename, char* buffer, FPI_U32 capacity, FPI_U32* out_length, FPI_Error* error);

#endif /* FPI_IO_H */
