# Native Vulkan loader selection and foveation tracing

This local experimental branch is Klepton `ce74582` plus VeRantyx native
`38c02b9492c9148f6a4ca86dc607ce52aa6f7403` prerequisites (applied separately).
No game is included or needed for the boundary test.

Existing bundled MoltenVK remains the default. Setting `KL_VULKAN_LIBRARY`
loads exactly that dylib, with no fallback. Use the standard Vulkan loader and
one `VK_DRIVER_FILES` ICD, and point xrizer's `XRIZER_VULKAN_LIBRARY` at the
same loader. Do not hand raw driver dispatchable handles to another library.

```sh
KL_VULKAN_LIBRARY=/opt/homebrew/lib/libvulkan.1.dylib \
VK_DRIVER_FILES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json \
KL_VK_FOVEATION_TRACE=1 sh tools/check_vulkan_backend.sh

KL_VULKAN_LIBRARY=/opt/homebrew/lib/libvulkan.1.dylib \
VK_DRIVER_FILES=/path/to/mesa-build/src/kosmickrisp/vulkan/kosmickrisp_mesa_devenv_icd.aarch64.json \
KL_VK_FOVEATION_TRACE=1 sh tools/check_vulkan_backend.sh
```

The new trace logs resolved function names, device extension enumeration,
requested and forwarded device extensions, pNext sTypes, FDM/FDM2/FSR feature
values, render-pass/dynamic-rendering FDM attachments, image geometry/usage,
and original SPIR-V capability and BuiltIn decorations. It observes before and
after the existing feature clamp, and adds no extension or feature support.
Tracing itself does not modify caller data or driver answers. Results from
this smoke fixture are not observations of Alyx.

This is initial instrumentation, not a complete Vulkan capture layer. Offset
payloads, buffer/descriptor tracking, map content capture, copy2 aliases,
secondary command buffers and a reproduction require follow-up when native
Alyx is available. Both tested ICDs return FDM=false.

Existing compatibility code still changes requested extensions/features and
may return success stubs for missing functions. Its graphics-pipeline async
path also changes scheduling. These behaviours need auditing or an explicit
strict mode before a game trace can establish driver requirements reliably;
the trace does not endorse them. In particular, compare pre-clamp requests
with driver answers and forwarded requests. Disable async compilation when
measuring synchronous compilation cost. No guest game code was run here.
