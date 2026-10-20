// Copyright 2026, Nick Kennedy
// SPDX-License-Identifier: MIT
// Real-driver test for the explicit native Alyx sample-location profile.
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *kl_driver_target_name(void) { return "sample-locations-test"; }
void kl_glfb_note_eye_mtl_texture(int a,int b,void *c,int d,int e,int f) { abort(); }
int main(int argc,char **argv) {
    assert(argc==3);int enabled=atoi(argv[2]);
    void *library=dlopen(argv[1],RTLD_LAZY|RTLD_LOCAL);assert(library);
    void *(*sym)(const char*)=dlsym(library,"kl_vulkan_sym");assert(sym);
    PFN_vkGetInstanceProcAddr gipa=(void*)sym("vkGetInstanceProcAddr");
    PFN_vkGetDeviceProcAddr gdpa=(void*)sym("vkGetDeviceProcAddr");
    VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.apiVersion=VK_API_VERSION_1_2};
    VkInstanceCreateInfo ic={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};
    VkInstance instance;
    assert(((PFN_vkCreateInstance)gipa(0,"vkCreateInstance"))(&ic,0,&instance)==VK_SUCCESS);
    uint32_t n=1;VkPhysicalDevice pd;
    assert(((PFN_vkEnumeratePhysicalDevices)gipa(instance,"vkEnumeratePhysicalDevices"))(instance,&n,&pd)==VK_SUCCESS);
    float priority=1;VkDeviceQueueCreateInfo queue={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount=1,.pQueuePriorities=&priority};
    const char *extension="VK_EXT_sample_locations";
    VkDeviceCreateInfo dc={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,
        .pQueueCreateInfos=&queue,.enabledExtensionCount=0,.ppEnabledExtensionNames=&extension};
    VkDeviceCreateInfo before=dc;VkDevice device;
    PFN_vkCreateDevice create=(void*)gipa(instance,"vkCreateDevice");
    assert(create(pd,&dc,0,&device)==VK_SUCCESS);
    assert(!memcmp(&before,&dc,sizeof dc));
    PFN_vkCmdSetSampleLocationsEnableEXT sample=(void*)gdpa(device,"vkCmdSetSampleLocationsEnableEXT");
    printf("profile=%d native-command-available=%d\n",enabled,sample!=NULL);
    assert((sample!=NULL)==enabled);
    if(enabled) {
        VkCommandPoolCreateInfo pc={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        VkCommandPool pool;
        assert(((PFN_vkCreateCommandPool)gdpa(device,"vkCreateCommandPool"))(device,&pc,0,&pool)==VK_SUCCESS);
        VkCommandBufferAllocateInfo ac={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool=pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};
        VkCommandBuffer command;
        assert(((PFN_vkAllocateCommandBuffers)gdpa(device,"vkAllocateCommandBuffers"))(device,&ac,&command)==VK_SUCCESS);
        VkCommandBufferBeginInfo bc={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        assert(((PFN_vkBeginCommandBuffer)gdpa(device,"vkBeginCommandBuffer"))(command,&bc)==VK_SUCCESS);
        sample(command,VK_TRUE);sample(command,VK_FALSE);
        assert(((PFN_vkEndCommandBuffer)gdpa(device,"vkEndCommandBuffer"))(command)==VK_SUCCESS);
        VkQueue q;((PFN_vkGetDeviceQueue)gdpa(device,"vkGetDeviceQueue"))(device,0,0,&q);
        VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&command};
        assert(((PFN_vkQueueSubmit)gdpa(device,"vkQueueSubmit"))(q,1,&submit,VK_NULL_HANDLE)==VK_SUCCESS);
        assert(((PFN_vkQueueWaitIdle)gdpa(device,"vkQueueWaitIdle"))(q)==VK_SUCCESS);
        ((PFN_vkDestroyCommandPool)gdpa(device,"vkDestroyCommandPool"))(device,pool,0);
        puts("PASS: native sample-location enable/disable commands record, submit and complete");
    }
    ((PFN_vkDestroyDevice)gdpa(device,"vkDestroyDevice"))(device,0);
    if(enabled) {
        VkPhysicalDeviceExtendedDynamicState3FeaturesEXT existing={
            .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT};
        dc.pNext=&existing;
        assert(create(pd,&dc,0,&device)==VK_ERROR_FEATURE_NOT_PRESENT);
        assert(existing.pNext==NULL && !existing.extendedDynamicState3SampleLocationsEnable);
        existing.extendedDynamicState3SampleLocationsEnable=VK_TRUE;
        assert(create(pd,&dc,0,&device)==VK_SUCCESS);
        assert(existing.pNext==NULL && existing.extendedDynamicState3SampleLocationsEnable);
        ((PFN_vkDestroyDevice)gdpa(device,"vkDestroyDevice"))(device,0);
        existing.extendedDynamicState3ColorWriteMask=VK_TRUE;
        assert(create(pd,&dc,0,&device)==VK_ERROR_FEATURE_NOT_PRESENT);
        assert(existing.extendedDynamicState3ColorWriteMask==VK_TRUE);
        existing.extendedDynamicState3ColorWriteMask=VK_FALSE;
        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT state1={
            .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,.pNext=&existing};
        dc.pNext=&state1;
        assert(create(pd,&dc,0,&device)==VK_ERROR_FEATURE_NOT_PRESENT);
        assert(!state1.extendedDynamicState && state1.pNext==&existing);
        state1.extendedDynamicState=VK_TRUE;
        assert(create(pd,&dc,0,&device)==VK_SUCCESS);
        assert(state1.extendedDynamicState && state1.pNext==&existing && existing.pNext==NULL);
        ((PFN_vkDestroyDevice)gdpa(device,"vkDestroyDevice"))(device,0);
        puts("PASS: existing EDS1/EDS3 feature chains preserved; unsupported color-write-mask requests still fail");
    }
    ((PFN_vkDestroyInstance)gipa(instance,"vkDestroyInstance"))(instance,0);
    return 0;
}
