#include <stdbool.h>
#include "curl_xml.h"
#include "hash.h"
#include <pthread.h>

#define URL_SIZE 512
#define FILE_PATH_SIZE 512
#define STACK_SIZE 1024
#define HMAP_SIZE 1024

void *runner(void *args);