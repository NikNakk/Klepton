// SPDX-License-Identifier: MIT
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *kl_driver_target_name(void){return "strict-fixture";}
void kl_glfb_note_eye_mtl_texture(int a,int b,void*c,int d,int e,int f){abort();}
int main(int argc,char **argv){
 assert(argc==3);
 void *driver=dlopen(argv[2],RTLD_NOW|RTLD_LOCAL);assert(driver);
 void *lib=dlopen(argv[1],RTLD_LAZY|RTLD_LOCAL);assert(lib);
 void *(*sym)(const char*)=dlsym(lib,"kl_vulkan_sym");assert(sym);
 PFN_vkGetInstanceProcAddr gipa=sym("vkGetInstanceProcAddr");
 PFN_vkGetDeviceProcAddr gdpa=sym("vkGetDeviceProcAddr");
 PFN_vkGetInstanceProcAddr native=dlsym(driver,"vkGetInstanceProcAddr");
 assert(!gipa(NULL,"vkCreateDevice"));assert(!gdpa((VkDevice)1,"vkCreateInstance"));
 assert(!gipa(NULL,"vkUnknownNeverProvided"));assert(!gdpa((VkDevice)1,"vkUnknownNeverProvided"));
 assert(!gipa(NULL,"vkGetPhysicalDeviceFragmentShadingRatesKHR"));
 PFN_vkEnumerateInstanceExtensionProperties ie=(PFN_vkEnumerateInstanceExtensionProperties)gipa(NULL,"vkEnumerateInstanceExtensionProperties");
 uint32_t count=0;assert(ie("VK_LAYER_MISSING",&count,NULL)==VK_ERROR_LAYER_NOT_PRESENT);
 const char *direct[]={"vkCreateGraphicsPipelines","vkCreateShaderModule","vkGetPhysicalDeviceFeatures2","vkCmdBeginRenderPass","vkCreateImage"};
 for(unsigned i=0;i<5;i++){
  assert(gipa((VkInstance)1,direct[i])==native((VkInstance)1,direct[i]));
  assert(gdpa((VkDevice)1,direct[i])==native(NULL,direct[i]));
 }
 char buf[70][32];const char *names[71];
 for(int i=0;i<70;i++){snprintf(buf[i],32,"VK_TEST_%d",i);names[i]=buf[i];}
 names[0]="VK_TEST_missing";names[70]="VK_KHR_xcb_surface";
 const char *layer="VK_LAYER_TEST";
 VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.apiVersion=VK_API_VERSION_1_0};
 VkInstanceCreateInfo ci={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app,
 .enabledLayerCount=1,.ppEnabledLayerNames=&layer,.enabledExtensionCount=71,.ppEnabledExtensionNames=names};
 VkInstanceCreateInfo before=ci;VkInstance inst;
 PFN_vkCreateInstance create=(PFN_vkCreateInstance)gipa(NULL,"vkCreateInstance");
 assert(create(&ci,NULL,&inst)==VK_ERROR_EXTENSION_NOT_PRESENT);
 assert(!memcmp(&ci,&before,sizeof ci));assert(app.apiVersion==VK_API_VERSION_1_0);
 VkPhysicalDeviceFragmentDensityMapFeaturesEXT f={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT,.fragmentDensityMap=1,.fragmentDensityMapDynamic=1};
 VkPhysicalDeviceFeatures features={.geometryShader=1};
 VkDeviceCreateInfo dc={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.pNext=&f,.pEnabledFeatures=&features,
 .enabledExtensionCount=1,.ppEnabledExtensionNames=names,.enabledLayerCount=1,.ppEnabledLayerNames=&layer};
 VkDeviceCreateInfo db=dc;VkDevice dev;
 PFN_vkCreateDevice cd=(PFN_vkCreateDevice)gipa((VkInstance)1,"vkCreateDevice");
 assert(cd((VkPhysicalDevice)0x200,&dc,NULL,&dev)==VK_ERROR_FEATURE_NOT_PRESENT);
 int(*calls)(void)=dlsym(driver,"driver_calls");assert(calls()==1);
 assert(!memcmp(&dc,&db,sizeof dc)&&f.fragmentDensityMap&&f.fragmentDensityMapDynamic&&features.geometryShader);
 assert(cd(NULL,&dc,NULL,&dev)==VK_ERROR_INITIALIZATION_FAILED);assert(calls()==1);
 puts("PASS: strict Vulkan preserves >64 extensions, layers/API/features/pNext; no retry, null substitution, graphics repair or success stub");
}
