#ifndef SPO_LAB_3_FILE_EXCHANGE_UTIL_H
#define SPO_LAB_3_FILE_EXCHANGE_UTIL_H

#include <stdint.h>

int parse(const char *cmd, char **args);
void get_hex_chars(uint8_t byte, char *hex);
#endif //SPO_LAB_3_FILE_EXCHANGE_UTIL_H
