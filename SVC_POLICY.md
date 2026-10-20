# Raw syscall preflight policy

`KL_SVC_POLICY=reject` conservatively refuses AArch64 SVC instruction candidates
in file-backed executable ELF LOAD segments **before guest mapping, relocation,
ABI rewrites or constructors**. It recognizes SVC with any immediate, not just
SVC #0, and does not use the six-word x8 heuristic. Bounds of inspected program
headers and executable bytes are checked. Invalid policy values fail explicitly.
Pretranslated Mach-O loading is also refused under this policy, preventing that
path from bypassing the ELF scan. Default/unset or `legacy` retains existing
behaviour, including its heuristic bare-SVC NOP pass.

This is an execution guard, not syscall translation. A conservative executable
segment scan can reject data which happens to encode an SVC instruction. It
cannot prove which candidates execute, and does not make guest TLS/x18/CTR
instruction rewriting disappear. Keep its setting stable across loading; unlike
strict Vulkan selection, this experimental policy is read per load.

Owned ELF fixtures in VeRantyx's `check-svc-guard` test SVC #0, SVC with nonzero
immediate, malformed program headers/executable segments, a truncated ELF,
unknown policy and refusal of pretranslated images. They execute no ELF code.

Before any actual game run, settle the allowed private-memory ABI translation
boundary, audit the actual syscall sites and implement explicit translation or
refusal of operations required by those sites. Broadly NOPing candidates is not
a correct general Linux syscall strategy. This guard is deliberately unable to
run images that contain candidates, even if a particular path never reaches one.

## Section-aware refusal and fail-closed execution

The user authorized required private-memory ABI translation on 20 October 2026,
with installed game binaries unchanged and game-specific logic patches disabled.

`reject-code` validates section headers and scans SHF_EXECINSTR PROGBITS sections
which must map consistently into executable LOAD segments. It requires usable
section metadata and refuses any code SVC. This avoids a confirmed false positive
in Debian libstdc++'s .gcc_except_table, which is data in an executable LOAD.
`reject` retains its broader segment scan. Owned fixtures cover malformed and
inconsistently mapped sections, sectionless refusal and an SVC-shaped data word
which passes code preflight but reaches the deliberate missing-dynamic-table
failure of that non-runnable fixture.

`trap-code` uses the same validated section boundary and translates each SVC
in code chunks into a reserved `BRK #0x4b4c` before execution. It never executes
the original SVC, returns success or NOPs it. All SVC immediates are handled.
The image census distinguishes total and trapped counts. The fault reporter
names this marker and prints image/offset, Linux x8 and Darwin x16 before the
normal fatal SIGTRAP termination. There is no recovery or syscall emulation.
This is an explicit unsupported-operation stop; it is NOT working raw-syscall
compatibility. It may also stop on a debug-break site. No assumption that every
un-numbered SVC is a harmless debug break is used.

Owned native tests verify the encoding, preservation of unrelated instructions
and fatal SIGTRAP behaviour. Actual libvideo relocation census reports
1703 SVC sites, all trapped. No such trap was reached in its constructor probe
or the initial headless engine startup; video playback remains unverified.

`KL_LOAD_NO_INIT=1` is a separate relocation diagnostic: recursive loading skips
constructors and JNI entry points, while retaining normal binding/rebinding.
It must not be used to run a game. The constrained host explicitly forces it off
when --run-constrained authorizes execution.
