// Copyright 2026, Nick Kennedy
// SPDX-License-Identifier: MIT
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *kl_driver_target_name(void) {return "resolve-fixture";}
void kl_glfb_note_eye_mtl_texture(int a,int b,void*c,int d,int e,int f) {abort();}
int main(int argc,char **argv) {
 assert(argc==4); int enabled=atoi(argv[3]);
 void *driver=dlopen(argv[2],RTLD_NOW|RTLD_LOCAL);assert(driver);
 void *library=dlopen(argv[1],RTLD_LAZY|RTLD_LOCAL);if(!library) fprintf(stderr,"%s\n",dlerror());assert(library);
 void (*expect)(const void*)=dlsym(driver,"resolve_expect");unsigned(*calls)(void)=dlsym(driver,"resolve_calls");
 void *(*symbol)(const char*)=dlsym(library,"kl_vulkan_sym");
 PFN_vkGetDeviceProcAddr gdpa=symbol("vkGetDeviceProcAddr"),native=dlsym(driver,"vkGetDeviceProcAddr");
 const char *names[]={"vkCreateImage","vkCreateImageView","vkCreateShaderModule","vkCreateGraphicsPipelines",
  "vkCreateRenderPass","vkCreateRenderPass2","vkCreateRenderPass2KHR","vkCmdBeginRendering","vkCmdBeginRenderingKHR",
  "vkCmdBeginCustomResolveEXT","vkCmdBindPipeline","vkCmdDraw","vkCmdEndRendering","vkCmdEndRenderingKHR"};
 for(unsigned i=0;i<sizeof names/sizeof names[0];i++) assert((gdpa((VkDevice)1,names[i])!=native((VkDevice)1,names[i]))==enabled);
 assert(!gdpa((VkDevice)1,"vkDoesNotExistTEST"));
#define CALL(n) ((PFN_vk##n)gdpa((VkDevice)1,"vk" #n))
 VkImageCreateInfo image={.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,.extent={16,16,1},.arrayLayers=2,.samples=VK_SAMPLE_COUNT_4_BIT};
 VkImageCreateInfo image_before=image;VkImage im;expect(&image);
 assert(CALL(CreateImage)((VkDevice)1,&image,NULL,&im)==VK_ERROR_OUT_OF_DEVICE_MEMORY && im==(VkImage)0xabc);
 assert(!memcmp(&image,&image_before,sizeof image));
 VkImageViewCreateInfo view={.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,.image=(VkImage)0x111};VkImageView vi;expect(&view);
 assert(CALL(CreateImageView)((VkDevice)1,&view,NULL,&vi)==VK_ERROR_OUT_OF_DEVICE_MEMORY && vi==(VkImageView)0xabc);
 uint32_t index=0;VkFormat format=VK_FORMAT_R8G8B8A8_UNORM;
 VkRenderingInputAttachmentIndexInfo map={.sType=VK_STRUCTURE_TYPE_RENDERING_INPUT_ATTACHMENT_INDEX_INFO,.colorAttachmentCount=1,.pColorAttachmentInputIndices=&index};
 VkCustomResolveCreateInfoEXT custom={.sType=VK_STRUCTURE_TYPE_CUSTOM_RESOLVE_CREATE_INFO_EXT,.pNext=&map,.customResolve=1,.colorAttachmentCount=1,.pColorAttachmentFormats=&format};
 VkGraphicsPipelineCreateInfo ci[2]={{.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,.pNext=&custom},{.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO}};
 VkGraphicsPipelineCreateInfo before[2]; memcpy(before,ci,sizeof ci); VkPipeline out[2];expect(ci);
 assert(CALL(CreateGraphicsPipelines)((VkDevice)1,(VkPipelineCache)0x10,2,ci,NULL,out)==VK_PIPELINE_COMPILE_REQUIRED);
 assert(out[0]==(VkPipeline)0xabc && !out[1] && !memcmp(ci,before,sizeof ci));
 assert(custom.pNext==&map && custom.pColorAttachmentFormats==&format && map.pColorAttachmentInputIndices==&index);
 VkRenderPass pass;
 VkRenderPassCreateInfo rp={.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};expect(&rp);
 assert(CALL(CreateRenderPass)((VkDevice)1,&rp,NULL,&pass)==VK_ERROR_OUT_OF_DEVICE_MEMORY && pass==(VkRenderPass)0xabc);
 VkRenderPassCreateInfo2 rp2={.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2};expect(&rp2);
 assert(CALL(CreateRenderPass2)((VkDevice)1,&rp2,NULL,&pass)==VK_ERROR_OUT_OF_DEVICE_MEMORY);
 assert(CALL(CreateRenderPass2KHR)((VkDevice)1,&rp2,NULL,&pass)==VK_ERROR_OUT_OF_DEVICE_MEMORY);
 VkRenderingAttachmentInfo color={.sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,.imageView=(VkImageView)0x111,.resolveMode=VK_RESOLVE_MODE_CUSTOM_BIT_EXT,.resolveImageView=(VkImageView)0x222};
 VkRenderingInfo ri={.sType=VK_STRUCTURE_TYPE_RENDERING_INFO,.pNext=&map,.flags=VK_RENDERING_CUSTOM_RESOLVE_BIT_EXT,.colorAttachmentCount=1,.pColorAttachments=&color};
 VkRenderingInfo ri_before=ri;expect(&ri);
 CALL(CmdBeginRendering)((VkCommandBuffer)0x789,&ri);CALL(CmdBeginRenderingKHR)((VkCommandBuffer)0x789,&ri);
 assert(!memcmp(&ri,&ri_before,sizeof ri) && color.imageView==(VkImageView)0x111);
 VkBeginCustomResolveInfoEXT begin={.sType=VK_STRUCTURE_TYPE_BEGIN_CUSTOM_RESOLVE_INFO_EXT,.pNext=&custom};expect(&begin);
 CALL(CmdBeginCustomResolveEXT)((VkCommandBuffer)0x789,&begin);
 CALL(CmdBindPipeline)((VkCommandBuffer)0x789,VK_PIPELINE_BIND_POINT_GRAPHICS,(VkPipeline)0xabc);
 CALL(CmdDraw)((VkCommandBuffer)0x789,3,2,7,9);
 CALL(CmdEndRendering)((VkCommandBuffer)0x789);CALL(CmdEndRenderingKHR)((VkCommandBuffer)0x789);
 assert(begin.pNext==&custom);
 expect(NULL);CALL(CmdBeginCustomResolveEXT)((VkCommandBuffer)0x789,NULL);
 PFN_vkVoidFunction other=gdpa((VkDevice)2,"vkCmdBeginCustomResolveEXT");assert(other==native((VkDevice)2,"vkCmdBeginCustomResolveEXT"));
 ((PFN_vkCmdBeginCustomResolveEXT)other)((VkCommandBuffer)0x789,NULL);
 const uint32_t words[]={0x07230203,0x10000,0,8,0,(9u<<16)|25,1,2,6,0,0,1,2,0,(4u<<16)|71,3,43,0,(1u<<16)|98};
 VkShaderModuleCreateInfo sc={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=sizeof words,.pCode=words};expect(&sc);VkShaderModule sm;
 assert(CALL(CreateShaderModule)((VkDevice)1,&sc,NULL,&sm)==VK_ERROR_OUT_OF_DEVICE_MEMORY && sm==(VkShaderModule)0xabc);
 assert(sc.pCode==words && words[6]==1 && calls()==16);
 for(unsigned i=0;i<140;i++) CALL(CmdDraw)((VkCommandBuffer)0x789,3,2,7,9);
 CALL(CmdEndRendering)((VkCommandBuffer)0x789); assert(calls()==157);
 printf("PASS resolve observation=%d: exact arguments, pNext, failures/partial outputs, aliases, NULL info, missing commands and changed native target\n",enabled);
}
