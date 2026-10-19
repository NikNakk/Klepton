// SPDX-License-Identifier: MIT
// Guest Vulkan boundary smoke test. Does not load any game or OpenVR runtime.
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
const char *kl_driver_target_name(void) { return "alyx-probe"; }
void kl_glfb_note_eye_mtl_texture(int a,int b,void *c,int d,int e,int f) { abort(); }
int main(int argc, char **argv) {
 if (argc != 2) return 2;
 void *h=dlopen(argv[1], RTLD_LAZY|RTLD_LOCAL);
 if(!h) {puts(dlerror());return 1;}
 void *(*sym)(const char*)=dlsym(h,"kl_vulkan_sym");
 PFN_vkGetInstanceProcAddr gipa=(PFN_vkGetInstanceProcAddr)sym("vkGetInstanceProcAddr");
 if(!gipa) return 1;
 PFN_vkCreateInstance create=(PFN_vkCreateInstance)gipa(0,"vkCreateInstance");
 VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.apiVersion=VK_API_VERSION_1_2};
 VkInstanceCreateInfo ci={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};
 VkInstance instance;
 VkResult r=create(&ci,0,&instance);printf("CreateInstance=%d\n",r);if(r) return 1;
 PFN_vkEnumeratePhysicalDevices enumerate=(PFN_vkEnumeratePhysicalDevices)gipa(instance,"vkEnumeratePhysicalDevices");
 uint32_t n=1;VkPhysicalDevice pd; r=enumerate(instance,&n,&pd); if(r) return 1;
 VkPhysicalDeviceFragmentDensityMapFeaturesEXT fdm={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT};
 VkPhysicalDeviceFeatures2 f={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,.pNext=&fdm};
 PFN_vkGetPhysicalDeviceFeatures2 features=(PFN_vkGetPhysicalDeviceFeatures2)gipa(instance,"vkGetPhysicalDeviceFeatures2");
 features(pd,&f);printf("fdm=%u dynamic=%u\n",fdm.fragmentDensityMap,fdm.fragmentDensityMapDynamic);
 ((PFN_vkDestroyInstance)gipa(instance,"vkDestroyInstance"))(instance,0);
 return 0;
}
