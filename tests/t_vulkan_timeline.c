// Copyright 2026, Nick Kennedy
// SPDX-License-Identifier: MIT
// Real driver: profile negotiation, caller-chain integrity and GPU timeline signal.
#define VK_USE_PLATFORM_METAL_EXT
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *kl_driver_target_name(void) { return "timeline-test"; }
void kl_glfb_note_eye_mtl_texture(int a,int b,void *c,int d,int e,int f) { abort(); }
int main(int argc,char **argv) {
    assert(argc==3);
    void *library=dlopen(argv[1],RTLD_LAZY|RTLD_LOCAL);assert(library);
    void *(*sym)(const char*)=dlsym(library,"kl_vulkan_sym");assert(sym);
    PFN_vkGetInstanceProcAddr gipa=(void*)sym("vkGetInstanceProcAddr");
    PFN_vkGetDeviceProcAddr gdpa=(void*)sym("vkGetDeviceProcAddr");
    VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.apiVersion=VK_API_VERSION_1_2};
    VkInstanceCreateInfo ic={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};VkInstance instance;
    assert(((PFN_vkCreateInstance)gipa(0,"vkCreateInstance"))(&ic,0,&instance)==VK_SUCCESS);
    uint32_t n=1;VkPhysicalDevice pd;
    assert(((PFN_vkEnumeratePhysicalDevices)gipa(instance,"vkEnumeratePhysicalDevices"))(instance,&n,&pd)==VK_SUCCESS);
    float priority=1;VkDeviceQueueCreateInfo queue={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount=1,.pQueuePriorities=&priority};
    VkPhysicalDeviceTimelineSemaphoreFeatures timeline={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES};
    VkPhysicalDeviceVulkan12Features core={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkDeviceCreateInfo dc={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&queue};
    int reject=0;
    if(!strcmp(argv[2],"false")) { dc.pNext=&timeline;reject=1; }
    if(!strcmp(argv[2],"true")) { dc.pNext=&timeline;timeline.timelineSemaphore=VK_TRUE; }
    if(!strcmp(argv[2],"v12false")) { dc.pNext=&core;reject=1; }
    if(!strcmp(argv[2],"v12true")) { dc.pNext=&core;core.timelineSemaphore=VK_TRUE; }
    if(!strcmp(argv[2],"duplicate")) { dc.pNext=&timeline;timeline.timelineSemaphore=VK_TRUE;
        timeline.pNext=&core;core.timelineSemaphore=VK_TRUE;reject=1; }
    const char *metal="VK_EXT_metal_objects";
    dc.enabledExtensionCount=1;dc.ppEnabledExtensionNames=&metal;
    VkDeviceCreateInfo before=dc;VkPhysicalDeviceTimelineSemaphoreFeatures tbefore=timeline;
    VkPhysicalDeviceVulkan12Features cbefore=core;VkDevice device;
    VkResult result=((PFN_vkCreateDevice)gipa(instance,"vkCreateDevice"))(pd,&dc,0,&device);
    assert(!memcmp(&dc,&before,sizeof dc) && !memcmp(&timeline,&tbefore,sizeof timeline) && !memcmp(&core,&cbefore,sizeof core));
    if(reject) assert(result==VK_ERROR_FEATURE_NOT_PRESENT);
    else {
        assert(result==VK_SUCCESS);
        VkSemaphoreTypeCreateInfo type={.sType=VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,.semaphoreType=VK_SEMAPHORE_TYPE_TIMELINE};
        VkExportMetalObjectCreateInfoEXT export_create={.sType=VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECT_CREATE_INFO_EXT,
            .pNext=&type,.exportObjectType=VK_EXPORT_METAL_OBJECT_TYPE_METAL_SHARED_EVENT_BIT_EXT};
        VkSemaphoreCreateInfo sc={.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,.pNext=&export_create};VkSemaphore semaphore;
        assert(((PFN_vkCreateSemaphore)gdpa(device,"vkCreateSemaphore"))(device,&sc,0,&semaphore)==VK_SUCCESS);
        VkQueue q;((PFN_vkGetDeviceQueue)gdpa(device,"vkGetDeviceQueue"))(device,0,0,&q);
        uint64_t value=7;VkTimelineSemaphoreSubmitInfo ts={.sType=VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
            .signalSemaphoreValueCount=1,.pSignalSemaphoreValues=&value};
        VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.pNext=&ts,.signalSemaphoreCount=1,.pSignalSemaphores=&semaphore};
        assert(((PFN_vkQueueSubmit)gdpa(device,"vkQueueSubmit"))(q,1,&submit,VK_NULL_HANDLE)==VK_SUCCESS);
        VkSemaphoreWaitInfo wait={.sType=VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,.semaphoreCount=1,.pSemaphores=&semaphore,.pValues=&value};
        assert(((PFN_vkWaitSemaphores)gdpa(device,"vkWaitSemaphores"))(device,&wait,5000000000ull)==VK_SUCCESS);
        uint64_t actual=0;
        assert(((PFN_vkGetSemaphoreCounterValue)gdpa(device,"vkGetSemaphoreCounterValue"))(device,semaphore,&actual)==VK_SUCCESS && actual>=value);
        VkExportMetalSharedEventInfoEXT exported={.sType=VK_STRUCTURE_TYPE_EXPORT_METAL_SHARED_EVENT_INFO_EXT,.semaphore=semaphore};
        VkExportMetalObjectsInfoEXT objects={.sType=VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECTS_INFO_EXT,.pNext=&exported};
        ((PFN_vkExportMetalObjectsEXT)gdpa(device,"vkExportMetalObjectsEXT"))(device,&objects);
        assert(exported.mtlSharedEvent);
        // Import at the current value, as Monado does, then signal the alias on
        // the GPU. The original semaphore must observe the same event value.
        type.initialValue=actual;
        VkImportMetalSharedEventInfoEXT imported={.sType=VK_STRUCTURE_TYPE_IMPORT_METAL_SHARED_EVENT_INFO_EXT,
            .pNext=&type,.mtlSharedEvent=exported.mtlSharedEvent};
        sc.pNext=&imported;VkSemaphore alias;
        assert(((PFN_vkCreateSemaphore)gdpa(device,"vkCreateSemaphore"))(device,&sc,0,&alias)==VK_SUCCESS);
        value=11;submit.pSignalSemaphores=&alias;
        assert(((PFN_vkQueueSubmit)gdpa(device,"vkQueueSubmit"))(q,1,&submit,VK_NULL_HANDLE)==VK_SUCCESS);
        assert(((PFN_vkWaitSemaphores)gdpa(device,"vkWaitSemaphores"))(device,&wait,5000000000ull)==VK_SUCCESS);
        assert(((PFN_vkGetSemaphoreCounterValue)gdpa(device,"vkGetSemaphoreCounterValue"))(device,semaphore,&actual)==VK_SUCCESS && actual>=value);
        ((PFN_vkDestroySemaphore)gdpa(device,"vkDestroySemaphore"))(device,alias,0);
        ((PFN_vkDestroySemaphore)gdpa(device,"vkDestroySemaphore"))(device,semaphore,0);
        ((PFN_vkDestroyDevice)gdpa(device,"vkDestroyDevice"))(device,0);
    }
    ((PFN_vkDestroyInstance)gipa(instance,"vkDestroyInstance"))(instance,0);
    printf("PASS timeline %s: result=%d, caller data unchanged\n",argv[2],result);
}
