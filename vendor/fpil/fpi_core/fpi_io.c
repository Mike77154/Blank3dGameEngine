#include "fpi_io.h"
#include <stdio.h>

int fpi_io_read_file(const char* filename, char* buffer, FPI_U32 capacity, FPI_U32* out_length, FPI_Error* error) {
    FILE* file;
    size_t read_count;
    int extra;
    FPI_Span span;
    fpi_span_clear(&span);
    if (out_length) *out_length = 0UL;
    if (!filename || !buffer || capacity < 2UL) return FPI_ERR_ARGUMENT;
    file = fopen(filename, "rb");
    if (!file) {
        fpi_error_set(error, FPI_ERR_IO, span, "unable to open script file");
        return FPI_ERR_IO;
    }
    read_count = fread(buffer, 1, (size_t)(capacity - 1UL), file);
    if (ferror(file)) {
        fclose(file);
        fpi_error_set(error, FPI_ERR_IO, span, "unable to read script file");
        return FPI_ERR_IO;
    }
    extra = fgetc(file);
    fclose(file);
    if (extra != EOF) {
        buffer[0] = '\0';
        fpi_error_set(error, FPI_ERR_FILE_TOO_LARGE, span, "script exceeds caller-provided file buffer");
        return FPI_ERR_FILE_TOO_LARGE;
    }
    buffer[read_count] = '\0';
    if (out_length) *out_length = (FPI_U32)read_count;
    return FPI_OK;
}
