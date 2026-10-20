// Copyright 2026, Nick Kennedy
// SPDX-License-Identifier: MIT
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <assert.h>
#include <string.h>
static const void *expected;
static unsigned calls;
void observe_expect(const void *pointer) { expected=pointer; }
unsigned observe_calls(void) { return calls; }
static void features(VkPhysicalDevice pd,VkPhysicalDeviceFeatures2 *f) {
 assert(pd==(VkPhysicalDevice)0x123 && f==expected);calls++;
 VkPhysicalDeviceFragmentDensityMapFeaturesEXT *d=f->pNext;
 assert(d->pNext);d->fragmentDensityMap=VK_FALSE;d->fragmentDensityMapDynamic=VK_FALSE;
 d->fragmentDensityMapNonSubsampledImages=VK_FALSE;
 ((VkPhysicalDeviceFragmentDensityMapOffsetFeaturesEXT *)d->pNext)->fragmentDensityMapOffset=VK_FALSE;
}
static void properties(VkPhysicalDevice pd,VkPhysicalDeviceProperties2 *p) {
 assert(pd==(VkPhysicalDevice)0x123 && p==expected);calls++;
 VkPhysicalDeviceFragmentDensityMapOffsetPropertiesEXT *offset=p->pNext;
 offset->fragmentDensityOffsetGranularity=(VkExtent2D){16,8};
}
static VkResult image(VkDevice dev,const VkImageCreateInfo *ci,const VkAllocationCallbacks *a,VkImage *out) {
 assert(dev==(VkDevice)1 && ci==expected && !a);calls++;*out=(VkImage)0x456;
 return VK_ERROR_FORMAT_NOT_SUPPORTED;
}
static VkResult other_image(VkDevice dev,const VkImageCreateInfo *ci,const VkAllocationCallbacks *a,VkImage *out) {
 assert(dev==(VkDevice)2 && ci==expected && !a);calls++;return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}
static void pass_end(VkCommandBuffer cb,const VkSubpassEndInfo *end) {
 assert(cb==(VkCommandBuffer)0x789 && end==expected);calls++;
 const VkRenderPassFragmentDensityMapOffsetEndInfoEXT *f=end->pNext;
 assert(f->fragmentDensityOffsetCount==2 && f->pFragmentDensityOffsets[1].x==-32);
}
static VkResult shader(VkDevice dev,const VkShaderModuleCreateInfo *ci,const VkAllocationCallbacks *a,VkShaderModule *out) {
 assert(dev==(VkDevice)1 && ci==expected && ci->pCode[6]==5291 && !a);calls++;
 return VK_ERROR_INVALID_SHADER_NV;
}
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance inst,const char *name) {
 if(!strcmp(name,"vkGetPhysicalDeviceFeatures2") || !strcmp(name,"vkGetPhysicalDeviceFeatures2KHR"))return (void*)features;
 if(!strcmp(name,"vkGetPhysicalDeviceProperties2") || !strcmp(name,"vkGetPhysicalDeviceProperties2KHR"))return (void*)properties;
 if(!strcmp(name,"vkCmdEndRenderPass2") || !strcmp(name,"vkCmdEndRenderPass2KHR"))return (void*)pass_end;
 if(!strcmp(name,"vkCreateImage"))return (void*)image;
 if(!strcmp(name,"vkCreateShaderModule"))return (void*)shader;
 return 0;
}
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev,const char *name) {
 if(dev==(VkDevice)2 && !strcmp(name,"vkCreateImage"))return (void*)other_image;
 return vkGetInstanceProcAddr(0,name);
}
