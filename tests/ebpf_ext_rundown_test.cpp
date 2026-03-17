// Copyright (c) eBPF for Windows contributors
// SPDX-License-Identifier: MIT

#include <catch2/catch_all.hpp>

#include "ebpf_ext_rundown.h"

#include <thread>

TEST_CASE("init_rundown", "[rundown]")
{
    ebpf_ext_hook_rundown_t rundown = {};
    ebpf_ext_init_rundown(&rundown);

    REQUIRE(rundown.rundown_initialized == true);
    REQUIRE(rundown.rundown_occurred == false);

    // Clean up: wait for rundown so it can be torn down.
    ebpf_ext_wait_for_rundown(&rundown);
    REQUIRE(rundown.rundown_occurred == true);
}

TEST_CASE("enter_leave_rundown", "[rundown]")
{
    ebpf_ext_hook_rundown_t rundown = {};
    ebpf_ext_init_rundown(&rundown);

    // Acquire rundown protection.
    bool acquired = ebpf_ext_enter_rundown(&rundown);
    REQUIRE(acquired == true);

    // Release rundown protection.
    ebpf_ext_leave_rundown(&rundown);

    // Clean up.
    ebpf_ext_wait_for_rundown(&rundown);
    REQUIRE(rundown.rundown_occurred == true);
}

TEST_CASE("enter_after_rundown_fails", "[rundown]")
{
    ebpf_ext_hook_rundown_t rundown = {};
    ebpf_ext_init_rundown(&rundown);

    // Complete rundown.
    ebpf_ext_wait_for_rundown(&rundown);
    REQUIRE(rundown.rundown_occurred == true);

    // Attempt to acquire protection after rundown should fail.
    bool acquired = ebpf_ext_enter_rundown(&rundown);
    REQUIRE(acquired == false);
}

TEST_CASE("wait_blocks_until_leave", "[rundown]")
{
    ebpf_ext_hook_rundown_t rundown = {};
    ebpf_ext_init_rundown(&rundown);

    // Acquire rundown protection on the main thread.
    bool acquired = ebpf_ext_enter_rundown(&rundown);
    REQUIRE(acquired == true);

    bool wait_completed = false;

    // Start a thread that waits for rundown to complete.
    std::thread waiter([&]() {
        ebpf_ext_wait_for_rundown(&rundown);
        wait_completed = true;
    });

    // Give the waiter thread time to block.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(wait_completed == false);

    // Release protection — this should unblock the waiter.
    ebpf_ext_leave_rundown(&rundown);
    waiter.join();

    REQUIRE(wait_completed == true);
    REQUIRE(rundown.rundown_occurred == true);
}

TEST_CASE("multiple_enter_leave", "[rundown]")
{
    ebpf_ext_hook_rundown_t rundown = {};
    ebpf_ext_init_rundown(&rundown);

    // Acquire multiple rundown references.
    bool acquired1 = ebpf_ext_enter_rundown(&rundown);
    bool acquired2 = ebpf_ext_enter_rundown(&rundown);
    REQUIRE(acquired1 == true);
    REQUIRE(acquired2 == true);

    bool wait_completed = false;

    std::thread waiter([&]() {
        ebpf_ext_wait_for_rundown(&rundown);
        wait_completed = true;
    });

    // Release one — wait should still be blocked.
    ebpf_ext_leave_rundown(&rundown);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(wait_completed == false);

    // Release the second — wait should now complete.
    ebpf_ext_leave_rundown(&rundown);
    waiter.join();

    REQUIRE(wait_completed == true);
}
