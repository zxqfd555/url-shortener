#include "util.h"

uint32_t GetCurrentTimestamp() {
    std::time_t result = std::time(nullptr);
    return (uint32_t)(result);
}

