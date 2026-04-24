# Trace Logging

This library provides a common trace logging infrastructure for eBPF extension
drivers. Consumers provide their own TraceLogging provider; the library supplies
macros, dispatch functions, and registration/teardown logic.

## Design: Why switch dispatch instead of inline macros

`TraceLoggingWrite` requires keyword and level arguments to be **compile-time
constants**. A straightforward macro-only approach would inline the full
`TraceLoggingWrite` expansion (~200-300 bytes of machine code) at every call
site. For a driver with many trace points, this adds up quickly.

To keep binary size small, the public logging macros (e.g.,
`EBPF_EXT_LOG_MESSAGE`) route through helper functions in
`ebpf_ext_tracelog.c`. These functions use `switch` statements to convert
runtime enum values back into the compile-time constants that
`TraceLoggingWrite` needs. Each keyword/level combination appears exactly once
in the switch body, so the `TraceLoggingWrite` metadata is emitted once per
variant rather than once per call site.

The tradeoff is that adding a new keyword requires updating three places (see
below). This is intentional — it keeps the dispatch centralized and the binary
compact.

## Setup

### 1. Define a TraceLogging provider

In exactly one source file in your extension, define the provider with your
own name and GUID:

```c
#include "ebpf_ext_tracelog.h"

TRACELOGGING_DEFINE_PROVIDER(
    ebpf_ext_tracelog_provider,
    "MyExtensionProvider",
    // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    (0x12345678, 0xabcd, 0xef01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01));
```

### 2. Register and unregister the provider

Call `ebpf_ext_trace_initiate()` during driver/module initialization and
`ebpf_ext_trace_terminate()` during teardown:

```c
NTSTATUS DriverEntry(...)
{
    NTSTATUS status = ebpf_ext_trace_initiate();
    if (!NT_SUCCESS(status)) {
        return status;
    }
    // ...
}

void DriverUnload(...)
{
    // ...
    ebpf_ext_trace_terminate();
}
```

## Adding a new extension keyword

When a consuming extension needs a new keyword for filtering its trace events,
update these three files in this repository:

### 1. `include/ebpf_ext_tracelog.h`

Add the bitmask constant (use the next available power-of-two starting from
`0x8`):

```c
#define EBPF_EXT_TRACELOG_KEYWORD_MY_FEATURE 0x200
```

Add a corresponding entry in the `ebpf_ext_tracelog_keyword_t` enum:

```c
typedef enum _ebpf_ext_tracelog_keyword
{
    _EBPF_EXT_TRACELOG_KEYWORD_BASE,
    // ... existing entries ...
    _EBPF_EXT_TRACELOG_KEYWORD_MY_FEATURE,   // <-- add here
} ebpf_ext_tracelog_keyword_t;
```

### 2. `src/ebpf_ext_tracelog.c`

Add shorthand aliases near the top of the file:

```c
#define KEYWORD_MY_FEATURE EBPF_EXT_TRACELOG_KEYWORD_MY_FEATURE
#define CASE_MY_FEATURE    case _EBPF_EXT_TRACELOG_KEYWORD_MY_FEATURE
```

Then add a `CASE_MY_FEATURE` entry to **every** keyword switch macro (e.g.,
`EBPF_EXT_LOG_MESSAGE_KEYWORD_SWITCH`, `EBPF_EXT_LOG_NTSTATUS_API_FAILURE_KEYWORD_SWITCH`,
etc.). Follow the pattern of existing entries:

```c
CASE_MY_FEATURE:                                                         \
    _EBPF_EXT_LOG_MESSAGE(trace_level, KEYWORD_MY_FEATURE, message);     \
    break;                                                               \
```

### 3. Build and test

Rebuild and run the existing tests to verify the new keyword compiles
correctly. If a keyword is used without being added to the switch, the
`default: ASSERT(!"Invalid keyword")` branch will fire at runtime.

## Reserved keyword bits

| Bit   | Constant                                        | Purpose                  |
|-------|-------------------------------------------------|--------------------------|
| `0x1` | `EBPF_EXT_TRACELOG_KEYWORD_FUNCTION_ENTRY_EXIT` | Function entry/exit logs |
| `0x2` | `EBPF_EXT_TRACELOG_KEYWORD_BASE`                | Base/common code logs    |
| `0x4` | `EBPF_EXT_TRACELOG_KEYWORD_EXTENSION`           | Generic extension logs   |
| `0x8+`| Extension-specific (XDP, BIND, etc.)            | Per-extension filtering  |

## Available macros

### Entry/exit

- `EBPF_EXT_LOG_ENTRY()` — logs function entry at VERBOSE level.
- `EBPF_EXT_LOG_EXIT()` — logs function exit at VERBOSE level.

### Message logging

- `EBPF_EXT_LOG_MESSAGE(level, keyword, message)`
- `EBPF_EXT_LOG_MESSAGE_STRING(level, keyword, message, string_value)`
- `EBPF_EXT_LOG_MESSAGE_NTSTATUS(level, keyword, message, status)`
- `EBPF_EXT_LOG_MESSAGE_UINT32(level, keyword, message, value)`
- `EBPF_EXT_LOG_MESSAGE_UINT64(level, keyword, message, value)`
- `EBPF_EXT_LOG_MESSAGE_UINT64_UINT64(level, keyword, message, v1, v2)`
- `EBPF_EXT_LOG_MESSAGE_UINT64_UINT64_UINT64(level, keyword, message, v1, v2, v3)`
- `EBPF_EXT_LOG_MESSAGE_BOOL(level, keyword, message, value)`
- `EBPF_EXT_LOG_MESSAGE_POINTER(level, keyword, message, value)`
- `EBPF_EXT_LOG_MESSAGE_GUID_STATUS(level, keyword, message, guid, status)`

### Error logging

- `EBPF_EXT_LOG_NTSTATUS_API_FAILURE(keyword, api, status)`
- `EBPF_EXT_LOG_NTSTATUS_API_FAILURE_MESSAGE_STRING(keyword, api, status, message, value)`
- `EBPF_EXT_LOG_NTSTATUS_API_FAILURE_UINT64_UINT64(keyword, api, status, v1, v2)`

### Convenience / bail macros

- `EBPF_EXT_RETURN_RESULT(status)` — logs and returns an `ebpf_result_t`.
- `EBPF_EXT_RETURN_NTSTATUS(status)` — logs and returns an `NTSTATUS`.
- `EBPF_EXT_RETURN_POINTER(type, pointer)` — logs and returns a pointer.
- `EBPF_EXT_RETURN_BOOL(flag)` — logs and returns a bool.
- `EBPF_EXT_BAIL_ON_ERROR_RESULT(result)` — goto Exit on failure.
- `EBPF_EXT_BAIL_ON_ERROR_STATUS(status)` — goto Exit on failure.
- `EBPF_EXT_BAIL_ON_ALLOC_FAILURE_RESULT(keyword, ptr, name, result)`
- `EBPF_EXT_BAIL_ON_ALLOC_FAILURE_STATUS(keyword, ptr, name, status)`
- `EBPF_EXT_BAIL_ON_API_FAILURE_STATUS(keyword, api, status)`
