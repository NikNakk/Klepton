// SPDX-License-Identifier: MIT
// Guest Vulkan boundary smoke test. Does not load any game or OpenVR runtime.
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
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
 VkPhysicalDeviceProperties props;
 ((PFN_vkGetPhysicalDeviceProperties)gipa(instance,"vkGetPhysicalDeviceProperties"))(pd,&props);
 printf("device=%s vendor=%#x deviceID=%#x driverVersion=%u api=%u.%u.%u\n",props.deviceName,props.vendorID,props.deviceID,props.driverVersion,VK_API_VERSION_MAJOR(props.apiVersion),VK_API_VERSION_MINOR(props.apiVersion),VK_API_VERSION_PATCH(props.apiVersion));
 VkPhysicalDeviceDriverProperties driver={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
 VkPhysicalDeviceProperties2 p2={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,.pNext=&driver};
 ((PFN_vkGetPhysicalDeviceProperties2)gipa(instance,"vkGetPhysicalDeviceProperties2"))(pd,&p2);
 printf("driverID=%u driverName=%s driverInfo=%s\n",driver.driverID,driver.driverName,driver.driverInfo);
 uint32_t ext_count=0;PFN_vkEnumerateDeviceExtensionProperties enum_ext=(PFN_vkEnumerateDeviceExtensionProperties)gipa(instance,"vkEnumerateDeviceExtensionProperties");
 assert(enum_ext(pd,NULL,&ext_count,NULL)==VK_SUCCESS);
 VkExtensionProperties *extensions=calloc(ext_count,sizeof *extensions);assert(extensions);
 assert(enum_ext(pd,NULL,&ext_count,extensions)==VK_SUCCESS);
 for(uint32_t i=0;i<ext_count;i++)printf("extension=%s spec=%u\n",extensions[i].extensionName,extensions[i].specVersion);
 free(extensions);
 uint32_t nq=0;
 PFN_vkGetPhysicalDeviceQueueFamilyProperties queues=(PFN_vkGetPhysicalDeviceQueueFamilyProperties)gipa(instance,"vkGetPhysicalDeviceQueueFamilyProperties");
 queues(pd,&nq,NULL);assert(nq);VkQueueFamilyProperties *qp=calloc(nq,sizeof *qp);queues(pd,&nq,qp);
 uint32_t family=0;while(family<nq && !(qp[family].queueFlags&VK_QUEUE_GRAPHICS_BIT))family++;assert(family<nq);free(qp);
 float priority=1;VkDeviceQueueCreateInfo q={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};
 VkDeviceCreateInfo dc={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&q};
 VkDevice dev;PFN_vkCreateDevice cd=(PFN_vkCreateDevice)gipa(instance,"vkCreateDevice");
 r=cd(pd,&dc,NULL,&dev);printf("CreateDevice=%d\n",r);assert(r==VK_SUCCESS);
 PFN_vkGetDeviceProcAddr gdpa=(PFN_vkGetDeviceProcAddr)sym("vkGetDeviceProcAddr");
 assert(!gdpa(dev,"vkNeverProvidedTEST"));
 ((PFN_vkDestroyDevice)gdpa(dev,"vkDestroyDevice"))(dev,NULL);
 VkPhysicalDeviceFeatures request={0};
 VkBool32 *have=(VkBool32*)&f.features,*want=(VkBool32*)&request;
 size_t index=0;while(index<sizeof request/sizeof *want && have[index])index++;
 assert(index<sizeof request/sizeof *want);want[index]=VK_TRUE;dc.pEnabledFeatures=&request;
 r=cd(pd,&dc,NULL,&dev);printf("unsupportedFeatureIndex=%zu CreateDevice=%d requestedAfter=%u\n",index,r,want[index]);
 assert(r==VK_ERROR_FEATURE_NOT_PRESENT && want[index]==VK_TRUE);
 ((PFN_vkDestroyInstance)gipa(instance,"vkDestroyInstance"))(instance,0);
 return 0;
}
