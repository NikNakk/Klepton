// Copyright 2026, Nick Kennedy
// SPDX-License-Identifier: MIT
// Included only by kl_vulkan.c. Strict-mode observation bypasses every legacy
// graphics workaround. Each trampoline retains the exact native dispatch target.
// If another instance/device produces a different target, leave it unwrapped.
#define OBS_TARGET(name) static PFN_vkVoidFunction obs_native_##name
#define OBS_CALL(name, ...) ((PFN_vk##name)obs_native_##name)(__VA_ARGS__)
#define OBS_QUERY(name, type) \
 OBS_TARGET(name); \
 static void VKAPI_CALL obs_##name(VkPhysicalDevice pd, type *out) { \
    VKI("fdm-observe " #name " called\n"); \
    if (out) trace_chain_impl("observe " #name " request", out->pNext, 0); \
    OBS_CALL(name, pd, out); \
    if (out) trace_chain("observe " #name " result", out->pNext); \
 }
OBS_QUERY(GetPhysicalDeviceFeatures2, VkPhysicalDeviceFeatures2)
OBS_QUERY(GetPhysicalDeviceFeatures2KHR, VkPhysicalDeviceFeatures2)
OBS_QUERY(GetPhysicalDeviceProperties2, VkPhysicalDeviceProperties2)
OBS_QUERY(GetPhysicalDeviceProperties2KHR, VkPhysicalDeviceProperties2)
#undef OBS_QUERY

OBS_TARGET(CreateImage);
static VkResult VKAPI_CALL obs_CreateImage(VkDevice dev, const VkImageCreateInfo *ci,
        const VkAllocationCallbacks *allocator, VkImage *out) {
    if (ci) {
        static unsigned logged;
        if (__atomic_fetch_add(&logged, 1, __ATOMIC_RELAXED) < 128 ||
            (ci->usage & VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT) ||
            (ci->flags & VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT)) {
            VKI("fdm-observe CreateImage extent=%ux%ux%u layers=%u format=%u usage=%#x flags=%#x samples=%u\n",
                ci->extent.width, ci->extent.height, ci->extent.depth, ci->arrayLayers,
                ci->format, ci->usage, ci->flags, ci->samples);
            trace_chain("observe CreateImage", ci->pNext);
        }
    }
    return OBS_CALL(CreateImage, dev, ci, allocator, out);
}

#define OBS_PASS(name, type) \
 OBS_TARGET(name); \
 static VkResult VKAPI_CALL obs_##name(VkDevice dev, const type *ci, \
        const VkAllocationCallbacks *allocator, VkRenderPass *out) { \
    static unsigned logged; \
    if (ci && __atomic_fetch_add(&logged, 1, __ATOMIC_RELAXED) < 16) \
        VKI("fdm-observe " #name " attachments=%u subpasses=%u\n", ci->attachmentCount, ci->subpassCount); \
    if (ci) trace_chain("observe " #name, ci->pNext); \
    return OBS_CALL(name, dev, ci, allocator, out); \
 }
OBS_PASS(CreateRenderPass, VkRenderPassCreateInfo)
OBS_PASS(CreateRenderPass2, VkRenderPassCreateInfo2)
OBS_PASS(CreateRenderPass2KHR, VkRenderPassCreateInfo2)
#undef OBS_PASS

#define OBS_RENDER(name) \
 OBS_TARGET(name); \
 static void VKAPI_CALL obs_##name(VkCommandBuffer cb, const VkRenderingInfo *info) { \
    if (info) { \
        static unsigned logged; \
        if (__atomic_fetch_add(&logged, 1, __ATOMIC_RELAXED) < 16) \
            VKI("fdm-observe " #name " area=%ux%u layers=%u viewMask=%#x\n", \
                info->renderArea.extent.width, info->renderArea.extent.height, info->layerCount, info->viewMask); \
        trace_chain("observe " #name, info->pNext); \
    } \
    OBS_CALL(name, cb, info); \
 }
OBS_RENDER(CmdBeginRendering)
OBS_RENDER(CmdBeginRenderingKHR)
#undef OBS_RENDER

#define OBS_END(name) \
 OBS_TARGET(name); \
 static void VKAPI_CALL obs_##name(VkCommandBuffer cb, const VkSubpassEndInfo *info) { \
    if (info) trace_chain("observe " #name, info->pNext); \
    OBS_CALL(name, cb, info); \
 }
OBS_END(CmdEndRenderPass2)
OBS_END(CmdEndRenderPass2KHR)
#undef OBS_END

#define OBS_RENDER_END(name) \
 OBS_TARGET(name); \
 static void VKAPI_CALL obs_##name(VkCommandBuffer cb, const VkRenderingEndInfoKHR *info) { \
    static unsigned calls; \
    unsigned call = __atomic_fetch_add(&calls, 1, __ATOMIC_RELAXED); \
    if (call < 16 || !(call % 600)) { \
        VKI("fdm-observe " #name " called cb=%p call=%u\n", (void *)cb, call); \
        if (info) trace_chain("observe " #name, info->pNext); \
    } \
    OBS_CALL(name, cb, info); \
 }
OBS_RENDER_END(CmdEndRendering2EXT)
OBS_RENDER_END(CmdEndRendering2KHR)
#undef OBS_RENDER_END

OBS_TARGET(CreateShaderModule);
static VkResult VKAPI_CALL obs_CreateShaderModule(VkDevice dev, const VkShaderModuleCreateInfo *ci,
        const VkAllocationCallbacks *allocator, VkShaderModule *out) {
    // Enumerated metadata only: do not copy or rewrite the game's shader code.
    if (ci && ci->pCode && ci->codeSize >= 20 && !(ci->codeSize & 3) && ci->pCode[0] == 0x07230203) {
        const uint32_t *words = ci->pCode;
        for (size_t at = 5; at < ci->codeSize / 4;) {
            uint32_t length = words[at] >> 16, opcode = words[at] & 0xffff;
            if (!length || length > ci->codeSize / 4 - at) break;
            if (opcode == 17 && length >= 2) VKI("fdm-observe shader capability=%u\n", words[at+1]);
            if (opcode == 71 && length >= 4 && words[at+2] == 11)
                VKI("fdm-observe shader builtin=%u target=%u\n", words[at+3], words[at+1]);
            at += length;
        }
    }
    return OBS_CALL(CreateShaderModule, dev, ci, allocator, out);
}

OBS_TARGET(EnumerateDeviceExtensionProperties);
static VkResult VKAPI_CALL obs_EnumerateDeviceExtensionProperties(VkPhysicalDevice pd, const char *layer,
        uint32_t *count, VkExtensionProperties *properties) {
    VkResult result = OBS_CALL(EnumerateDeviceExtensionProperties, pd, layer, count, properties);
    VKI("fdm-observe EnumerateDeviceExtensionProperties result=%d count=%u payload=%u\n",
        result, (result == VK_SUCCESS || result == VK_INCOMPLETE) && count ? *count : 0, properties != NULL);
    if ((result == VK_SUCCESS || result == VK_INCOMPLETE) && count && properties)
        for (uint32_t i = 0; i < *count; i++)
            if (strstr(properties[i].extensionName, "fragment_density") || strstr(properties[i].extensionName, "fragment_shading_rate"))
                VKI("fdm-observe available-extension=%s\n", properties[i].extensionName);
    return result;
}
OBS_TARGET(CmdSetFragmentShadingRateKHR);
static void VKAPI_CALL obs_CmdSetFragmentShadingRateKHR(VkCommandBuffer cb, const VkExtent2D *size,
        const VkFragmentShadingRateCombinerOpKHR operations[2]) {
    if (size && operations) VKI("fdm-observe CmdSetFragmentShadingRateKHR size=%ux%u combine=%u/%u\n",
        size->width, size->height, operations[0], operations[1]);
    OBS_CALL(CmdSetFragmentShadingRateKHR, cb, size, operations);
}

static PFN_vkVoidFunction strict_observe_proc(const char *name, PFN_vkVoidFunction native) {
    native = strict_resolve_observe_proc(name, native);
    if (!native || !vk_strict() || !fdm_trace() || !kl_env_on("KL_VK_FOVEATION_OBSERVE", 0)) return native;
#define ENTRY(n) {"vk" #n, (PFN_vkVoidFunction)obs_##n, &obs_native_##n}
    static const struct { const char *name; PFN_vkVoidFunction wrapper; PFN_vkVoidFunction *native; } entries[] = {
        ENTRY(EnumerateDeviceExtensionProperties), ENTRY(CmdSetFragmentShadingRateKHR),
        ENTRY(GetPhysicalDeviceFeatures2), ENTRY(GetPhysicalDeviceFeatures2KHR),
        ENTRY(GetPhysicalDeviceProperties2), ENTRY(GetPhysicalDeviceProperties2KHR),
        ENTRY(CreateImage), ENTRY(CreateRenderPass), ENTRY(CreateRenderPass2), ENTRY(CreateRenderPass2KHR),
        ENTRY(CmdBeginRendering), ENTRY(CmdBeginRenderingKHR), ENTRY(CmdEndRenderPass2), ENTRY(CmdEndRenderPass2KHR),
        ENTRY(CmdEndRendering2EXT), ENTRY(CmdEndRendering2KHR), ENTRY(CreateShaderModule),
    };
#undef ENTRY
    for (size_t i = 0; i < sizeof entries / sizeof entries[0]; i++) {
        if (strcmp(name, entries[i].name)) continue;
        pthread_mutex_lock(&g_lock);
        if (!*entries[i].native) *entries[i].native = native;
        int compatible = *entries[i].native == native;
        pthread_mutex_unlock(&g_lock);
        if (!compatible) {
            static unsigned bypasses;
            if (__atomic_fetch_add(&bypasses, 1, __ATOMIC_RELAXED) < 16)
                VKI("fdm-observe dispatch-bypass name=%s native_target_changed\n", name);
        }
        // No substitution across distinct layer/driver dispatch tables.
        return compatible ? entries[i].wrapper : native;
    }
    return native;
}
#undef OBS_TARGET
#undef OBS_CALL
