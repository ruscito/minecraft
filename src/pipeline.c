//#include "pipeline.h"

#include "log.h"
#include "vulkan_if.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static VkPipelineLayout pipeline_layout;

VkRenderPass render_pass;
VkPipeline pipeline;

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

static VkShaderModule create_shader_module(const unsigned char *code, size_t size) {
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = size;
    create_info.pCode =(const uint32_t *) code;

    VkShaderModule shader_module;
    if (vkCreateShaderModule(logical_device, &create_info, NULL, &shader_module)) {
        ERROR("Shader module creation error");
        return NULL;
    }
    return shader_module;
}

bool create_pipeline() {
    size_t vert_shader_file_size;
    size_t frag_shader_file_size;
    unsigned char *vert_shader_file = load_file(SHADERS_PATH"triangle.vert.spv", &vert_shader_file_size);
    unsigned char *frag_shader_file = load_file(SHADERS_PATH"triangle.frag.spv", &frag_shader_file_size);

    INFO("FILE I/O vert. size:%d frag. size:%d", vert_shader_file_size, frag_shader_file_size);

    // create a wrap around the shader files
    VkShaderModule vert_shader_module = NULL;
    VkShaderModule frag_shader_module = NULL; 
    if ((vert_shader_module = create_shader_module(vert_shader_file, vert_shader_file_size)) == NULL) {
        return false;
    }

    if ((frag_shader_module = create_shader_module(frag_shader_file, frag_shader_file_size)) == NULL) {
        false;
    }
    
    // To actually use the shaders we'll need to assign them to a specific pipeline stage through 
    // VkPipelineShaderStageCreateInfo structures as part of the actual pipeline creation process.
    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    // Fixed function setup
    VkPipelineVertexInputStateCreateInfo  vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // Bindings: spacing between data and whether the data is per-vertex or per-instance
    vertex_input_info.vertexBindingDescriptionCount = 0; 
    vertex_input_info.pVertexBindingDescriptions = NULL; // Optional
    // type of the attributes passed to the vertex shader, which binding to load them from and at which offset
    vertex_input_info.vertexAttributeDescriptionCount = 0; 
    vertex_input_info.pVertexAttributeDescriptions = NULL; // Optional 

    // input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // viewports and sissors
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swap_chain.extent.width;
    viewport.height = (float) swap_chain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // sissors
    VkOffset2D offset = {0, 0};
    VkRect2D scissor = {};
    scissor.offset = offset;
    scissor.extent = swap_chain.extent;

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports  = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors  = &scissor;

    // rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //The polygonMode determines how fragments are generated for geometry
    rasterizer.lineWidth = 1.0f;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    // multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = NULL; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    // color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // O
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f; // Optional
    color_blending.blendConstants[1] = 0.0f; // Optional
    color_blending.blendConstants[2] = 0.0f; // Optional
    color_blending.blendConstants[3] = 0.0f; // Optional

    // pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0; // Optional
    pipeline_layout_info.pSetLayouts = NULL; // Optional
    pipeline_layout_info.pushConstantRangeCount = 0; // Optional
    pipeline_layout_info.pPushConstantRanges = NULL; // Optional

    if (vkCreatePipelineLayout(logical_device, &pipeline_layout_info, NULL, &pipeline_layout) != VK_SUCCESS) {
        FATAL("Failed to create pipeline layout");
        return false;
    }

    // finally pipline creation
    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;

    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = NULL; // Optional
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipeline_info.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline) != VK_SUCCESS) {
        FATAL("Failed to create graphics pipeline!");
        return false;
    }


    vkDestroyShaderModule(logical_device, vert_shader_module, NULL);
    vkDestroyShaderModule(logical_device, frag_shader_module, NULL);
    free(vert_shader_file);
    free(frag_shader_file);
    return true;
}

void destroy_pipeline() {
    vkDestroyPipeline(logical_device, pipeline, NULL);
    vkDestroyPipelineLayout(logical_device, pipeline_layout, NULL);
}


bool create_render_passes() {
    // Before we can finish creating the pipeline, we need to tell Vulkan about the framebuffer attachments 
    // that will be used while rendering. We need to specify how many color and depth buffers there will be, 
    // how many samples to use for each of them and how their contents should be handled throughout the rendering operations. 
    // All of this information is wrapped in a render pass object, for which we'll create a new createRenderPass function. 
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swap_chain.image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Subpasses and attachment references
    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    if (vkCreateRenderPass(logical_device, &render_pass_info, NULL, &render_pass) != VK_SUCCESS) {
        FATAL("failed to create render pass!");
        return false;
    }
    return true;
}

void destroy_render_passes() {
     vkDestroyRenderPass(logical_device, render_pass, NULL);
}