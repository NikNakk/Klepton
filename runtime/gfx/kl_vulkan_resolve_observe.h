// Copyright 2026, Nick Kennedy
// SPDX-License-Identifier: MIT
// Included by kl_vulkan.c; observations preserve native objects and dispatch.
static int resolve_trace(void) { return kl_env_on("KL_VK_CUSTOM_RESOLVE_TRACE", 0); }
static int resolve_budget(unsigned *count, unsigned limit) {
    unsigned n=__atomic_fetch_add(count,1,__ATOMIC_RELAXED);
    if(n==limit) VKI("resolve-observe budget-exhausted limit=%u\n",limit);
    return n<limit;
}
static void resolve_chain(const char *where,const void *chain) {
    for(const VkBaseInStructure *p=chain;p;p=p->pNext) {
        VKI("resolve-observe %s pNext=%u\n",where,p->sType);
        if(p->sType==VK_STRUCTURE_TYPE_CUSTOM_RESOLVE_CREATE_INFO_EXT) {
            const VkCustomResolveCreateInfoEXT *c=(const void*)p;
            VKI("resolve-observe %s custom=%u colors=%u depth=%u stencil=%u\n",where,c->customResolve,c->colorAttachmentCount,c->depthAttachmentFormat,c->stencilAttachmentFormat);
            for(uint32_t i=0;c->pColorAttachmentFormats && i<c->colorAttachmentCount && i<16;i++)
                VKI("resolve-observe %s resolve-color=%u format=%u\n",where,i,c->pColorAttachmentFormats[i]);
        } else if(p->sType==VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO) {
            const VkPipelineRenderingCreateInfo *c=(const void*)p;
            VKI("resolve-observe %s viewMask=%#x colors=%u depth=%u stencil=%u\n",where,c->viewMask,c->colorAttachmentCount,c->depthAttachmentFormat,c->stencilAttachmentFormat);
            for(uint32_t i=0;c->pColorAttachmentFormats && i<c->colorAttachmentCount && i<16;i++)
                VKI("resolve-observe %s source-color=%u format=%u\n",where,i,c->pColorAttachmentFormats[i]);
        } else if(p->sType==VK_STRUCTURE_TYPE_RENDERING_INPUT_ATTACHMENT_INDEX_INFO) {
            const VkRenderingInputAttachmentIndexInfo *c=(const void*)p;
            for(uint32_t i=0;c->pColorAttachmentInputIndices && i<c->colorAttachmentCount && i<16;i++)
                VKI("resolve-observe %s color=%u input-index=%u\n",where,i,c->pColorAttachmentInputIndices[i]);
            if(c->pDepthInputAttachmentIndex) VKI("resolve-observe %s depth-input-index=%u\n",where,*c->pDepthInputAttachmentIndex);
            if(c->pStencilInputAttachmentIndex) VKI("resolve-observe %s stencil-input-index=%u\n",where,*c->pStencilInputAttachmentIndex);
        } else if(p->sType==VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_LOCATION_INFO) {
            const VkRenderingAttachmentLocationInfo *c=(const void*)p;
            for(uint32_t i=0;c->pColorAttachmentLocations && i<c->colorAttachmentCount && i<16;i++)
                VKI("resolve-observe %s color=%u location=%u\n",where,i,c->pColorAttachmentLocations[i]);
        }
    }
}
static void resolve_attachment(const char *kind,uint32_t i,const VkRenderingAttachmentInfo *a) {
    if(!a) return;
    VKI("resolve-observe %s=%u view=%#llx layout=%u mode=%#x resolve-view=%#llx resolve-layout=%u load=%u store=%u\n",
        kind,i,(unsigned long long)a->imageView,a->imageLayout,a->resolveMode,(unsigned long long)a->resolveImageView,
        a->resolveImageLayout,a->loadOp,a->storeOp);
    resolve_chain(kind,a->pNext);
}
#define R_TARGET(n) static PFN_vkVoidFunction resolve_native_##n
#define R_CALL(n,...) ((PFN_vk##n)resolve_native_##n)(__VA_ARGS__)
R_TARGET(CreateImage);
static VkResult VKAPI_CALL resolve_CreateImage(VkDevice d,const VkImageCreateInfo *ci,const VkAllocationCallbacks *a,VkImage *out) {
    VkResult r=R_CALL(CreateImage,d,ci,a,out); static unsigned ordinary,render;
    int attachment=ci && (ci->usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT));
    if(ci && resolve_budget(attachment?&render:&ordinary,attachment?512:32)) {
        VKI("resolve-observe CreateImage result=%d image=%#llx format=%u size=%ux%ux%u layers=%u samples=%u usage=%#x flags=%#x\n",
            r,r==VK_SUCCESS && out?(unsigned long long)*out:0,ci->format,ci->extent.width,ci->extent.height,ci->extent.depth,
            ci->arrayLayers,ci->samples,ci->usage,ci->flags); resolve_chain("CreateImage",ci->pNext);
    } return r;
}
R_TARGET(CreateImageView);
static VkResult VKAPI_CALL resolve_CreateImageView(VkDevice d,const VkImageViewCreateInfo *ci,const VkAllocationCallbacks *a,VkImageView *out) {
    VkResult r=R_CALL(CreateImageView,d,ci,a,out); static unsigned count;
    if(ci && resolve_budget(&count,1024)) {
        VKI("resolve-observe CreateImageView result=%d view=%#llx image=%#llx format=%u type=%u aspect=%#x mip=%u levels=%u layer=%u layers=%u\n",
            r,r==VK_SUCCESS && out?(unsigned long long)*out:0,(unsigned long long)ci->image,ci->format,ci->viewType,
            ci->subresourceRange.aspectMask,ci->subresourceRange.baseMipLevel,ci->subresourceRange.levelCount,
            ci->subresourceRange.baseArrayLayer,ci->subresourceRange.layerCount);
    } return r;
}
R_TARGET(CreateGraphicsPipelines);
static VkResult VKAPI_CALL resolve_CreateGraphicsPipelines(VkDevice d,VkPipelineCache cache,uint32_t n,const VkGraphicsPipelineCreateInfo *ci,const VkAllocationCallbacks *a,VkPipeline *out) {
    VkResult r=R_CALL(CreateGraphicsPipelines,d,cache,n,ci,a,out); static unsigned ordinary,custom;
    for(uint32_t i=0;ci && i<n;i++) {
        int has_custom=0;
        for(const VkBaseInStructure *p=ci[i].pNext;p;p=p->pNext) if(p->sType==VK_STRUCTURE_TYPE_CUSTOM_RESOLVE_CREATE_INFO_EXT) has_custom|=((const VkCustomResolveCreateInfoEXT*)p)->customResolve!=VK_FALSE;
        if(!resolve_budget(has_custom?&custom:&ordinary,has_custom?256:32)) continue;
        VKI("resolve-observe CreateGraphicsPipelines result=%d pipeline=%#llx pass=%#llx subpass=%u flags=%#x layout=%#llx\n",
            r,r==VK_SUCCESS && out?(unsigned long long)out[i]:0,(unsigned long long)ci[i].renderPass,ci[i].subpass,ci[i].flags,(unsigned long long)ci[i].layout);
        resolve_chain("pipeline",ci[i].pNext);
        if(ci[i].pMultisampleState) VKI("resolve-observe pipeline samples=%u sample-shading=%u\n",ci[i].pMultisampleState->rasterizationSamples,ci[i].pMultisampleState->sampleShadingEnable);
        if(ci[i].pDepthStencilState) VKI("resolve-observe pipeline depth-test=%u depth-write=%u depth-compare=%u stencil=%u\n",
            ci[i].pDepthStencilState->depthTestEnable,ci[i].pDepthStencilState->depthWriteEnable,ci[i].pDepthStencilState->depthCompareOp,ci[i].pDepthStencilState->stencilTestEnable);
        for(uint32_t s=0;ci[i].pStages && s<ci[i].stageCount && s<8;s++)
            VKI("resolve-observe pipeline stage=%#x module=%#llx\n",ci[i].pStages[s].stage,(unsigned long long)ci[i].pStages[s].module);
    } return r;
}
#define R_PASS(n,type) \
 R_TARGET(n); \
 static VkResult VKAPI_CALL resolve_##n(VkDevice d,const type *ci,const VkAllocationCallbacks *a,VkRenderPass *out) { \
    VkResult r=R_CALL(n,d,ci,a,out); static unsigned count; \
    if(ci && resolve_budget(&count,128)) { \
        VKI("resolve-observe " #n " result=%d pass=%#llx attachments=%u subpasses=%u\n",r,r==VK_SUCCESS && out?(unsigned long long)*out:0,ci->attachmentCount,ci->subpassCount); \
        resolve_chain(#n,ci->pNext); \
        for(uint32_t i=0;ci->pAttachments && i<ci->attachmentCount && i<32;i++) { \
            VKI("resolve-observe pass-attachment=%u format=%u samples=%u load=%u store=%u stencil-load=%u stencil-store=%u initial=%u final=%u\n",i, \
                ci->pAttachments[i].format,ci->pAttachments[i].samples,ci->pAttachments[i].loadOp,ci->pAttachments[i].storeOp, \
                ci->pAttachments[i].stencilLoadOp,ci->pAttachments[i].stencilStoreOp,ci->pAttachments[i].initialLayout,ci->pAttachments[i].finalLayout); \
        } \
        for(uint32_t i=0;ci->pSubpasses && i<ci->subpassCount && i<16;i++) { \
            VKI("resolve-observe subpass=%u flags=%#x inputs=%u colors=%u\n",i,ci->pSubpasses[i].flags,ci->pSubpasses[i].inputAttachmentCount,ci->pSubpasses[i].colorAttachmentCount); \
            for(uint32_t j=0;ci->pSubpasses[i].pInputAttachments && j<ci->pSubpasses[i].inputAttachmentCount && j<16;j++) \
                VKI("resolve-observe subpass=%u input=%u attachment=%u layout=%u\n",i,j,ci->pSubpasses[i].pInputAttachments[j].attachment,ci->pSubpasses[i].pInputAttachments[j].layout); \
            for(uint32_t j=0;ci->pSubpasses[i].pColorAttachments && j<ci->pSubpasses[i].colorAttachmentCount && j<16;j++) \
                VKI("resolve-observe subpass=%u color=%u attachment=%u layout=%u\n",i,j,ci->pSubpasses[i].pColorAttachments[j].attachment,ci->pSubpasses[i].pColorAttachments[j].layout); \
            if(ci->pSubpasses[i].pDepthStencilAttachment) \
                VKI("resolve-observe subpass=%u depth-stencil=%u layout=%u\n",i,ci->pSubpasses[i].pDepthStencilAttachment->attachment,ci->pSubpasses[i].pDepthStencilAttachment->layout); \
        } \
    } return r; \
 }
R_PASS(CreateRenderPass,VkRenderPassCreateInfo)
R_PASS(CreateRenderPass2,VkRenderPassCreateInfo2)
R_PASS(CreateRenderPass2KHR,VkRenderPassCreateInfo2)
#undef R_PASS
#define R_RENDER(n) \
 R_TARGET(n); \
 static void VKAPI_CALL resolve_##n(VkCommandBuffer cmd,const VkRenderingInfo *ri) { \
    static unsigned ordinary,custom; \
    if(ri && resolve_budget((ri->flags&VK_RENDERING_CUSTOM_RESOLVE_BIT_EXT)?&custom:&ordinary,(ri->flags&VK_RENDERING_CUSTOM_RESOLVE_BIT_EXT)?128:16)) { \
        VKI("resolve-observe " #n " command=%p flags=%#x area=%d,%d/%ux%u layers=%u viewMask=%#x colors=%u\n", \
            (void*)cmd,ri->flags,ri->renderArea.offset.x,ri->renderArea.offset.y,ri->renderArea.extent.width,ri->renderArea.extent.height,ri->layerCount,ri->viewMask,ri->colorAttachmentCount); \
        resolve_chain(#n,ri->pNext); \
        for(uint32_t i=0;ri->pColorAttachments && i<ri->colorAttachmentCount && i<16;i++) resolve_attachment("color",i,&ri->pColorAttachments[i]); \
        resolve_attachment("depth",0,ri->pDepthAttachment); resolve_attachment("stencil",0,ri->pStencilAttachment); \
    } R_CALL(n,cmd,ri); \
 }
R_RENDER(CmdBeginRendering)
R_RENDER(CmdBeginRenderingKHR)
#undef R_RENDER
static _Thread_local VkCommandBuffer resolve_active_command;
R_TARGET(CmdBeginCustomResolveEXT);
static void VKAPI_CALL resolve_CmdBeginCustomResolveEXT(VkCommandBuffer cmd,const VkBeginCustomResolveInfoEXT *info) {
    resolve_active_command=cmd;
    static unsigned count;
    if(resolve_budget(&count,128)) {VKI("resolve-observe CmdBeginCustomResolveEXT command=%p\n",(void*)cmd);if(info) resolve_chain("begin-custom",info->pNext);}
    R_CALL(CmdBeginCustomResolveEXT,cmd,info);
}
R_TARGET(CmdBindPipeline);
static void VKAPI_CALL resolve_CmdBindPipeline(VkCommandBuffer cmd,VkPipelineBindPoint point,VkPipeline pipeline) {
    static unsigned ordinary,custom;
    if(resolve_budget(resolve_active_command==cmd?&custom:&ordinary,resolve_active_command==cmd?128:16)) VKI("resolve-observe CmdBindPipeline command=%p bind=%u pipeline=%#llx\n",(void*)cmd,point,(unsigned long long)pipeline);
    R_CALL(CmdBindPipeline,cmd,point,pipeline);
}
R_TARGET(CmdDraw);
static void VKAPI_CALL resolve_CmdDraw(VkCommandBuffer cmd,uint32_t vertices,uint32_t instances,uint32_t first,uint32_t first_instance) {
    static unsigned count;
    if(resolve_active_command==cmd && resolve_budget(&count,128))
        VKI("resolve-observe custom CmdDraw command=%p vertices=%u instances=%u first=%u first-instance=%u\n",(void*)cmd,vertices,instances,first,first_instance);
    R_CALL(CmdDraw,cmd,vertices,instances,first,first_instance);
}
#define R_END(n) \
 R_TARGET(n); \
 static void VKAPI_CALL resolve_##n(VkCommandBuffer cmd) { \
    if(resolve_active_command==cmd) resolve_active_command=VK_NULL_HANDLE; \
    R_CALL(n,cmd); \
 }
