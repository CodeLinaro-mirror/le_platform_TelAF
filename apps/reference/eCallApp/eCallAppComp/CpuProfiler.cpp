/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

// CpuProfiler.cpp
// Previously EcallMgrCpuProfiling.cpp — class renamed to CpuProfiler.

#include "CpuProfiler.hpp"

#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sched.h>

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <map>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstdarg>
#include <vector>
#include <string>
#include <thread>
#include <atomic>

extern "C"
{
#include "legato.h"
}

namespace ecall
{

// =============================================================================
// File-local helpers
// =============================================================================

static bool ReadFileToString(const std::string& path, std::string& out)
{
    int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;

    out.clear();
    char    buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        out.append(buf, buf + n);

    close(fd);
    return !out.empty();
}

static bool IsNumeric(const char* name)
{
    if (!name || !*name) return false;
    for (const char* p = name; *p; ++p)
        if (*p < '0' || *p > '9') return false;
    return true;
}

static std::string ReadProcName(int pid)
{
    std::string s;
    if (ReadFileToString("/proc/" + std::to_string(pid) + "/comm", s))
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();
    return s;
}

static int GetNice(int tid)
{
    errno     = 0;
    const int v = getpriority(PRIO_PROCESS, tid);
    return (errno != 0) ? 0 : v;
}

static bool ParseStatLine(const std::string& line,
                          std::string&       comm,
                          char&              state,
                          long long&         utime,
                          long long&         stime,
                          int&               processor)
{
    const size_t l = line.find('(');
    const size_t r = line.rfind(')');
    if (l == std::string::npos || r == std::string::npos || r <= l) return false;

    comm = line.substr(l + 1, r - l - 1);

    std::istringstream iss(line.substr(r + 2));
    std::string        token;
    if (!(iss >> token) || token.empty()) return false;
    state = token[0];

    int field = 3;
    utime = stime = 0;
    processor     = -1;

    while (iss >> token)
    {
        ++field;
        if      (field == 14) utime     = atoll(token.c_str());
        else if (field == 15) stime     = atoll(token.c_str());
        else if (field == 39) processor = atoi(token.c_str());
    }
    return true;
}

static bool ParseSchedstat(const std::string&  content,
                           unsigned long long& runtimeNs,
                           unsigned long long& runqueueNs,
                           unsigned long long& switches)
{
    std::istringstream iss(content);
    return static_cast<bool>(iss >> runtimeNs >> runqueueNs >> switches);
}

static int MapProcessorToCoreNumber(int processorId)
{
    static std::map<int, int> procIdToCoreNum;
    static int                nextCoreNum = 0;
    if (processorId < 0) return -1;
    auto it = procIdToCoreNum.find(processorId);
    if (it != procIdToCoreNum.end()) return it->second;
    return procIdToCoreNum[processorId] = nextCoreNum++;
}

static double NowMonoSec()
{
    struct timespec ts{0, 0};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// =============================================================================
// Internal structures
// =============================================================================

struct TaskKey
{
    int pid{0};
    int tid{0};
    bool operator<(const TaskKey& o) const
    {
        return (pid < o.pid) || (pid == o.pid && tid < o.tid);
    }
};

struct TaskSample
{
    long long          utime{0};
    long long          stime{0};
    unsigned long long runtimeNs{0};
    unsigned long long runqueueNs{0};
    unsigned long long switches{0};
    int                processor{-1};
    int                policy{SCHED_OTHER};
    int                prio{0};
    int                nice{0};
    std::string        comm;
    std::string        procName;
};

struct SampleSet
{
    std::map<TaskKey, TaskSample> data;
    double                        monoSec{0.0};
};

// =============================================================================
// CpuProfiler
// =============================================================================

CpuProfiler::CpuProfiler()  = default;

CpuProfiler::~CpuProfiler()
{
    if (running_.load(std::memory_order_acquire))
        running_.store(false, std::memory_order_release);
    if (worker_.joinable())
        worker_.join();
}

CpuProfiler& CpuProfiler::GetInstance()
{
    static CpuProfiler inst;
    return inst;
}

void CpuProfiler::Start(uint32_t periodMs, std::size_t topN, bool includeThreads)
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true))
    {
        LE_INFO("CpuProfiler: already running");
        return;
    }

    LE_INFO("CpuProfiler: starting period=%u ms topN=%zu threads=%d",
            periodMs, topN, includeThreads ? 1 : 0);

    if (worker_.joinable()) worker_.join();

    worker_ = std::thread([this, periodMs, topN, includeThreads]
    {
        RunLoop(periodMs, topN, includeThreads);
    });
}

void CpuProfiler::Stop()
{
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) return;
    if (worker_.joinable()) worker_.join();
    LE_INFO("CpuProfiler: stopped");
}

