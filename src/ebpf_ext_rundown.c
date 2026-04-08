// Copyright (c) eBPF for Windows contributors
// SPDX-License-Identifier: MIT

#include "ebpf_ext_rundown.h"
#include "ebpf_ext_tracelog.h"

void
ebpf_ext_init_rundown(_Inout_ ebpf_ext_hook_rundown_t* rundown)
{
    EBPF_EXT_LOG_ENTRY();

    ASSERT(rundown->rundown_initialized == false);

    ExInitializeRundownProtection(&rundown->protection);
    rundown->rundown_occurred = false;
    rundown->rundown_initialized = true;

    EBPF_EXT_LOG_EXIT();
}

void
ebpf_ext_wait_for_rundown(_Inout_ ebpf_ext_hook_rundown_t* rundown)
{
    EBPF_EXT_LOG_ENTRY();

    ASSERT(rundown->rundown_initialized == true);

    ExWaitForRundownProtectionRelease(&rundown->protection);
    rundown->rundown_occurred = true;

    EBPF_EXT_LOG_EXIT();
}

_Must_inspect_result_ bool
ebpf_ext_enter_rundown(_Inout_ ebpf_ext_hook_rundown_t* rundown)
{
    ASSERT(rundown->rundown_initialized == true);
    return ExAcquireRundownProtection(&rundown->protection);
}

void
ebpf_ext_leave_rundown(_Inout_ ebpf_ext_hook_rundown_t* rundown)
{
    ASSERT(rundown->rundown_initialized == true);
    ExReleaseRundownProtection(&rundown->protection);
}
