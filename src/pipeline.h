#pragma once
#include <stdlib.h>
#include <stdbool.h>


unsigned char *load_file(const char *file_name, size_t *bytes_read );

bool create_pipeline();