// Copyright 2026, Nick Kennedy
// SPDX-License-Identifier: MIT
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <assert.h>
#include <string.h>
static const void *expected;
static unsigned calls;
void resolve_expect(const void *p) { expected=p; }
unsigned resolve_calls(void) { return calls; }
#define CREATE(n,t,o) static VkResult n(VkDevice d,const t *ci,const VkAllocationCallbacks *a,o *out) { \
 assert(d==(VkDevice)1 && ci==expected && !a);calls++;*out=(o)0xabc;return VK_ERROR_OUT_OF_DEVICE_MEMORY; }
CREATE(image,VkImageCreateInfo,VkImage)
CREATE(view,VkImageViewCreateInfo,VkImageView)
CREATE(shader,VkShaderModuleCreateInfo,VkShaderModule)
CREATE(pass,VkRenderPassCreateInfo,VkRenderPass)
CREATE(pass2,VkRenderPassCreateInfo2,VkRenderPass)
static VkResult pipelines(VkDevice d,VkPipelineCache cache,uint32_t n,const VkGraphicsPipelineCreateInfo *ci,const VkAllocationCallbacks *a,VkPipeline *out) {
 assert(d==(VkDevice)1 && cache==(VkPipelineCache)0x10 && n==2 && ci==expected && !a);calls++;
 out[0]=(VkPipeline)0xabc;out[1]=VK_NULL_HANDLE;return VK_PIPELINE_COMPILE_REQUIRED;
}
static void rendering(VkCommandBuffer cb,const VkRenderingInfo *info) { assert(cb==(VkCommandBuffer)0x789 && info==expected);calls++; }
static void custom(VkCommandBuffer cb,const VkBeginCustomResolveInfoEXT *info) { assert(cb==(VkCommandBuffer)0x789 && info==expected);calls++; }
static void other_custom(VkCommandBuffer cb,const VkBeginCustomResolveInfoEXT *info) { assert(cb==(VkCommandBuffer)0x789 && info==expected);calls++; }
static void bind(VkCommandBuffer cb,VkPipelineBindPoint point,VkPipeline p) { assert(cb==(VkCommandBuffer)0x789 && point==VK_PIPELINE_BIND_POINT_GRAPHICS && p==(VkPipeline)0xabc);calls++; }
static void draw(VkCommandBuffer cb,uint32_t a,uint32_t b,uint32_t c,uint32_t d) { assert(cb==(VkCommandBuffer)0x789 && a==3 && b==2 && c==7 && d==9);calls++; }
static void end(VkCommandBuffer cb) { assert(cb==(VkCommandBuffer)0x789);calls++; }
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance i,const char *name) {
#define F(n,f) if(!strcmp(name,n)) return (PFN_vkVoidFunction)f
 F("vkCreateImage",image);F("vkCreateImageView",view);F("vkCreateShaderModule",shader);
 F("vkCreateGraphicsPipelines",pipelines);F("vkCreateRenderPass",pass);F("vkCreateRenderPass2",pass2);F("vkCreateRenderPass2KHR",pass2);
 F("vkCmdBeginRendering",rendering);F("vkCmdBeginRenderingKHR",rendering);F("vkCmdBeginCustomResolveEXT",custom);
 F("vkCmdBindPipeline",bind);F("vkCmdDraw",draw);F("vkCmdEndRendering",end);F("vkCmdEndRenderingKHR",end);
 return NULL;
}
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice d,const char *name) {
 if(d==(VkDevice)2 && !strcmp(name,"vkCmdBeginCustomResolveEXT")) return (PFN_vkVoidFunction)other_custom;
 return vkGetInstanceProcAddr(NULL,name);
}
