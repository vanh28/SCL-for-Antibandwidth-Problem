#include "antibandwidth_encoder.h"
#include "abp_encoder.h"

#include <iostream>
#include <assert.h>
#include <unistd.h>
#include <regex>
#include <sstream>
#include <cmath>
#include <sys/mman.h>

namespace SATABP
{

    AntibandwidthEncoder::AntibandwidthEncoder()
    {
        max_consumed_memory = (float *)mmap(nullptr, sizeof(float), PROT_READ | PROT_WRITE,
                                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    };

    AntibandwidthEncoder::~AntibandwidthEncoder()
    {
        end_time = std::chrono::high_resolution_clock::now();
        auto encode_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        std::cout << "r\n";
        std::cout << "r Final results: \n";
        std::cout << "r Max width SAT:  \t" << (max_width_SAT == std::numeric_limits<int>::min() ? "-" : std::to_string(max_width_SAT)) << "\n";
        std::cout << "r Min width UNSAT:\t" << (min_width_UNSAT == std::numeric_limits<int>::max() ? "-" : std::to_string(min_width_UNSAT)) << "\n";
        std::cout << "r Total real time: " << encode_duration << " ms\n";
        std::cout << "r Total memory consumed " << *max_consumed_memory << " MB\n";
        std::cout << "r\n";
        std::cout << "r\n";
    };

    void AntibandwidthEncoder::read_graph(std::string graph_file_name)
    {
        graph = new Graph(graph_file_name);
    };

    void AntibandwidthEncoder::encode_and_solve_abws()
    {
        switch (iterative_strategy)
        {
        case from_lb:
            std::cout << "c Iterative strategy: from LB to UB.\n";
            encode_and_solve_abw_problems_from_lb();
            break;
        case from_ub:
            std::cout << "c Iterative strategy: from UB to LB.\n";
            encode_and_solve_abw_problems_from_ub();
            break;
        case bin_search:
            std::cout << "c Iterative strategy: binary search between LB and UB.\n";
            encode_and_solve_abw_problems_bin_search();
            break;
        default:
            std::cerr << "c Unrecognized iterative strategy " << iterative_strategy << ".\n";
            return;
        }
    };

    void AntibandwidthEncoder::encode_and_solve_abw_problems_from_lb()
    {
        int w_from, w_to;
        setup_bounds(w_from, w_to);
        encode_and_solve_abw_problems(w_from, +1, w_to + 1);
    };

    void AntibandwidthEncoder::encode_and_solve_abw_problems_from_ub()
    {
        std::cerr << "e Iterative strategy UB has not yet implemented.\n";
    };

    void AntibandwidthEncoder::encode_and_solve_abw_problems_bin_search()
    {
        std::cerr << "e Iterative strategy bin has not yet implemented.\n";
    };

    std::vector<int> AntibandwidthEncoder::get_child_pids(int ppid)
    {
        std::vector<int> childPIDs;
        std::ifstream file("/proc/" + std::to_string(ppid) + "/task/" + std::to_string(ppid) + "/children");

        if (!file.is_open())
        {
            std::cerr << "Unable to open /proc/" << ppid << "/task/" << ppid << "/children" << std::endl;
            return childPIDs;
        }

        std::string line;
        if (std::getline(file, line))
        {
            std::istringstream iss(line);
            int childPID;
            while (iss >> childPID)
            {
                childPIDs.push_back(childPID);
            }
        }
        file.close();
        return childPIDs;
    }

    std::vector<int> AntibandwidthEncoder::get_descendant_pids(int ppid)
    {
        std::vector<int> descendantPIDs;

        std::vector<int> childPIDs = get_child_pids(ppid);

        for (int childPID : childPIDs)
        {
            descendantPIDs.push_back(childPID);

            std::vector<int> grandChildPIDs = get_descendant_pids(childPID);
            descendantPIDs.insert(descendantPIDs.end(), grandChildPIDs.begin(), grandChildPIDs.end());
        }

        return descendantPIDs;
    }

    size_t AntibandwidthEncoder::get_total_memory_usage(int pid)
    {
        size_t totalMemoryUsage = get_memory_usage(pid);

        std::vector<int> descendant_pids = get_descendant_pids(pid);
        for (int descendant_pid : descendant_pids)
        {
            totalMemoryUsage += get_memory_usage(descendant_pid);
        }

        // std::cout << "c [Lim] Process " << pid << " consumed total " << totalMemoryUsage / 1024.0 << " MB.\n";
        return totalMemoryUsage;
    }

    size_t AntibandwidthEncoder::get_memory_usage(int pid)
    {
        std::ifstream file("/proc/" + std::to_string(pid) + "/status");
        if (!file.is_open())
        {
            std::cerr << "Unable to open /proc/" << pid << "/status" << std::endl;
            return 0;
        }

        std::string line;
        size_t memoryUsage = 0;

        while (std::getline(file, line))
        {
            if (line.find("VmRSS:") == 0)
            { // Look for the VmRSS field
                std::istringstream iss(line);
                std::string key, value, unit;
                iss >> key >> value >> unit;     // VmRSS: value unit
                memoryUsage = std::stoul(value); // Memory usage in kilobytes (KB)
                break;
            }
        }

        file.close();
        // std::cout << "c [Lim] Process " << pid << " consumed " << memoryUsage / 1024.0 << " MB.\n";
        return memoryUsage;
    }

    /*
     *  Check if limit conditions are satified or not
     *  Return:
     *      0   if all the conditions is satified.
     *      -1  if out of memory.
     *      -2  if out of real time.
     *      -3  if out of elapsed time.
     */
    int AntibandwidthEncoder::is_limit_satified()
    {
        if (consumed_memory > memory_limit)
            return -1;

        if (consumed_real_time > real_time_limit)
            return -2;

        if (consumed_elapsed_time > elapsed_time_limit)
            return -3;

        return 0;
    }

    void AntibandwidthEncoder::create_limit_pid()
    {
        lim_pid = fork();
        if (lim_pid < 0)
        {
            std::cerr << "e [Lim] Fork Failed!\n";
            exit(-1);
        }
        else if (lim_pid == 0)
        {
            pid_t main_pid = getppid();
            int limit_state = is_limit_satified();

            while (limit_state == 0)
            {
                consumed_memory = std::round(get_total_memory_usage(main_pid) * 10 / 1024.0) / 10;
                consumed_real_time += std::round((float)sample_rate * 10 / 1000000.0) / 10;
                consumed_elapsed_time += (float)(sample_rate * (get_descendant_pids(main_pid).size() - 1)) / 1000000.0;

                if (consumed_memory > *max_consumed_memory)
                {
                    *max_consumed_memory = consumed_memory;
                    // std::cout << "[Lim] Memory consumed: " << max_consumed_memory << " MB\n";
                }

                sampler_count++;
                if (sampler_count >= report_rate)
                {
                    std::cout << "c [Lim] Sampler:\t" << "Memory: " << consumed_memory << " MB\tReal time: "
                              << consumed_real_time << "s\tElapsed time: " << consumed_elapsed_time << "s\n";
                    sampler_count = 0;
                }
                usleep(sample_rate);

                limit_state = is_limit_satified();
            }

            exit(limit_state);
        }
        else
        {
            // std::cout << "c Lim pid is forked at " << lim_pid << "\n";
        }
    }

    void AntibandwidthEncoder::create_abp_pid(int width)
    {
        // std::cout << "p PID: " << getpid() << ", PPID: " << getppid() << "\n";
        pid_t pid = fork();
        // std::cout << "q PID: " << getpid() << ", PPID: " << getppid() << "\n";

        if (pid < 0)
        {
            std::cerr << "e [w = " << width << "] Fork failed!\n";
            exit(-1);
        }
        else if (pid == 0)
        {
            std::cout << "c [w = " << width << "] Start task in PID: " << getpid() << ".\n";

            // Child process: perform the task
            int result = do_abp_pid_task(width);

            exit(result);
        }
        else
        {
            // Parent process stores the child's PID
            // std::cout << "c Child pid " << width << " - " << pid << " is tracked in PID: " << getpid() << ".\n";
            abp_pids[width] = pid;
        }
    }

    int AntibandwidthEncoder::do_abp_pid_task(int width)
    {
        // Dynamically allocate and use ABPEncoder in child process
        ABPEncoder *abp_enc = new ABPEncoder(symmetry_break_strategy, graph, width);
        int result = abp_enc->encode_and_solve_abp();

        std::cout << "c [w = " << width << "] Result: " << result << "\n";

        // Clean up dynamically allocated memory
        delete abp_enc;

        // std::cout << "c [w = " << width << "] Child " << width << " completed task." << std::endl;
        return result;
    }

    void AntibandwidthEncoder::encode_and_solve_abw_problems(int start_w, int step, int stop_w)
    {
        start_time = std::chrono::high_resolution_clock::now();
        create_limit_pid();

        int current_width = start_w;
        int number_width = stop_w - start_w;

        for (int i = 0; i < process_count && i < number_width; i += step)
        {
            create_abp_pid(current_width);
            current_width += step;
        }

        bool limit_violated = false;

        // Parent process waits until all child processes finish
        while (!abp_pids.empty())
        {
            int status;
            pid_t finished_pid = wait(&status); // Wait for any child to complete

            if (finished_pid == lim_pid)
            {
                limit_violated = true;
                std::cout << "c [Lim] End with result: " << status << "\n";
                while (!abp_pids.empty())
                {
                    kill(abp_pids.begin()->second, SIGTERM);
                    abp_pids.erase(abp_pids.begin());
                }
            }
            else if (WIFEXITED(status))
            {
                // Remove the finished child from the map
                for (auto it = abp_pids.begin(); it != abp_pids.end(); ++it)
                {
                    if (it->second == finished_pid)
                    {
                        // std::cout << "c Child pid " << it->first << " - " << it->second << " exited with status " << WEXITSTATUS(status) << "\n";

                        switch (WEXITSTATUS(status))
                        {
                        case 10:
                            if (it->first > max_width_SAT)
                            {
                                max_width_SAT = it->first;
                                std::cout << "c Max width SAT is set to " << it->first << "\n";
                            }

                            for (auto ita = abp_pids.begin(); ita != abp_pids.end(); ita++)
                            {
                                // Pid with lower width than SAT pid is also SAT.
                                if (ita->first < it->first)
                                {
                                    std::cout << "c Kill lower pid " << ita->first << ".\n";
                                    kill(ita->second, SIGTERM);
                                }
                            }
                            break;
                        case 20:
                            if (it->first < min_width_UNSAT)
                            {
                                min_width_UNSAT = it->first;
                                std::cout << "c Min width UNSAT is set to " << it->first << "\n";
                            }

                            for (auto ita = abp_pids.begin(); ita != abp_pids.end(); ita++)
                            {
                                // Pid with higher width than UNSAT pid is also UNSAT.
                                if (ita->first > it->first)
                                {
                                    std::cout << "c Kill higher pid " << ita->first << ".\n";
                                    kill(ita->second, SIGTERM);
                                }
                            }
                            break;
                        default:
                            break;
                        }

                        abp_pids.erase(it);
                        if (abp_pids.empty() && kill(lim_pid, 0) == 0)
                        {
                            kill(lim_pid, SIGTERM);
                        }
                        break;
                    }
                }
            }
            else if (WIFSIGNALED(status))
            {
                // Remove the terminated child from the map
                for (auto it = abp_pids.begin(); it != abp_pids.end(); ++it)
                {
                    if (it->second == finished_pid)
                    {
                        std::cout << "c Child pid " << it->first << " - " << it->second << " terminated by signal " << WTERMSIG(status) << "\n";
                        abp_pids.erase(it);
                        if (abp_pids.empty() && kill(lim_pid, 0) == 0)
                        {
                            kill(lim_pid, SIGTERM);
                        }
                        break;
                    }
                }
            }
            else
            {
                for (auto it = abp_pids.begin(); it != abp_pids.end(); ++it)
                {
                    if (it->second == finished_pid)
                    {
                        std::cerr << "e Child pid " << it->first << " - " << it->second << " stopped or otherwise terminated.\n";
                        abp_pids.erase(it);
                        if (abp_pids.empty() && kill(lim_pid, 0) == 0)
                        {
                            kill(lim_pid, SIGTERM);
                        }
                        break;
                    }
                }
            }

            // std::string log_remaining_child_pids = "c Remaining child pids: " + std::to_string(abp_pids.size()) + "\n";
            // for (auto it = abp_pids.begin(); it != abp_pids.end(); ++it)
            // {
            //     log_remaining_child_pids.append("c - Pid ");
            //     log_remaining_child_pids.append(std::to_string(it->first));
            //     log_remaining_child_pids.append(" - ");
            //     log_remaining_child_pids.append(std::to_string(it->second));
            //     log_remaining_child_pids.append("\n");
            // }
            // std::cout << log_remaining_child_pids;

            /*
             *   Debug get_child_pids(int) and get_memory_usage functions.
             */
            // auto saved_child_pids = get_child_pids(getpid());
            // std::string log_saved_child_pids = "c Saved child pids: " + std::to_string(saved_child_pids.size()) + "\n";
            // for (auto pid : saved_child_pids)
            // {
            //     log_saved_child_pids.append("c - Saved pid ");
            //     log_saved_child_pids.append(std::to_string(pid));
            //     log_saved_child_pids.append(": ");
            //     log_saved_child_pids.append(std::to_string(get_memory_usage(pid)));
            //     log_saved_child_pids.append(" KB.\n");
            // }
            // std::cout << log_saved_child_pids;

            if (!limit_violated){
                while (int(abp_pids.size()) < process_count && current_width < stop_w && current_width < min_width_UNSAT)
                {
                    create_abp_pid(current_width);
                    current_width += step;
                }
            }
            
        }
        std::cout << "c All children have completed their tasks or were terminated." << std::endl;
    };

    void AntibandwidthEncoder::encode_and_print_abw_problem(int w)
    {
        w = w;
        std::cerr << "e Encode and print ABP have not yet implemented.\n";
    };

    void AntibandwidthEncoder::setup_bounds(int &w_from, int &w_to)
    {
        lookup_bounds(w_from, w_to);

        if (overwrite_lb)
        {
            std::cout << "c LB " << w_from << " is overwritten with " << forced_lb << ".\n";
            w_from = forced_lb;
        }
        if (overwrite_ub)
        {
            std::cout << "c UB " << w_to << " is overwritten with " << forced_ub << ".\n";
            w_to = forced_ub;
        }
        if (w_from > w_to)
        {
            int tmp = w_from;
            w_from = w_to;
            w_to = tmp;
            std::cout << "c Flipped LB and UB to avoid LB > UB: (LB = " << w_from << ", UB = " << w_to << ").\n";
        }

        assert((w_from <= w_to) && (w_from >= 1));
    };

    void AntibandwidthEncoder::lookup_bounds(int &lb, int &ub)
    {
        auto pos = abw_LBs.find(graph->graph_name);
        if (pos != abw_LBs.end())
        {
            lb = pos->second;
            std::cout << "c LB-w = " << lb << " (LB in Sinnl - A note on computational approaches for the antibandwidth problem).\n";
        }
        else
        {
            lb = 1;
            std::cout << "c No predefined lower bound is found for " << graph->graph_name << ".\n";
            std::cout << "c LB-w = 1 (default value).\n";
        }

        pos = abw_UBs.find(graph->graph_name);
        if (pos != abw_UBs.end())
        {
            ub = pos->second;
            if (verbose)
                std::cout << "c UB-w = " << ub << " (UB in Sinnl - A note on computational approaches for the antibandwidth problem).\n";
        }
        else
        {
            ub = graph->n / 2 + 1;
            std::cout << "c No predefined upper bound is found for " << graph->graph_name << ".\n";
            std::cout << "c UB-w = " << ub << " (default value calculated as n/2+1).\n";
        }
    };

    std::unordered_map<std::string, int> AntibandwidthEncoder::abw_LBs = {
        {"A-pores_1.mtx.rnd", 6},
        {"B-ibm32.mtx.rnd", 9},
        {"C-bcspwr01.mtx.rnd", 16},
        {"D-bcsstk01.mtx.rnd", 8},
        {"E-bcspwr02.mtx.rnd", 21},
        {"F-curtis54.mtx.rnd", 12},
        {"G-will57.mtx.rnd", 12},
        {"H-impcol_b.mtx.rnd", 8},
        {"I-ash85.mtx.rnd", 19},
        {"J-nos4.mtx.rnd", 32},
        {"K-dwt__234.mtx.rnd", 46},
        {"L-bcspwr03.mtx.rnd", 39},
        {"M-bcsstk06.mtx.rnd", 28},
        {"N-bcsstk07.mtx.rnd", 28},
        {"O-impcol_d.mtx.rnd", 91},
        {"P-can__445.mtx.rnd", 78},
        {"Q-494_bus.mtx.rnd", 219},
        {"R-dwt__503.mtx.rnd", 46},
        {"S-sherman4.mtx.rnd", 256},
        {"T-dwt__592.mtx.rnd", 103},
        {"U-662_bus.mtx.rnd", 219},
        {"V-nos6.mtx.rnd", 326},
        {"W-685_bus.mtx.rnd", 136},
        {"X-can__715.mtx.rnd", 112}};

    std::unordered_map<std::string, int> AntibandwidthEncoder::abw_UBs = {
        {"A-pores_1.mtx.rnd", 8},
        {"B-ibm32.mtx.rnd", 9},
        {"C-bcspwr01.mtx.rnd", 17},
        {"D-bcsstk01.mtx.rnd", 9},
        {"E-bcspwr02.mtx.rnd", 22},
        {"F-curtis54.mtx.rnd", 13},
        {"G-will57.mtx.rnd", 14},
        {"H-impcol_b.mtx.rnd", 8},
        {"I-ash85.mtx.rnd", 27},
        {"J-nos4.mtx.rnd", 40},
        {"K-dwt__234.mtx.rnd", 58},
        {"L-bcspwr03.mtx.rnd", 39},
        {"M-bcsstk06.mtx.rnd", 72},
        {"N-bcsstk07.mtx.rnd", 72},
        {"O-impcol_d.mtx.rnd", 173},
        {"P-can__445.mtx.rnd", 120},
        {"Q-494_bus.mtx.rnd", 246},
        {"R-dwt__503.mtx.rnd", 71},
        {"S-sherman4.mtx.rnd", 272},
        {"T-dwt__592.mtx.rnd", 150},
        {"U-662_bus.mtx.rnd", 220},
        {"V-nos6.mtx.rnd", 337},
        {"W-685_bus.mtx.rnd", 136},
        {"X-can__715.mtx.rnd", 142}};
}
