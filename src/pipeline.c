#include "pipeline.h"
#include "log.h"


#include <stdio.h>



unsigned char *load_file(const char *file_name, size_t *bytes_read ){
    unsigned char *data = NULL;
    
    FILE  *file = fopen(file_name, "rb");
    if (file != NULL)  {
        fseek(file, 0, SEEK_END);   //go to the end
        long size = ftell(file); //get current position
        fseek(file, 0, SEEK_SET);   //go back to the begining

        if (size> 0) {
            data = malloc (size * sizeof(unsigned char));

            size_t count = fread(data, sizeof(unsigned char), size, file);
            *bytes_read =  count;

            if (count != size) {
                WARNING("FILE I/O [%s] file partially loaded", file_name);
            }  
            else {
                INFO("FILE I/O [%s] file loaded successfully", file_name);
            }
        } 
        else {
            ERROR("FILE I/O [%s] failed to read the file", file_name);
        }
        fclose(file);
    } 
    else {
        ERROR("FILE I/O [%s] failed to open the file", file_name);
    }

    return data;
}


bool create_pipeline() {
    size_t vert_shader_file_size;
    size_t frag_shader_file_size;
    unsigned char *vert_shader_file = load_file(SHADERS_PATH"triangle.vert.spv", &vert_shader_file_size);
    unsigned char *frag_shader_file = load_file(SHADERS_PATH"triangle.frag.spv", &frag_shader_file_size);

    INFO("FILE I/O vert. size:%d frag. size:%d", vert_shader_file_size, frag_shader_file_size);



    return true;
}