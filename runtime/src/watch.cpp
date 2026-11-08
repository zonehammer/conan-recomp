// --watch <guest address>[:<length>]: logs every write to that guest memory (physical
// views only), with the writer's guest thread, lr and call stack, or the host code
// offset for writes made by the runtime itself. For finding who clobbers an object.
#include "watch.h"
#include "kernel/kernel.h"
#include "kernel/threads.h"
#include "log.h"
#include "memory.h"
#include "os.h"

#include <atomic>
#include <cstring>
#include <map>
#include <mutex>

namespace wet::watch
{
    namespace
    {
        std::mutex s_lock;
        std::map<uintptr_t, uint32_t> s_pages;   // host page -> physical page base
        uint32_t s_physStart = 0, s_physEnd = 0, s_guest = 0;
        std::atomic<int> s_logged{0};
        std::map<uint64_t, int> s_perWriter;
        thread_local uintptr_t t_page = 0, t_host = 0;
        thread_local uint32_t t_phys = 0;

        int OnFault(uintptr_t host, uintptr_t pc)
        {
            uintptr_t page = host & ~uintptr_t(0xFFF);
            uint32_t physPage;
            {
                std::lock_guard lock(s_lock);
                auto it = s_pages.find(page);
                if (it == s_pages.end())
                    return 0;
                physPage = it->second;
            }
            uint32_t phys = physPage + uint32_t(host & 0xFFF);
            if (phys >= s_physStart && phys < s_physEnd && s_logged.load() < 3000)
            {
                t_host = host;
                t_phys = phys;
            }
            else
                t_host = 0;
            (void)pc;
            t_page = page;
            PrepareHostWrite(0xA0000000u + physPage, 0x1000);   // the GPU watches' invalidation
            os::ProtectReadWrite(reinterpret_cast<void*>(page), 0x1000);
            GuestThread* wt = CurrentThread();
            if (t_host)
            {
                // Each writer (thread + lr) is logged 16 times at most.
                uint64_t key = (uint64_t(wt ? wt->id : 0) << 32) | (wt && wt->ctx ? uint32_t(wt->ctx->lr) : uint32_t(pc));
                std::lock_guard lock(s_lock);
                if (++s_perWriter[key] > 16)
                    t_host = 0;
            }
            if (t_host)
            {
                // Log before the write with the writer; the value follows after the step.
                GuestThread* t = wt;
                Log("WATCH: write to 0x%08X (+0x%X) by %s %u lr=%08X%s host=+0x%llX", s_guest + (phys - s_physStart),
                    phys - s_physStart, t ? "guest thread" : "host thread",
                    t ? t->id : os::CurrentHostThreadId(), t && t->ctx ? uint32_t(t->ctx->lr) : 0,
                    t ? CurrentGuestStack().c_str() : "", (unsigned long long)os::ExeOffset(pc));
                s_logged++;
            }
            return 1;
        }

        void OnStep()
        {
            if (t_host)
            {
                uint32_t v;
                std::memcpy(&v, reinterpret_cast<void*>(t_host & ~uintptr_t(3)), 4);
                Log("WATCH:   now 0x%08X at +0x%X", __builtin_bswap32(v), (t_phys & ~3u) - s_physStart);
                t_host = 0;
            }
            if (t_page)
                os::ProtectReadOnly(reinterpret_cast<void*>(t_page), 0x1000);
            t_page = 0;
        }
    }

    void Start(uint32_t guest, uint32_t length)
    {
        if (guest < 0xA0000000u || !length)
        {
            Log("WATCH: only physical-memory addresses (0xA0000000 and up) can be watched");
            return;
        }
        s_guest = guest;
        s_physStart = ViewToPhysical(guest);
        s_physEnd = s_physStart + length;
        os::SetDiagnosticWatch(OnFault, OnStep);
        std::lock_guard lock(s_lock);
        for (uint32_t p = s_physStart & ~0xFFFu; p < s_physEnd; p += 0x1000)
        {
            uint8_t* views[3];
            PhysicalPageViews(p / 0x1000, views);
            for (auto* v : views)
                if (v)
                {
                    s_pages[uintptr_t(v)] = p;
                    os::ProtectReadOnly(v, 0x1000);
                }
        }
        Log("WATCH: watching 0x%08X-0x%08X (physical 0x%08X), %zu host pages", guest, guest + length,
            s_physStart, s_pages.size());
    }
}

namespace wet::watch
{
    bool Overlaps(uint32_t guest, uint32_t length)
    {
        if (!s_guest || !length)
            return false;
        uint32_t end = s_guest + (s_physEnd - s_physStart);
        return guest < end && guest + length > s_guest;
    }
}
