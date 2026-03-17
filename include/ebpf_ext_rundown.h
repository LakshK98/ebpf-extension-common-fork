// Copyright (c) eBPF for Windows contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "framework.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Rundown protection state for hook providers and clients.
     */
    typedef struct _ebpf_ext_hook_rundown
    {
        EX_RUNDOWN_REF protection; ///< Rundown protection reference.
        bool rundown_occurred;     ///< Whether rundown has completed.
        bool rundown_initialized;  ///< Whether rundown has been initialized.
    } ebpf_ext_hook_rundown_t;

    /**
     * @brief Initialize the rundown state.
     *
     * @param[in,out] rundown Pointer to the rundown object to initialize.
     */
    void
    ebpf_ext_init_rundown(_Inout_ ebpf_ext_hook_rundown_t* rundown);

    /**
     * @brief Block execution of the thread until all invocations are completed.
     *
     * @param[in,out] rundown Rundown object to wait for.
     */
    void
    ebpf_ext_wait_for_rundown(_Inout_ ebpf_ext_hook_rundown_t* rundown);

    /**
     * @brief Acquire rundown protection. Must be paired with ebpf_ext_leave_rundown.
     *
     * @param[in,out] rundown Rundown object to acquire protection on.
     *
     * @retval true Rundown protection was acquired successfully.
     * @retval false Rundown has already occurred; protection was not acquired.
     */
    _Must_inspect_result_ bool
    ebpf_ext_enter_rundown(_Inout_ ebpf_ext_hook_rundown_t* rundown);

    /**
     * @brief Release rundown protection previously acquired by ebpf_ext_enter_rundown.
     *
     * @param[in,out] rundown Rundown object to release protection on.
     */
    void
    ebpf_ext_leave_rundown(_Inout_ ebpf_ext_hook_rundown_t* rundown);

#ifdef __cplusplus
}
#endif
