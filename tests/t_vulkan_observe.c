// Copyright 2026, Nick Kennedy
// SPDX-License-Identifier: MIT
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *kl_driver_target_name(void) { return "strict-observe-fixture"; }
void kl_glfb_note_eye_mtl_texture(int a,int b,void*c,int d,int e,int f) { abort(); }
int main(int argc,char **argv) {
 assert(argc==4);int enabled=atoi(argv[3]);
 void *driver=dlopen(argv[2],RTLD_NOW|RTLD_LOCAL);assert(driver);
 void *library=dlopen(argv[1],RTLD_LAZY|RTLD_LOCAL);assert(library);
 void (*expect)(const void*)=dlsym(driver,"observe_expect");unsigned(*calls)(void)=dlsym(driver,"observe_calls");
 void *(*symbol)(const char*)=dlsym(library,"kl_vulkan_sym");
 PFN_vkGetInstanceProcAddr gipa=symbol("vkGetInstanceProcAddr"),native=dlsym(driver,"vkGetInstanceProcAddr");
 PFN_vkGetDeviceProcAddr gdpa=symbol("vkGetDeviceProcAddr"),ngdpa=dlsym(driver,"vkGetDeviceProcAddr");
 assert(!gdpa((VkDevice)1,"vkCmdUnsupportedFdmTEST"));
 VkPhysicalDeviceFragmentDensityMapOffsetFeaturesEXT offset={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_FEATURES_EXT};
 VkPhysicalDeviceFragmentDensityMapFeaturesEXT density={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT,.pNext=&offset};
 VkPhysicalDeviceFeatures2 features={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,.pNext=&density};
 for(unsigned i=0;i<2;i++) {
  const char *name=i?"vkGetPhysicalDeviceFeatures2KHR":"vkGetPhysicalDeviceFeatures2";
  PFN_vkVoidFunction fn=gipa((VkInstance)1,name);assert((fn!=native((VkInstance)1,name))==enabled);
  density.fragmentDensityMap=VK_TRUE;offset.fragmentDensityMapOffset=VK_TRUE;expect(&features);
  ((PFN_vkGetPhysicalDeviceFeatures2)fn)((VkPhysicalDevice)0x123,&features);
  assert(!density.fragmentDensityMap && !offset.fragmentDensityMapOffset && density.pNext==&offset && features.pNext==&density);
 }
 VkPhysicalDeviceFragmentDensityMapOffsetPropertiesEXT op={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_PROPERTIES_EXT};
 VkPhysicalDeviceProperties2 props={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,.pNext=&op};expect(&props);
 ((PFN_vkGetPhysicalDeviceProperties2)gipa((VkInstance)1,"vkGetPhysicalDeviceProperties2"))((VkPhysicalDevice)0x123,&props);
 assert(op.fragmentDensityOffsetGranularity.width==16 && op.fragmentDensityOffsetGranularity.height==8);
 VkImageCreateInfo ci={.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,.format=VK_FORMAT_R8G8_UNORM,
  .extent={16,16,1},.arrayLayers=2,.samples=VK_SAMPLE_COUNT_1_BIT,.usage=VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT};
 VkImageCreateInfo before=ci;VkImage out;expect(&ci);
 PFN_vkVoidFunction primary=gdpa((VkDevice)1,"vkCreateImage");
 assert(((PFN_vkCreateImage)primary)((VkDevice)1,&ci,0,&out)==VK_ERROR_FORMAT_NOT_SUPPORTED && out==(VkImage)0x456);
 assert(!memcmp(&ci,&before,sizeof ci));
 PFN_vkVoidFunction other=gdpa((VkDevice)2,"vkCreateImage");assert(other==ngdpa((VkDevice)2,"vkCreateImage"));
 assert(((PFN_vkCreateImage)other)((VkDevice)2,&ci,0,&out)==VK_ERROR_OUT_OF_DEVICE_MEMORY);
 VkOffset2D offsets[2]={{16,8},{-32,-8}};
 VkRenderPassFragmentDensityMapOffsetEndInfoEXT endmap={.sType=VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_OFFSET_END_INFO_EXT,
  .fragmentDensityOffsetCount=2,.pFragmentDensityOffsets=offsets};
 VkSubpassEndInfo end={.sType=VK_STRUCTURE_TYPE_SUBPASS_END_INFO,.pNext=&endmap};expect(&end);
 ((PFN_vkCmdEndRenderPass2)gdpa((VkDevice)1,"vkCmdEndRenderPass2"))((VkCommandBuffer)0x789,&end);
 ((PFN_vkCmdEndRenderPass2)gdpa((VkDevice)1,"vkCmdEndRenderPass2KHR"))((VkCommandBuffer)0x789,&end);
 assert(end.pNext==&endmap && endmap.pFragmentDensityOffsets==offsets && offsets[1].x==-32);
 // Host-generated SPIR-V metadata fixture, never game shader bytes.
 const uint32_t words[]={0x07230203,0x10000,0,4,0,(2u<<16)|17,5291,(4u<<16)|71,1,11,5292};
 VkShaderModuleCreateInfo shader={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=sizeof words,.pCode=words};expect(&shader);
 VkShaderModule sm;assert(((PFN_vkCreateShaderModule)gdpa((VkDevice)1,"vkCreateShaderModule"))((VkDevice)1,&shader,0,&sm)==VK_ERROR_INVALID_SHADER_NV);
 assert(shader.pCode==words && words[6]==5291 && calls()==8);
 printf("PASS strict observation=%d: original arguments/results, alias calls, native dispatch differences, absent commands preserved\n",enabled);
}
