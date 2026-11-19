/* code example showing how to (de)compress a 3D array in chunks */

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zfp.h"

/* open compressed stream for (de)compressing field at given rate */
static zfp_stream*
stream(const zfp_field* field, double rate)
{
  const size_t bx = (field->nx + 3) / 4; /* # blocks along x */
  const size_t by = (field->ny + 3) / 4; /* # blocks along y */
  const size_t bz = (field->nz + 3) / 4; /* # blocks along z */

  zfp_stream* zfp;   /* compressed stream */
  size_t words;      /* word size of compressed buffer */
  size_t bytes;      /* byte size of compressed buffer */
  void* buffer;      /* storage for compressed stream */
  bitstream* stream; /* bit stream to write to or read from */

  /* allocate meta data for a compressed stream */
  zfp = zfp_stream_open(NULL);

  /* set fixed-rate mode with no alignment */
  zfp_stream_set_rate(zfp, rate, zfp_type_double, zfp_field_dimensionality(field), zfp_false);

  /* determine exact compressed size in words */
  words = (bx * by * bz * zfp->maxbits + stream_word_bits - 1) / stream_word_bits;

  /* allocate buffer for single chunk of compressed data */
  bytes = words * stream_word_bits / CHAR_BIT;
  buffer = malloc(bytes);

  /* associate bit stream with allocated buffer */
  stream = stream_open(buffer, bytes);
  zfp_stream_set_bit_stream(zfp, stream);

  return zfp;
}

/* compress chunk */
static zfp_bool
compress(zfp_stream* zfp, const zfp_field* field)
{
  void* buffer = stream_data(zfp_stream_bit_stream(zfp));

  /* compress chunk and output compressed data */
  size_t size = zfp_compress(zfp, field);
  if (!size)
    return zfp_false;
  fwrite(buffer, 1, size, stdout);

  return zfp_true;
}

/* decompress chunk */
static zfp_bool
decompress(zfp_stream* zfp, zfp_field* field)
{
  void* buffer = stream_data(zfp_stream_bit_stream(zfp));

  /* decompress chunk and output uncompressed data */
  size_t size = fread(buffer, 1, stream_capacity(zfp_stream_bit_stream(zfp)), stdin);
  if (zfp_decompress(zfp, field) != size)
    return zfp_false;
  fwrite(zfp_field_pointer(field), sizeof(double), zfp_field_size(field, NULL), stdout);

  return zfp_true;
}

/* print command usage */
static int
usage(void)
{
  fprintf(stderr, "chunk [options] <input >output\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "-3 <nx> <ny> <nz> : array dimensions\n");
  fprintf(stderr, "-d : decompress (from stdin to stdout); else compress\n");
  fprintf(stderr, "-n <count> : number of chunks along z dimension\n");
  fprintf(stderr, "-r <rate> : rate in bits/value\n");

  return EXIT_FAILURE;
}

int main(int argc, char* argv[])
{
  /* command-line arguments */
  zfp_bool decode = zfp_false;
  double rate = 16;
  int nx = 125;
  int ny = 100;
  int nz = 240;
  int chunks = 1;

  /* local variables */
  double* array;
  double* ptr;
  zfp_field* field;
  zfp_stream* zfp;
  int i, x, y, z, mz;

  /* process command line */
  for (i = 1; i < argc; i++)
    if (!strcmp(argv[i], "-3")) {
      if (++i == argc || sscanf(argv[i], "%d", &nx) != 1 ||
          ++i == argc || sscanf(argv[i], "%d", &ny) != 1 ||
          ++i == argc || sscanf(argv[i], "%d", &nz) != 1)
        return usage();
    }
    else if (!strcmp(argv[i], "-d"))
      decode = zfp_true;
    else if (!strcmp(argv[i], "-r")) {
      if (++i == argc || sscanf(argv[i], "%lf", &rate) != 1)
        return usage();
    }
    else if (!strcmp(argv[i], "-n")) {
      if (++i == argc || sscanf(argv[i], "%d", &chunks) != 1)
        usage();
    }
    else
      return usage();

  /* compute chunk size (must be a multiple of four) */
  mz = 4 * ((nz + 4 * chunks - 1) / (4 * chunks));
  if ((chunks - 1) * mz >= nz) {
    fprintf(stderr, "cannot partition nz=%d into %d chunks\n", nz, chunks);
    return EXIT_FAILURE;
  }

  /* allocate whole nx * ny * nz array of doubles */
  array = malloc(nx * ny * nz * sizeof(double));

  if (!decode) {
    /* initialize array to be compressed */
    for (z = 0; z < nz; z++)
      for (y = 0; y < ny; y++)
        for (x = 0; x < nx; x++)
          array[x + nx * (y + ny * z)] = 1. / (1 + x + nx * (y + ny * z));
  }

  /* initialize field, stream, and compressed buffer */
  field = zfp_field_3d(array, zfp_type_double, nx, ny, mz);
  zfp = stream(field, rate);

  /* warn if compressed size is not a multiple of word size */
  if (chunks > 1 && (zfp_field_blocks(field) * zfp->maxbits) % stream_word_bits)
    fprintf(stderr, "warning: compressed size (%ld) is not a multiple of word size (%ld)\n", (long)(zfp_field_blocks(field) * zfp->maxbits), (long)stream_word_bits);

  /* (de)compress array in chunks */
  ptr = array;
  for (z = 0; z < nz; z += mz) {
    /* compute current chunk size as min(mz, nz - z) */
    int cz = mz < nz - z ? mz : nz - z;

    /* set chunk size and pointer into uncompressed array */
    zfp_field_set_pointer(field, ptr);
    zfp_field_set_size_3d(field, nx, ny, cz);

    /* reuse compressed buffer by rewinding compressed stream */
    zfp_stream_rewind(zfp);

    if (decode) {
      /* decompress current chunk from stdin to stdout */
      if (!decompress(zfp, field)) {
        fprintf(stderr, "decompression failed\n");
        return EXIT_FAILURE;
      }
    }
    else {
      /* compress current chunk to stdout */
      if (!compress(zfp, field)) {
        fprintf(stderr, "compression failed\n");
        return EXIT_FAILURE;
      }
    }

    /* advance pointer to next chunk of uncompressed data */
    ptr += nx * ny * cz;
  }

  /* clean up */
  free(stream_data(zfp_stream_bit_stream(zfp)));
  stream_close(zfp_stream_bit_stream(zfp));
  zfp_stream_close(zfp);
  zfp_field_free(field);
  free(array);

  return EXIT_SUCCESS;
}