R_END(CmdEndRendering)
R_END(CmdEndRenderingKHR)
#undef R_END
R_TARGET(CreateShaderModule);
static VkResult VKAPI_CALL resolve_CreateShaderModule(VkDevice d,const VkShaderModuleCreateInfo *ci,const VkAllocationCallbacks *a,VkShaderModule *out) {
    VkResult r=R_CALL(CreateShaderModule,d,ci,a,out); static unsigned count;
    if(ci && ci->pCode && ci->codeSize>=20 && !(ci->codeSize&3) && ci->pCode[0]==0x07230203 && resolve_budget(&count,1024)) {
        unsigned image_reads=0,image_fetches=0;
        VKI("resolve-observe CreateShaderModule result=%d module=%#llx words=%zu\n",r,r==VK_SUCCESS && out?(unsigned long long)*out:0,ci->codeSize/4);
        for(size_t at=5;at<ci->codeSize/4;) {
            const uint32_t *w=&ci->pCode[at]; uint32_t length=w[0]>>16,op=w[0]&65535;
            if(!length || length>ci->codeSize/4-at) break;
            if(op==25 && length>=9 && w[3]==6)
                VKI("resolve-observe shader subpass-image id=%u array=%u ms=%u sampled=%u\n",w[1],w[5],w[6],w[7]);
            if(op==71 && length>=4 && (w[2]==43 || w[2]==11))
                VKI("resolve-observe shader decoration target=%u kind=%u value=%u\n",w[1],w[2],w[3]);
            image_reads+=op==98; image_fetches+=op==95; at+=length;
        }
        VKI("resolve-observe shader image-reads=%u image-fetches=%u\n",image_reads,image_fetches);
    } return r;
}
static PFN_vkVoidFunction strict_resolve_observe_proc(const char *name,PFN_vkVoidFunction native) {
    if(!native || !vk_strict() || !resolve_trace()) return native;
#define R_ENTRY(n) {"vk" #n,(PFN_vkVoidFunction)resolve_##n,&resolve_native_##n}
    static const struct {const char *name; PFN_vkVoidFunction wrapper; PFN_vkVoidFunction *native;} entries[]={
        R_ENTRY(CreateImage),R_ENTRY(CreateImageView),R_ENTRY(CreateGraphicsPipelines),R_ENTRY(CreateShaderModule),
        R_ENTRY(CreateRenderPass),R_ENTRY(CreateRenderPass2),R_ENTRY(CreateRenderPass2KHR),
        R_ENTRY(CmdBeginRendering),R_ENTRY(CmdBeginRenderingKHR),R_ENTRY(CmdBeginCustomResolveEXT),R_ENTRY(CmdBindPipeline),
        R_ENTRY(CmdDraw),R_ENTRY(CmdEndRendering),R_ENTRY(CmdEndRenderingKHR),
    };
#undef R_ENTRY
    for(size_t i=0;i<sizeof entries/sizeof entries[0];i++) {
        if(strcmp(name,entries[i].name)) continue;
        pthread_mutex_lock(&g_lock);
        if(!*entries[i].native) *entries[i].native=native;
        int same=*entries[i].native==native;
        pthread_mutex_unlock(&g_lock);
        if(!same) VKI("resolve-observe dispatch-bypass name=%s native_target_changed\n",name);
        return same?entries[i].wrapper:native;
    } return native;
}
#undef R_TARGET
#undef R_CALL
