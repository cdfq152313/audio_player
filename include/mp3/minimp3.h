#ifndef __MINIMP3
#define __MINIMP3

#include "decoder.h"

struct buffer {
  unsigned char const *start;
  unsigned long length;
};

int mp3_init(unsigned char const *, unsigned long);

#endif
