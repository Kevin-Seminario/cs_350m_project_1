//
//  scheduler.cpp
//  Processor Scheduler
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 9/13/26.
//

#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "scheduler.hpp"

double last_energy_consumed;
uint32_t time_slices_called;

ProcessId_t p_core_running[FIRST_EFF_CORE];
ProcessId_t e_core_running[CPU_COUNT - FIRST_EFF_CORE];
ProcessId_t running_interim;
bool started_runs = false;

std::queue<ProcessId_t> perfQ;
std::queue<ProcessId_t> efficQ;

const auto start_time = std::chrono::steady_clock::now();

void CreateProcess(ProcessId_t pid) {
    // A new process has been created. Update the scheduler's data structures and decisions accordingly.
    SimOutput("CreateProcess(" + std::to_string(pid) + ")", 4);
    
    if (!started_runs)
    {
        started_runs = true;

        for (uint8_t i = 1; i < FIRST_EFF_CORE; i++)
        {
            p_core_running[i] = InvalidProcessId();
        }
        for (uint8_t i = FIRST_EFF_CORE; i < CPU_COUNT; i++)
        {
            e_core_running[i - FIRST_EFF_CORE] = InvalidProcessId();
        }
        p_core_running[0] = pid;
        LoadContext(pid, 0);
        RunCore(0);
    } else
    {
        for (uint8_t i = 0; i < FIRST_EFF_CORE; i++)
        {
            if (p_core_running[i] == InvalidProcessId())
            {
                p_core_running[i] = pid;
                LoadContext(pid, i);
                RunCore(i);
                return;
            }
        }
        perfQ.push(pid);
    }
}

void ExitProcess(ProcessId_t pid) {
    // Process finished running. Update the scheduler's data structures and decisions accordingly.
    for (uint8_t i = 0; i < FIRST_EFF_CORE; i++)
    {
        if (p_core_running[i] == pid)
        {
            // print("Killing process %u in CPU %u\n", pid, i);
            if (!perfQ.empty())
            {
                running_interim = perfQ.front();
                p_core_running[i] = running_interim;
                perfQ.pop();
                LoadContext(running_interim, i);
                RunCore(i);
            } else
            {
                p_core_running[i] = InvalidProcessId();
            }
            return;
        }
    }
    for (uint8_t i = FIRST_EFF_CORE; i < CPU_COUNT; i++)
    {
        if (e_core_running[i - FIRST_EFF_CORE] == pid)
        {
            // print("Killing process %u in CPU %u\n", pid, i);
            if (!efficQ.empty())
            {
                running_interim = efficQ.front();
                e_core_running[i - FIRST_EFF_CORE] = running_interim;
                efficQ.pop();
                LoadContext(running_interim, i);
                RunCore(i);
            } else
            {
                e_core_running[i - FIRST_EFF_CORE] = InvalidProcessId();
            }
            return;
        }
    }
    ThrowException("A process that was not running is calling exit!!!");
}

void TimerInterrupt(Time_t now) {
    // You received a timer interrupt. This is where you want to execute scheduling decisions
    
    double cur_total_energy = GetTotalEnergyConsumed();
    bool need_performance = time_slices_called == 0 ||         // Initial state so assume every core uses more power than is expected and migrate everything to back of performance queue
         last_energy_consumed / time_slices_called < (cur_total_energy - last_energy_consumed) * PERF_SOFT; 
    // bool need_performance = true;
        
    // If the average amount of energy previously consumed is higher for the current time slice, assume
    // higher power consumption than hoped for from efficiency cores and shift their processes to performance queue

    // If the average amount of energy previously consumed is lower for the current time slice, assume
    // lower power consumption than hoped for from performance cores and shift their processes to efficiency queue
        
        
    for (uint8_t i = 0; i < FIRST_EFF_CORE; i++)
    {
        if (p_core_running[i] != InvalidProcessId())
        {
            // print("Saving efficiency core %u with process %u\n", i, e_core_running[i - FIRST_EFF_CORE]);
            // Shift possibly higher cost processes to performance queue
            SaveContext(p_core_running[i], i);
            if (need_performance)
            {
                perfQ.push(p_core_running[i]);
            } else
            {
                efficQ.push(p_core_running[i]);
            }
        }

        if (!perfQ.empty())
        {
            running_interim = perfQ.front();
            p_core_running[i] = running_interim;
            perfQ.pop();
            LoadContext(running_interim, i);
            RunCore(i);
        } else
        {
            p_core_running[i] = InvalidProcessId();
        }
    }
        
    for (uint8_t i = FIRST_EFF_CORE; i < CPU_COUNT; i++)
    {
        if (e_core_running[i - FIRST_EFF_CORE] != InvalidProcessId())
        {
            // print("Saving power core %u with process %u\n", i, p_core_running[i]);
            SaveContext(e_core_running[i - FIRST_EFF_CORE], i);
            if (need_performance)
            {
                perfQ.push(e_core_running[i - FIRST_EFF_CORE]);
            } else
            {
                efficQ.push(e_core_running[i - FIRST_EFF_CORE]);
            }
        }

        if (!efficQ.empty())
        {
            running_interim = efficQ.front();
            e_core_running[i - FIRST_EFF_CORE] = running_interim;
            efficQ.pop();
            LoadContext(running_interim, i);
            RunCore(i);
        } else
        {
            e_core_running[i - FIRST_EFF_CORE] = InvalidProcessId();
        }
    }

    last_energy_consumed = cur_total_energy;
    time_slices_called++;
}

void CStateTransitionComplete(CPUId_t core_id){
}

void SimulationComplete(Time_t now) {
    // Add any bookkeeping or statistics that you would want to collect. Program terminates after this function returns.
    std::cout << "Run stopped at " << FormatTime(now) << " after consuming " << GetTotalEnergyConsumed()/3600000000.0 << " kWh" << std::endl;
    
    const auto end_time = std::chrono::steady_clock::now();
    const auto elapsed_time = std::chrono::duration<double>(end_time - start_time).count();
    const double energy_delay =  elapsed_time * GetTotalEnergyConsumed();
    std::cout << "Energy Delay Product: " << energy_delay << std::endl;
}
