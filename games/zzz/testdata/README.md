# ZZZ test data

This directory contains small, checked-in development evidence only. It does
not include a game binary.

- `2.5.json` and `2.6.json` are test-only manual resolver oracles.
- `3.1.json` and `3.2.json` provide the current field schemas used to generate
  the production address-free `profile.hpp` and test-only `test_profiles.hpp`.
- `patterns.json` records the generated masked-rule digest for the preserved
  3.1 input.
- `ida/<version>` contains the instruction listings used exclusively by the
  static validator. The listings retain their source-version JSON companions.

Regenerate scan rules only with an explicitly supplied, read-only 3.1
`GameAssembly.dll`; no tool in this directory loads or executes that input.
