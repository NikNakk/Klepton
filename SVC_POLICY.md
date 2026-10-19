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
