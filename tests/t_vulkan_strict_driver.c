// SPDX-License-Identifier: MIT
// Host-owned fake dispatch library: checks exact forwarded requests.
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <assert.h>
#include <string.h>
static int calls;
static VkResult inst_ext(const char *layer,uint32_t *n,VkExtensionProperties *p) {
 if(layer) return VK_ERROR_LAYER_NOT_PRESENT;
 if(!p) {*n=1;return VK_SUCCESS;}
 assert(*n>=1);*n=1;strcpy(p[0].extensionName,"VK_KHR_portability_enumeration");return VK_SUCCESS;
}
static VkResult device_ext(VkPhysicalDevice pd,const char *layer,uint32_t *n,VkExtensionProperties *p) {
 if(!pd) return VK_ERROR_INITIALIZATION_FAILED;
 if(!p) {*n=2;return VK_SUCCESS;}
 assert(*n>=2);*n=2;strcpy(p[0].extensionName,"VK_KHR_portability_subset");strcpy(p[1].extensionName,"VK_EXT_metal_objects");return VK_SUCCESS;
}
static VkResult create_instance(const VkInstanceCreateInfo *ci,const VkAllocationCallbacks *a,VkInstance *out) {
 assert(ci->pApplicationInfo->apiVersion==VK_API_VERSION_1_0);
 assert(ci->enabledLayerCount==1 && !strcmp(ci->ppEnabledLayerNames[0],"VK_LAYER_TEST"));
 assert(ci->enabledExtensionCount==71);
 assert(!strcmp(ci->ppEnabledExtensionNames[0],"VK_TEST_missing"));
 assert(!strcmp(ci->ppEnabledExtensionNames[69],"VK_TEST_69"));
 assert(!strcmp(ci->ppEnabledExtensionNames[70],"VK_KHR_portability_enumeration"));
 assert(ci->flags & VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR);
 *out=(VkInstance)(uintptr_t)0x100;return VK_ERROR_EXTENSION_NOT_PRESENT;
}
static VkResult create_device(VkPhysicalDevice pd,const VkDeviceCreateInfo *ci,const VkAllocationCallbacks *a,VkDevice *out) {
 calls++;
 assert(pd==(VkPhysicalDevice)(uintptr_t)0x200);
 assert(ci->enabledLayerCount==1 && !strcmp(ci->ppEnabledLayerNames[0],"VK_LAYER_TEST"));
 assert(ci->enabledExtensionCount==2);
 assert(!strcmp(ci->ppEnabledExtensionNames[0],"VK_TEST_missing"));
 assert(!strcmp(ci->ppEnabledExtensionNames[1],"VK_KHR_portability_subset"));
 const VkPhysicalDeviceFragmentDensityMapFeaturesEXT *f=ci->pNext;
 assert(f && f->fragmentDensityMap==VK_TRUE && f->fragmentDensityMapDynamic==VK_TRUE);
 assert(ci->pEnabledFeatures->geometryShader==VK_TRUE);
 return VK_ERROR_FEATURE_NOT_PRESENT;
}
int driver_calls(void){return calls;}
static void sentinel(void){}
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance inst,const char *name) {
#define F(n,f) if(!strcmp(name,n))return (PFN_vkVoidFunction)f
 F("vkEnumerateInstanceExtensionProperties",inst_ext);F("vkCreateInstance",create_instance);
 F("vkEnumerateDeviceExtensionProperties",device_ext);F("vkCreateDevice",create_device);
 F("vkCreateGraphicsPipelines",sentinel);F("vkGetPhysicalDeviceFeatures2",sentinel);
 F("vkCreateShaderModule",sentinel);F("vkCmdBeginRenderPass",sentinel);
 F("vkCreateImage",sentinel);
 return NULL;
}
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev,const char *name) {
 return vkGetInstanceProcAddr(NULL,name);
}
