#pragma once
#include <cstdint>

namespace wet::watch
{
    // Logs every write to [guest, guest + length) (physical memory only).
    void Start(uint32_t guest, uint32_t length);
}

namespace wet::watch
{
    // True when --watch is on and [guest, guest + length) touches the watched bytes.
    bool Overlaps(uint32_t guest, uint32_t length);
}
