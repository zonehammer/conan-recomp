#pragma once
#include <cstdint>

namespace wet::watch
{
    // Logs every write to [guest, guest + length) (physical memory only).
    void Start(uint32_t guest, uint32_t length);
}
