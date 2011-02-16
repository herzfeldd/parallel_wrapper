#ifndef CHIRP_UTIL_H
#define CHIRP_UTIL_H

#include <string.h>
#include <errno.h>
#include "chirp_client.h"
#include "string_util.h"
#include "log.h"

extern int get_chirp_integer(struct chirp_client *chirp, const char *key, int *value);
extern int get_chirp_string(struct chirp_client *chirp, const char *key, char **value);


#endif /* CHIRP_UTIL_H */
