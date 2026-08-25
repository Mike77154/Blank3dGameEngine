#include "psd89/psd89_stdio.h"

static int psd89_stdio_read_fn(void *user, void *dst, psd89_u32 size)
{
    FILE *fp;
    fp = (FILE *)user;
    return fp != 0 && fread(dst, 1U, (size_t)size, fp) == (size_t)size;
}

static int psd89_stdio_write_fn(void *user, const void *src, psd89_u32 size)
{
    FILE *fp;
    fp = (FILE *)user;
    return fp != 0 && fwrite(src, 1U, (size_t)size, fp) == (size_t)size;
}

static int psd89_stdio_seek_fn(void *user, psd89_u32 offset)
{
    FILE *fp;
    fp = (FILE *)user;
    if (fp == 0) {
        return 0;
    }
    if (offset == 0xFFFFFFFFU) {
        return fseek(fp, 0, SEEK_END) == 0;
    }
    return fseek(fp, offset, SEEK_SET) == 0;
}

static psd89_u32 psd89_stdio_tell_fn(void *user)
{
    psd89_s32 pos;
    FILE *fp;
    fp = (FILE *)user;
    if (fp == 0) {
        return 0U;
    }
    pos = (psd89_s32)ftell(fp);
    return pos < 0 ? 0U : (psd89_u32)pos;
}

void psd89_stdio_make_io(FILE *fp, psd89_io *io)
{
    if (io == 0) {
        return;
    }
    io->read = psd89_stdio_read_fn;
    io->write = psd89_stdio_write_fn;
    io->seek = psd89_stdio_seek_fn;
    io->tell = psd89_stdio_tell_fn;
    io->user = fp;
}

int psd89_read_file(psd89_doc *doc, FILE *fp)
{
    psd89_io io;
    if (doc == 0 || fp == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    psd89_stdio_make_io(fp, &io);
    return psd89_read(doc, &io);
}

int psd89_write_file(FILE *fp, const psd89_doc *doc)
{
    psd89_io io;
    if (fp == 0 || doc == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    psd89_stdio_make_io(fp, &io);
    return psd89_write(&io, doc);
}

int psd89_read_path(psd89_doc *doc, const char *path)
{
    FILE *fp;
    int rc;
    if (doc == 0 || path == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    fp = fopen(path, "rb");
    if (fp == 0) {
        return PSD89_E_IO;
    }
    rc = psd89_read_file(doc, fp);
    fclose(fp);
    return rc;
}

int psd89_write_path(const char *path, const psd89_doc *doc)
{
    FILE *fp;
    int rc;
    if (path == 0 || doc == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    fp = fopen(path, "wb");
    if (fp == 0) {
        return PSD89_E_IO;
    }
    rc = psd89_write_file(fp, doc);
    fclose(fp);
    return rc;
}
