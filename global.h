#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "settings.h"

typedef struct {
  char buf[BUF_SIZE];
  int n;
  char *fn;
  gboolean open_md5;
  gboolean close;
  gboolean write_hash;
  gboolean quit;
} workunit;

#define RING_BUFFER_TYPE workunit

#endif /*  __GLOBAL_H__ */
