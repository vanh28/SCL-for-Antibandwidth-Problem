#ifndef ABW_ENCODER_H
#define ABW_ENCODER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <limits>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <chrono>
#include <fstream>

#include "encoder.h"

namespace SATABP
{

	enum EncoderStrategy
	{
		duplex,
		reduced,
		seq,
		product,
		ladder,
	};

	enum IterativeStrategy
	{
		from_lb,
		from_ub,
		bin_search,
	};

	class AntibandwidthEncoder
	{
	public:
		AntibandwidthEncoder();
		virtual ~AntibandwidthEncoder();

		Graph *graph;
		int process_count = 1;

		int max_width_SAT = std::numeric_limits<int>::min();
		int min_width_UNSAT = std::numeric_limits<int>::max();

		EncoderStrategy enc_choice = duplex;
		IterativeStrategy iterative_strategy = from_lb;

		std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
		std::chrono::time_point<std::chrono::high_resolution_clock> end_time;

		// Solver configurations
		bool force_phase = false;
		bool verbose = true;
		std::string sat_configuration = "sat";

		bool enable_solution_verification = true;
		int split_limit = 0;
		std::string symmetry_break_strategy = "n";

		bool overwrite_lb = false;
		bool overwrite_ub = false;
		int forced_lb = 0;
		int forced_ub = 0;

		void read_graph(std::string graph_file_name);
		void encode_and_solve_abws();
		void encode_and_print_abw_problem(int w);

		void create_limit_pid();
		void create_abp_pid(int width);
		int do_abp_pid_task(int width);

		std::vector<int> get_child_pids(int ppid);		// Get child pids of the given pid
		std::vector<int> get_descendant_pids(int pids); // Get descendant pids of the given pid, include its child pids
		size_t get_total_memory_usage(int pid);			// Get memory usage of the given pid and all of its descendant pids
		size_t get_memory_usage(int pid);				// Get memory usage of the given pid

		int sample_rate = 100000; // Interval of sampler, in microseconds
		int report_rate = 100;	  // Interval of report, in number of sampler
		int sampler_count = 0;

		float memory_limit = std::numeric_limits<float>::max();		  // bound of total memory consumed by all the processes, in megabyte
		float real_time_limit = std::numeric_limits<float>::max();	  // bound of time consumed by main process, in seconds
		float elapsed_time_limit = std::numeric_limits<float>::max(); // bound of total time consumed by all the process, in seconds

		float *max_consumed_memory;
		float consumed_memory = 0;		 // total memory consumed by all the processes, in megabyte
		float consumed_real_time = 0;	 // time consumed by main process, in seconds
		float consumed_elapsed_time = 0; // total time consumed by all the process, in seconds

	private:
		std::unordered_map<int, pid_t> abp_pids;
		pid_t lim_pid;

		void encode_and_solve_abw_problems_from_lb();
		void encode_and_solve_abw_problems_from_ub();
		void encode_and_solve_abw_problems_bin_search();

		void encode_and_solve_abw_problems(int w_from, int w_to, int stop_w);
		bool encode_and_solve_antibandwidth_problem(int w);

		void lookup_bounds(int &lb, int &ub);
		void setup_bounds(int &w_from, int &w_to);

		int is_limit_satified();

		static std::unordered_map<std::string, int> abw_LBs;
		static std::unordered_map<std::string, int> abw_UBs;
	};

}

#endif
