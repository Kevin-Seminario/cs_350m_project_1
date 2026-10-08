//
//  scheduler.cpp
//  Processor Scheduler
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 9/13/26.
//


#include <queue>
#include "scheduler.hpp"

std::queue<ProcessId_t> readyQ;

ProcessId_t core_running[CPU_COUNT];
ProcessId_t running_interim;
bool started_runs = false;

void CreateProcess(ProcessId_t pid) {
    // A new process has been created. Update the scheduler's data structures and decisions accordingly.
    SimOutput("CreateProcess(" + std::to_string(pid) + ")", 4);\
    
    if (!started_runs)
    {
        started_runs = true;
        for (uint8_t i = 1; i < CPU_COUNT; i++)
        {
            core_running[i] = InvalidProcessId();
        }
        core_running[0] = pid;
        LoadContext(pid, 0);
        RunCore(0);
    } else
    {
        for (uint8_t i = 0; i < FIRST_EFF_CORE; i++)
        {
            if (core_running[i] == InvalidProcessId())
            {
                core_running[i] = pid;
                LoadContext(pid, i);
                RunCore(i);
                return;
            }
        }
        readyQ.push(pid);
    }
}

void ExitProcess(ProcessId_t pid) {
    // Process finished running. Update the scheduler's data structures and decisions accordingly.
    for (uint8_t i = 0; i < FIRST_EFF_CORE; i++)
    {
        if (core_running[i] == pid)
        {
            if (!readyQ.empty())
            {
                running_interim = readyQ.front();
                core_running[i] = running_interim;
                readyQ.pop();
                LoadContext(running_interim, i);
                RunCore(i);
            } else
            {
                core_running[i] = InvalidProcessId();
            }
            return;
        }
    }
    ThrowException("A process that was not running is calling exit!!!");
}

void TimerInterrupt(Time_t now) {
    // You received a timer interrupt. This is where you want to execute scheduling decisions
    
    // Iterate through each core to identify which ones are running a process
    for (uint8_t i = 0; i < FIRST_EFF_CORE && !readyQ.empty(); i++)
    {
        // If the core is running a process, stop it and enqueue the process before installing a new one
        if (core_running[i] != InvalidProcessId())
        {
            SaveContext(core_running[i], i);
            readyQ.push(core_running[i]);
        }
        running_interim = readyQ.front();
        core_running[i] = running_interim;
        readyQ.pop();
        LoadContext(running_interim, i);
        RunCore(i);
    }
}

void CStateTransitionComplete(CPUId_t core_id){
    
}

void SimulationComplete(Time_t now) {
    // Add any bookkeeping or statistics that you would want to collect. Program terminates after this function returns.
    std::cout << "Run stopped at " << FormatTime(now) << " after consuming " << GetTotalEnergyConsumed()/3600000000.0 << " kWh" << std::endl;
}
