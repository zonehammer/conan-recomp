// --watch <guest address>[:<length>] (repeatable): logs every write to that guest
// memory with the writer's guest thread, lr and call stack, or the host code offset
// for writes made by the runtime itself. Physical (0xA0000000+) and ordinary
// memory both work. For finding who clobbers something.
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
#include <vector>

namespace wet::watch
{
    namespace
    {
        struct Range { uint32_t guest, key, length; bool phys; };   // key: physical address or guest address
        struct Page { uint32_t keyBase; bool phys; };
        std::mutex s_lock;
        std::vector<Range> s_ranges;
        std::map<uintptr_t, Page> s_pages;      // host page -> page key
        std::map<uint64_t, int> s_perWriter;
        std::atomic<int> s_logged{0};
        thread_local uintptr_t t_page = 0, t_host = 0;
        thread_local uint32_t t_guest = 0;

        const Range* Find(uint32_t key, bool phys)
        {
            for (auto& r : s_ranges)
                if (r.phys == phys && key >= r.key && key < r.key + r.length)
                    return &r;
            return nullptr;
        }

        int OnFault(uintptr_t host, uintptr_t pc)
        {
            uintptr_t page = host & ~uintptr_t(0xFFF);
            Page pg;
            const Range* r;
            {
                std::lock_guard lock(s_lock);
                auto it = s_pages.find(page);
                if (it == s_pages.end())
                    return 0;
                pg = it->second;
                r = Find(pg.keyBase + uint32_t(host & 0xFFF), pg.phys);
            }
            t_page = page;
            t_host = 0;
            GuestThread* wt = CurrentThread();
            if (r && s_logged.load() < 3000)
            {
                uint64_t key = (uint64_t(wt ? wt->id : 0) << 32) | (wt && wt->ctx ? uint32_t(wt->ctx->lr) : uint32_t(pc));
                std::lock_guard lock(s_lock);
                if (++s_perWriter[key] <= 16)
                {
                    t_host = host;
                    t_guest = r->guest + (pg.keyBase + uint32_t(host & 0xFFF) - r->key);
                }
            }
            if (pg.phys)
                PrepareHostWrite(0xA0000000u + pg.keyBase, 0x1000);   // the GPU watches' invalidation
            os::ProtectReadWrite(reinterpret_cast<void*>(page), 0x1000);
            if (t_host)
            {
                Log("WATCH: write to 0x%08X by %s %u lr=%08X%s host=+0x%llX", t_guest,
                    wt ? "guest thread" : "host thread", wt ? wt->id : os::CurrentHostThreadId(),
                    wt && wt->ctx ? uint32_t(wt->ctx->lr) : 0, wt ? CurrentGuestStack().c_str() : "",
                    (unsigned long long)os::ExeOffset(pc));
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
                Log("WATCH:   now 0x%08X at 0x%08X", __builtin_bswap32(v), t_guest & ~3u);
                t_host = 0;
            }
            if (t_page)
                os::ProtectReadOnly(reinterpret_cast<void*>(t_page), 0x1000);
            t_page = 0;
        }
    }

    void Start(uint32_t guest, uint32_t length)
    {
        if (!length)
            return;
        bool phys = guest >= 0xA0000000u;
        Range r{ guest, phys ? ViewToPhysical(guest) : guest, length, phys };
        os::SetDiagnosticWatch(OnFault, OnStep);
        std::lock_guard lock(s_lock);
        s_ranges.push_back(r);
        size_t before = s_pages.size();
        for (uint32_t p = r.key & ~0xFFFu; p < r.key + length; p += 0x1000)
        {
            if (phys)
            {
                uint8_t* views[3];
                PhysicalPageViews(p / 0x1000, views);
                for (auto* v : views)
                    if (v)
                    {
                        s_pages[uintptr_t(v)] = { p, true };
                        os::ProtectReadOnly(v, 0x1000);
                    }
            }
            else
            {
                uint8_t* v = HostPtr(p);
                s_pages[uintptr_t(v)] = { p, false };
                if (!os::ProtectReadOnly(v, 0x1000))
                {
                    // Not allocated yet: keep trying until the game maps it.
                    Log("WATCH: 0x%08X is not mapped yet; watching from when it is", p);
                    os::StartThread([](void* arg) {
                        auto* page = static_cast<uint8_t*>(arg);
                        while (!os::ProtectReadOnly(page, 0x1000))
                            os::SleepNs(2000000);
                        Log("WATCH: page 0x%08X mapped, now watched", uint32_t(page - g_mem.base));
                    }, v, 64 * 1024);
                }
            }
        }
        Log("WATCH: watching 0x%08X-0x%08X%s, %zu host pages", guest, guest + length, phys ? " (physical)" : "",
            s_pages.size() - before);
    }

    bool Overlaps(uint32_t guest, uint32_t length)
    {
        std::lock_guard lock(s_lock);
        for (auto& r : s_ranges)
            if (length && guest < r.guest + r.length && guest + length > r.guest)
                return true;
        return false;
    }
}
