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

## Strict graphics boundary

`KL_VK_STRICT=1` is opt-in and latched on first use. The legacy path remains
the default. In strict mode:

- Graphics/shader/render-pass/property commands go directly to the selected
  Vulkan dispatch implementation. Klepton's repair, emulation and async-pipeline
  wrappers are bypassed. Missing functions return NULL; no success stub is used.
- Instance/device requests preserve API version, layers, feature structures,
  pNext and requested extensions. Unsupported requests reach the driver and its
  error is returned. No feature clamp/retry, GPU/null-handle substitution or
  automatic Metal-object extension addition occurs.
- The implemented synthetic desktop WSI remains adapted. Its instance extension
  names are removed from the request passed to the native driver; advertised
  Vulkan portability enumeration/subset requirements are appended when needed.
  This intentionally remains a synthetic desktop swapchain, not native desktop
  presentation or a claim of complete Vulkan loader/WSI conformance.
- Extension lists are allocated to request size rather than capped at 64 names.
  Instance enumeration errors and layer errors propagate. Device bookkeeping is
  released on destroy; strict bookkeeping overflow destroys/refuses the device.

Owned fake-driver request-preservation checks (no GPU):

```sh
sh tools/check_vulkan_strict.sh
```

Real driver initialization, feature query, device creation, missing function,
unsupported-feature refusal and complete extension/driver census (no game or XR):

```sh
KL_VULKAN_LIBRARY=/opt/homebrew/lib/libvulkan.1.dylib \
VK_DRIVER_FILES=/opt/homebrew/Cellar/molten-vk/1.4.2/etc/vulkan/icd.d/MoltenVK_icd.json \
sh tools/check_vulkan_strict_real.sh

KL_VULKAN_LIBRARY=/opt/homebrew/lib/libvulkan.1.dylib \
VK_DRIVER_FILES=/private/tmp/alyx-mesa-build/src/kosmickrisp/vulkan/kosmickrisp_mesa_devenv_icd.aarch64.json \
sh tools/check_vulkan_strict_real.sh
```

20 October: both pass on Apple M5 outside the filesystem sandbox (sandboxed
Metal access was unavailable). MoltenVK driverID14/info1.4.2, KosmicKrisp
driverID28/Mesa27.0.0-devel git0913fce718. Each creates a device, returns
VK_ERROR_FEATURE_NOT_PRESENT for geometryShader and preserves the requested
true flag. Both query base fragmentDensityMap=false. This does not resolve
KosmicKrisp's missing Metal-object interoperability with Monado and is not an
Alyx run or a benchmark.

## Custom resolve diagnosis (21 October 2026)

`KL_VK_CUSTOM_RESOLVE_TRACE=1` independently enables bounded metadata observation
in the strict path. It records original image/view associations, render-pass
attachments/subpasses, dynamic rendering attachments, custom pipeline formats,
input attachment indices and original shader metadata (no shader byte dump).
Actual `vkCmdBeginCustomResolveEXT` calls, pipeline binds and direct draws on the
same recording thread are recorded. Direct-draw association is diagnostic only:
secondary buffers, indirect draws and cross-thread handovers are not replayed.
Budgets are separate for custom pipelines/rendering and ordinary work, so an
intro cannot consume every custom event slot. Budget exhaustion is explicit.

No driver feature, pipeline, attachment, shader or command is changed. Native
errors/partial pipeline outputs pass through; missing commands remain NULL;
a changed native dispatch target bypasses the observer. Opting out retains the
native pointer. Trace output contains transient Vulkan handles and stays local.
The existing FDM observer can be enabled independently or simultaneously.

Run the owned no-GPU forwarding/metadata/budget tests with:

```sh
sh tools/check_vulkan_resolve.sh
```

This is a diagnostic capture, not a performance run or a claim that Alyx's
rendering failure is fixed. The separate application launcher retains the
working custom-resolve-disabled path as the normal KosmicKrisp candidate.
