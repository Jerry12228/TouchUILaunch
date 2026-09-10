# HDE64

Unmodified HDE64 sources from TsudaKageyu/minhook commit
`8af6b4acae5a9388fd742b56fa79ece89d96f823`, `src/hde/`.

Upstream: https://github.com/TsudaKageyu/minhook/tree/8af6b4acae5a9388fd742b56fa79ece89d96f823/src/hde

Only the instruction decoder is compiled. No MinHook hook implementation is
included. `LICENSE.txt` contains the upstream notices, including the HDE64 BSD
license; distribute this notice with the launcher/payload binaries.

The project wrapper copies at most 15 file-backed bytes into a padded local
buffer, checks decoder error/length flags, and rejects unsupported instructions.
No client machine code is executed during discovery.