void CpuProfiler::RunLoop(uint32_t periodMs, std::size_t topN, bool includeThreads)
{
    const long clkTck = sysconf(_SC_CLK_TCK);
    const int  ncpus  = std::max(1, static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN)));

    int       updateCount = 0;
    SampleSet prev{};

    auto printLine = [](const char* fmt, ...)
    {
        char    buf[512];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        LE_INFO("%s", buf);
    };

    while (running_.load(std::memory_order_acquire))
    {
        SampleSet cur{};
        cur.monoSec = NowMonoSec();

        DIR* proc = opendir("/proc");
        if (proc)
        {
            dirent* de;
            while ((de = readdir(proc)) != nullptr)
            {
                if (!IsNumeric(de->d_name)) continue;
                const int pid = atoi(de->d_name);

                std::vector<int> tids;
                if (includeThreads)
                {
                    const std::string taskDir =
                        "/proc/" + std::to_string(pid) + "/task";
                    DIR* tdir = opendir(taskDir.c_str());
                    if (tdir)
                    {
                        dirent* te;
                        while ((te = readdir(tdir)) != nullptr)
                            if (IsNumeric(te->d_name))
                                tids.push_back(atoi(te->d_name));
                        closedir(tdir);
                    }
                }
                if (tids.empty()) tids.push_back(pid);

                const std::string procName = ReadProcName(pid);

                for (int tid : tids)
                {
                    TaskKey    key{pid, tid};
                    TaskSample ts{};

                    std::string statStr;
                    if (!ReadFileToString(
                            "/proc/" + std::to_string(pid) +
                            "/task/" + std::to_string(tid) + "/stat",
                            statStr))
                        continue;

                    std::string comm;
                    char        st = '?';
                    if (!ParseStatLine(statStr, comm, st,
                                       ts.utime, ts.stime, ts.processor))
                        continue;

                    std::string schedStatContent;
                    if (ReadFileToString(
                            "/proc/" + std::to_string(pid) +
                            "/task/" + std::to_string(tid) + "/schedstat",
                            schedStatContent))
                        ParseSchedstat(schedStatContent,
                                       ts.runtimeNs, ts.runqueueNs, ts.switches);

                    ts.policy = sched_getscheduler(tid);
                    struct sched_param sp{0};
                    sched_getparam(tid, &sp);
                    ts.prio     = sp.sched_priority;
                    ts.nice     = GetNice(tid);
                    ts.comm     = comm;
                    ts.procName = procName.empty() ? comm : procName;

                    cur.data.emplace(key, std::move(ts));
                }
            }
            closedir(proc);
        }

        if (!prev.data.empty())
        {
            struct Row
            {
                int         pid, tid;
                std::string name;
                double      runSec, waitSec, periodSec, cpuPct;
                int         coreNum, pol, pri, nice;
            };

            std::vector<Row> rows;
            rows.reserve(cur.data.size());

            for (const auto& it : cur.data)
            {
                const auto& key = it.first;
                const auto& now = it.second;
                auto        pit = prev.data.find(key);
                if (pit == prev.data.end()) continue;
                const auto& old = pit->second;

                const double periodSec =
                    std::max(1e-6, cur.monoSec - prev.monoSec);
                const double runSec =
                    static_cast<double>(now.runtimeNs > old.runtimeNs
                                            ? now.runtimeNs - old.runtimeNs
                                            : 0ull) / 1e9;
                const double waitSec =
                    static_cast<double>(now.runqueueNs > old.runqueueNs
                                            ? now.runqueueNs - old.runqueueNs
                                            : 0ull) / 1e9;

                const long long dJiff =
                    (now.utime - old.utime) + (now.stime - old.stime);
                double cpuPct =
                    ((static_cast<double>(dJiff) /
                      static_cast<double>(clkTck)) / periodSec) * 100.0;
                if (cpuPct < 0.0) cpuPct = 0.0;

                rows.push_back(Row{
                    key.pid, key.tid,
                    now.procName + "[" + now.comm + "]",
                    runSec, waitSec, periodSec, cpuPct,
                    MapProcessorToCoreNumber(now.processor),
                    now.policy, now.prio, now.nice});
            }

            std::sort(rows.begin(), rows.end(),
                      [](const Row& a, const Row& b)
                      {
                          if (a.cpuPct  != b.cpuPct)  return a.cpuPct  > b.cpuPct;
                          if (a.runSec  != b.runSec)  return a.runSec  > b.runSec;
                          return a.waitSec > b.waitSec;
                      });

            double totalCpu = 0.0;
            for (const auto& r : rows) totalCpu += r.cpuPct;

            ++updateCount;
            const std::time_t tnow = std::time(nullptr);
            char              tbuf[128]{};
            std::tm           tmres{};
            localtime_r(&tnow, &tmres);
            std::strftime(tbuf, sizeof(tbuf), "%a %b %d %H:%M:%S %Y", &tmres);

            const int selfPid = getpid();
            printLine("cpuStatistic[%d][NOTICE] TOPC Update #%d    %s",
                      selfPid, updateCount, tbuf);
            printLine("cpuStatistic[%d][NOTICE] TOPC "
                      "==========================================================================",
                      selfPid);
            printLine("cpuStatistic[%d][NOTICE] TOPC   PID Process[Thread]"
                      "                  Run       Wait    Period   CPU @ Core Pol Pri Nice",
                      selfPid);
            printLine("cpuStatistic[%d][NOTICE] TOPC "
                      "==========================================================================",
                      selfPid);

            const std::size_t count = std::min(topN, rows.size());
            for (std::size_t i = 0; i < count; ++i)
            {
                const auto& r = rows[i];
                printLine(
                    "cpuStatistic[%d][NOTICE] TOPC %5d %-30s"
                    " %8.3f %8.3f %8.3f %7.3f%% %3d %2d %3d %4d",
                    selfPid, r.pid, r.name.c_str(),
                    r.runSec, r.waitSec, r.periodSec, r.cpuPct,
                    std::max(r.coreNum, 0), r.pol, r.pri, r.nice);
            }

            printLine("cpuStatistic[%d][NOTICE] TOPC"
                      "                                          Total: %7.3f%%   %d",
                      selfPid, totalCpu, ncpus);
        }

        prev = std::move(cur);

        // Sleep in small chunks so Stop() can terminate faster.
        uint32_t left = periodMs;
        while (left > 0 && running_.load(std::memory_order_relaxed))
        {
            const uint32_t step = std::min<uint32_t>(left, 50);
            std::this_thread::sleep_for(std::chrono::milliseconds(step));
            left -= step;
        }
    }
}

} // namespace ecall
