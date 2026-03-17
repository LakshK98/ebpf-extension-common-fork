# eBPF Extension Common

Shared static library providing common functionality for eBPF extension drivers on Windows. This library is consumed by both [ebpf-for-windows](https://github.com/microsoft/ebpf-for-windows) and [ntosebpfext](https://github.com/microsoft/ntosebpfext).

Tracked by [microsoft/ebpf-for-windows#4996](https://github.com/microsoft/ebpf-for-windows/issues/4996).

> **🚧 Active Development** — This repository is being developed according to the [Extension Deduplication Plan](https://github.com/microsoft/ebpf-for-windows/blob/main/docs/ExtensionDeduplicationPlan.md). Rundown protection is the first module extracted; additional common functions (hook providers, program info providers, etc.) will be migrated incrementally as outlined in the plan.

## Overview

This repository provides kernel-mode and user-mode static libraries that encapsulate common patterns used across eBPF extension drivers. The same source code compiles for both modes — kernel-mode builds against WDK headers, while user-mode builds use [usersim](https://github.com/microsoft/usersim) to provide kernel API stubs.

### Rundown Protection

The first module extracted is **rundown protection**, which wraps the Windows `EX_RUNDOWN_REF` APIs used by hook providers and clients to safely coordinate teardown.

| Function | Description |
|---|---|
| `ebpf_ext_init_rundown` | Initialize the rundown protection state |
| `ebpf_ext_wait_for_rundown` | Block until all acquired references are released |
| `ebpf_ext_enter_rundown` | Acquire rundown protection (returns `false` if rundown already occurred) |
| `ebpf_ext_leave_rundown` | Release previously acquired rundown protection |

## Prerequisites

- **Visual Studio 2022** (v143 toolset)
- **CMake** (for Catch2 build file generation)

## Building

### 1. Clone with submodules

```bash
git clone --recursive https://github.com/microsoft/ebpf-extension-common.git
cd ebpf-extension-common
```

### 2. Generate Catch2 build files

```bash
cmake -G "Visual Studio 17 2022" -S external\Catch2 -B external\Catch2\build -DBUILD_TESTING=OFF
```

### 3. Restore NuGet packages

```bash
nuget restore packages.config -PackagesDirectory packages
```

### 4. Build

Open a **Developer Command Prompt for VS 2022** and run:

```bash
msbuild ebpf_extension_common.sln /p:Configuration=Debug /p:Platform=x64
```

This builds all projects:
- `ebpf_extension_common_km.lib` — kernel-mode static library
- `ebpf_extension_common_um.lib` — user-mode static library
- `ebpf_ext_rundown_test.exe` — unit tests

## Running Tests

After building, run the Catch2 unit tests:

```bash
.\x64\Debug\ebpf_ext_rundown_test.exe
```
