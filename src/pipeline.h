#pragma once

#include <stdlib.h>
#include <stdbool.h>

extern VkPipeline pipeline;

unsigned char *load_file(const char *file_name, size_t *bytes_read );

bool create_pipeline();
void destroy_pipeline();

bool create_render_passes();
void destroy_render_passes();