
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define EXAMPLE_SERVER_URL "http://192.168.1.55:8080/firmware.bin"

#define BUFFSIZE 2048
#define HASH_LEN 32 /* SHA-256 digest length */

void init_partition();
void print_sha256 (const uint8_t *image_hash, const char *label);
void ota_example_task(void *pvParameter);