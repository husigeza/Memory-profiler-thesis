#ifndef MEMORY_PROFILER_H_INCLUDED
#define MEMORY_PROFILER_H_INCLUDED

#include <map>
#include <bfd.h>

#include "Memory_Profiler_process.h"

//using namespace std;

class Memory_Profiler {

        map<pid_t const,Process_handler> Processes;
        string fifo_path;
        int mem_prof_fifo;

    public:
        Memory_Profiler();
        Memory_Profiler(string fifo_path);
        ~Memory_Profiler();

        bool Add_Process_to_list(const pid_t PID);

        void Add_process_to_profiling(const pid_t PID);
        void Add_all_process_to_profiling();

        map<const pid_t,Process_handler>& Get_profiled_processes_list();
        map<const pid_t,Process_handler>& Get_all_processes_list();

        void Set_process_alive_flag(const pid_t PID, bool value);
        bool Get_process_alive_flag(const pid_t PID);

        bool Get_process_shared_memory_initilized_flag(const pid_t PID);

        void Remove_process_from_profiling(const pid_t PID);
        void Remove_all_process_from_profiling();

        void Start_stop_profiling(const pid_t PID);
        void Start_stop_profiling_all_processes();

        bool Remap_process_shared_memory(const pid_t PID);
        void Remap_all_process_shared_memory();

        void Read_FIFO();

        void Analyze_process(const pid_t PID);
        void Analyze_all_process();

        void Print_all_processes()const;
        void Print_alive_processes();
        void Print_profiled_processes();
        void Print_profiled_process_shared_memory(const pid_t PID);
        void Print_all_processes_shared_memory();
        void Print_process_symbol_table(pid_t PID);
};

#endif // MEMORY_PROFILER_H_INCLUDED