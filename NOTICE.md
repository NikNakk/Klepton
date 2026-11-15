# Notices for this branch

This branch is [Klepton](https://github.com/shinyquagsire23/Klepton) by Max
Thomas (shinyquagsire23) and contributors (MIT, [LICENSE](LICENSE)), at
`ce74582`, with changes for running Linux ARM64 and Android VR applications
natively on Apple Silicon macOS. It is the Klepton used by
[threshold-runtime](https://github.com/NikNakk/threshold-runtime).

## VeRantyx

The commit "Apply VeRantyx native ARM64 compatibility prerequisites" applies
the Klepton patch set from [VeRantyx](https://github.com/Ag3497120/VeRantyx)
by Ag3497120, `native` branch at `38c02b9492c9148f6a4ca86dc607ce52aa6f7403`
(`NativeARM/klepton-patches/`), MIT. It added ELF TLS for glibc guests
(`runtime/kl_tls.*`), asynchronous pipeline creation
(`runtime/gfx/kl_vk_async.inc`) and changes to `runtime/gfx/kl_vulkan.c`,
`runtime/kl_dl.c`, `runtime/kl_image.c`, `runtime/kl_x18.*`, `runtime/klepton.h`,
`runtime/libc/kl_libc.c`, `runtime/libc/kl_shim.c`, `tools/klepton_ld.c` and the
`Makefile`. Its licence is in [LICENSE.VeRantyx](LICENSE.VeRantyx).

## Later changes

Later commits on this branch are copyright 2026 Nick Kennedy and are released
under the MIT licence, like Klepton. No game, application or captured game
content is included.
