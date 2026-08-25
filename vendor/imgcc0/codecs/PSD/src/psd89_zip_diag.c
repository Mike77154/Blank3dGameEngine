#include "psd89/psd89.h"

#include <string.h>

void psd89_zip_diag_init(psd89_zip_diag *diag)
{
    if (diag == 0) {
        return;
    }
    memset(diag, 0, sizeof(*diag));
    diag->code = PSD89_ZIP_DIAG_OK;
}

psd89_u32 psd89_zip_expected_planar_bytes(psd89_u32 rows,
                                              psd89_u32 cols,
                                              psd89_u16 channels)
{
    return (psd89_u32)rows * (psd89_u32)cols * (psd89_u32)channels;
}

int psd89_zip_diag_check_planar(psd89_u16 compression,
                                psd89_u32 rows,
                                psd89_u32 cols,
                                psd89_u16 channels,
                                psd89_u32 compressed_bytes,
                                psd89_u32 decoded_bytes,
                                int zlib_status,
                                psd89_zip_diag *diag)
{
    psd89_u32 expected;

    expected = psd89_zip_expected_planar_bytes(rows, cols, channels);

    if (diag != 0) {
        psd89_zip_diag_init(diag);
        diag->compression = compression;
        diag->rows = rows;
        diag->cols = cols;
        diag->channels = channels;
        diag->compressed_bytes = compressed_bytes;
        diag->decoded_bytes = decoded_bytes;
        diag->expected_bytes = expected;
        diag->zlib_status = zlib_status;
    }

    if (zlib_status != 0) {
        if (diag != 0) {
            diag->code = PSD89_ZIP_DIAG_ZLIB_STATUS;
        }
        return PSD89_ZIP_DIAG_ZLIB_STATUS;
    }

    if (decoded_bytes != expected) {
        if (diag != 0) {
            if (compression == PSD89_COMP_ZIP_PRED) {
                diag->code = PSD89_ZIP_DIAG_BAD_PREDICTED_SIZE;
            } else if (compression == PSD89_COMP_ZIP) {
                diag->code = PSD89_ZIP_DIAG_BAD_PLANAR_SIZE;
            } else {
                diag->code = PSD89_ZIP_DIAG_STREAM_MISMATCH;
            }
        }
        return diag != 0 ? diag->code : PSD89_ZIP_DIAG_STREAM_MISMATCH;
    }

    return PSD89_ZIP_DIAG_OK;
}

const char *psd89_zip_diag_string(int code)
{
    switch (code) {
        case PSD89_ZIP_DIAG_OK:
            return "ok";
        case PSD89_ZIP_DIAG_ZLIB_STATUS:
            return "zlib status error";
        case PSD89_ZIP_DIAG_BAD_PLANAR_SIZE:
            return "decoded planar byte count mismatch";
        case PSD89_ZIP_DIAG_BAD_PREDICTED_SIZE:
            return "decoded ZIP+prediction byte count mismatch";
        case PSD89_ZIP_DIAG_STREAM_MISMATCH:
            return "generic ZIP stream mismatch";
        default:
            return "unknown ZIP diagnostic";
    }
}
