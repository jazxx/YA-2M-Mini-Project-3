#pragma once
#include <iostream>

// Minimal test helper: CHECK(condition) counts checks and reports failures with the line number.
static int checks = 0, failures = 0;

#define CHECK(cond)                                                                 \
    do                                                                              \
    {                                                                               \
        checks++;                                                                   \
        if (!(cond))                                                                \
        {                                                                           \
            failures++;                                                             \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << ": " #cond "\n"; \
        }                                                                           \
    } while (0)
