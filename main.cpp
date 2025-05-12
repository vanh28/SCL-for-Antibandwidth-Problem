#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <signal.h>
#include <stdexcept>
#include <map>
#include "src/antibandwidth_encoder.h"

using namespace SATABP;

const int ver{0};

static void SIGINT_exit(int);

static void (*signal_SIGINT)(int);
static void (*signal_SIGXCPU)(int);
static void (*signal_SIGSEGV)(int);
static void (*signal_SIGTERM)(int);
static void (*signal_SIGABRT)(int);

static void SIGINT_exit(int signum)
{
    signal(SIGINT, signal_SIGINT);
    signal(SIGXCPU, signal_SIGXCPU);
    signal(SIGSEGV, signal_SIGSEGV);
    signal(SIGTERM, signal_SIGTERM);
    signal(SIGABRT, signal_SIGABRT);

    std::cout << "c Signal interruption." << std::endl;

    fflush(stdout);
    fflush(stderr);

    raise(signum);
}

static const std::map<std::string, std::string> option_list = {
    {"--help", "Print usage message with all possible options"},
    {"--reduced", "Use reduced naive encoding for staircase constraints [default: false]"},
    {"--seq", "Use sequential encoding for staircase constraints [default: false]"},
    {"--product", "Use 2-Product encoding for staircase constraints [default: false]"},
    {"--duplex", "Use duplex encoding for staircase constraints [default: true]"},
    {"--ladder", "Use ladder encoding for staircase constraints and NSC for At-Most-One constraints [default: false]"},
    {"--conf-sat", "Use --sat configuration of CaDiCaL [default: true]"},
    {"--conf-unsat", "Use --unsat configuration of CaDiCaL [default: false]"},
    {"--force-phase", "Set options --forcephase,--phase=0 and --no-rephase of CaDiCal [default: false]"},
    {"--verify-result", "Verify the antibandwidth of the found SAT solution [default: false]"},
    {"--from-ub", "Start solving with width = UB, decreasing in each iteration [default: false]"},
    {"--from-lb", "Start solving with width = LB, increasing in each iteration [default: true]"},
    {"--bin-search", "Start solving with LB+UB/2 and update LB or UB according to SAT/UNSAT result and repeat"},
    {"-split-size <n>", "Maximal allowed length of clauses, every longer clause is split up into two by introducing a new variable"},
    {"-set-lb <new LB>", "Overwrite predefined LB with <new LB>, has to be at least 2"},
    {"-set-ub <new UB>", "Overwrite predefined UB with <new UB>, has to be positive"},
    {"-limit-memory <MB>", "Overwrite memory consumption restriction [default: FLOAT_MAX]"},
    {"-limit-real-time <seconds>", "Overwrite real time consumption restriction [default: FLOAT_MAX]"},
    {"-limit-elapsed-time <seconds>", "Overwrite elapsed time consumption restriction [default: FLOAT_MAX]"},
    {"-sample-rate <microseconds>", "Overwrite sampler interval [default: 100000]"},
    {"-report-rate <sampler>", "Overwrite elapsed time interval [default: 100 samplers generates 1 report]"},
    {"-symmetry-break <break point>", "Apply symetry breaking technique in <break point> (f: first node, h: highest degree node, l: lowest degree node, n: none) [default: none]"},
    {"-print-w <w>", "Only encode and print SAT formula of specified width w (where w > 0), without solving it"},
    {"-process-count <number process>", "Number processes used to solve"}};

int get_number_arg(std::string const &arg)
{
    try
    {
        std::size_t pos;
        int x = std::stoi(arg, &pos);
        if (pos < arg.size())
        {
            std::cerr << "Trailing characters after number: " << arg << '\n';
        }
        return x;
    }
    catch (std::invalid_argument const &ex)
    {
        std::cerr << "Invalid number: " << arg << '\n';
        return 0;
    }
    catch (std::out_of_range const &ex)
    {
        std::cerr << "Number out of range: " << arg << '\n';
        return 0;
    }
}

void print_usage()
{
    std::cout << "usage: abw_enc path_to_graph_file/graph_file.mtx.rnd [ <option> ... ] " << std::endl;
    std::cout << "where '<option>' is one of the following options:" << std::endl;
    std::cout << std::endl;
    for (auto option : option_list)
    {
        std::cout << std::left << "\t" << std::setw(30) << option.first << "\t" << option.second << std::endl;
    }
    std::cout << std::endl;
}

int main(int argc, char **argv)
{
    signal_SIGINT = signal(SIGINT, SIGINT_exit);
    signal_SIGXCPU = signal(SIGXCPU, SIGINT_exit);
    signal_SIGSEGV = signal(SIGSEGV, SIGINT_exit);
    signal_SIGTERM = signal(SIGTERM, SIGINT_exit);
    signal_SIGABRT = signal(SIGABRT, SIGINT_exit);

    AntibandwidthEncoder *abw_enc;

    std::string graph_file;

    bool just_print_dimacs = false;
    int spec_width = 2;

    if (argc < 2)
    {
        std::cout << "c LadderEncoder 1." << ver << "." << std::endl;
        std::cerr << "c Error, no graph file was specified." << std::endl;
        print_usage();
        return 1;
    }

    std::cout << "c LadderEncoder 1." << ver << "." << std::endl;

    abw_enc = new AntibandwidthEncoder();

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            abw_enc->read_graph(argv[i]);
        }
        else if (argv[i] == std::string("--help"))
        {
            print_usage();
            delete abw_enc;
            return 1;
        }
        else if (argv[i] == std::string("--reduced"))
        {
            abw_enc->enc_choice = EncoderStrategy::reduced;
        }
        else if (argv[i] == std::string("--seq"))
        {
            abw_enc->enc_choice = EncoderStrategy::seq;
        }
        else if (argv[i] == std::string("--product"))
        {
            abw_enc->enc_choice = EncoderStrategy::product;
        }
        else if (argv[i] == std::string("--duplex"))
        {
            abw_enc->enc_choice = EncoderStrategy::duplex;
        }
        else if (argv[i] == std::string("--ladder"))
        {
            abw_enc->enc_choice = EncoderStrategy::ladder;
        }
        else if (argv[i] == std::string("--conf-sat"))
        {
            abw_enc->sat_configuration = "sat";
        }
        else if (argv[i] == std::string("--conf-unsat"))
        {
            abw_enc->sat_configuration = "unsat";
        }
        else if (argv[i] == std::string("--conf-def"))
        {
            abw_enc->sat_configuration = "";
        }
        else if (argv[i] == std::string("--force-phase"))
        {
            abw_enc->force_phase = true;
        }
        else if (argv[i] == std::string("--verify-result"))
        {
            abw_enc->enable_solution_verification = true;
        }
        else if (argv[i] == std::string("--from-ub"))
        {
            abw_enc->iterative_strategy = IterativeStrategy::from_ub;
        }
        else if (argv[i] == std::string("--from-lb"))
        {
            abw_enc->iterative_strategy = IterativeStrategy::from_lb;
        }
        else if (argv[i] == std::string("--bin-search"))
        {
            abw_enc->iterative_strategy = IterativeStrategy::bin_search;
        }
        else if (argv[i] == std::string("-print-w"))
        {
            spec_width = get_number_arg(argv[++i]);
            if (spec_width < 2)
            {
                std::cout << "Error, width has to be at least 2." << std::endl;
                delete abw_enc;
                return 1;
            }
            std::cout << "c DIMACS Printing mode for w = " << spec_width << "." << std::endl;
            just_print_dimacs = true;
        }
        else if (argv[i] == std::string("-set-lb"))
        {
            abw_enc->forced_lb = get_number_arg(argv[++i]);
            if (abw_enc->forced_lb < 2)
            {
                std::cout << "Error, width has to be at least 2." << std::endl;
                delete abw_enc;
                return 1;
            }
            abw_enc->overwrite_lb = true;
            std::cout << "c LB is predefined as " << abw_enc->forced_lb << "." << std::endl;
        }
        else if (argv[i] == std::string("-set-ub"))
        {
            abw_enc->forced_ub = get_number_arg(argv[++i]);
            if (abw_enc->forced_ub < 0)
            {
                std::cout << "Error, width has to be positive." << std::endl;
                delete abw_enc;
                return 1;
            }
            abw_enc->overwrite_ub = true;
            std::cout << "c UB is predefined as " << abw_enc->forced_ub << "." << std::endl;
        }
        else if (argv[i] == std::string("-limit-memory"))
        {
            int lim_mem = get_number_arg(argv[++i]);
            if (lim_mem <= 0)
            {
                std::cout << "Error, memory limit has to be positive." << std::endl;
                delete abw_enc;
                return 1;
            }
            std::cout << "c Memory limit is set to " << lim_mem << "." << std::endl;
            abw_enc->memory_limit = lim_mem;
        }
        else if (argv[i] == std::string("-limit-real-time"))
        {
            int limit_real_time = get_number_arg(argv[++i]);
            if (limit_real_time <= 0)
            {
                std::cout << "Error, real time limit has to be positive." << std::endl;
                delete abw_enc;
                return 1;
            }
            std::cout << "c Real time limit is set to " << limit_real_time << "." << std::endl;
            abw_enc->real_time_limit = limit_real_time;
        }
        else if (argv[i] == std::string("-limit-elapsed-time"))
        {
            int limit_elapsed_time = get_number_arg(argv[++i]);
            if (limit_elapsed_time <= 0)
            {
                std::cout << "Error, elapsed time limit has to be positive." << std::endl;
                delete abw_enc;
                return 1;
            }
            std::cout << "c Elapsed time limit is set to " << limit_elapsed_time << "." << std::endl;
            abw_enc->elapsed_time_limit = limit_elapsed_time;
        }
        else if (argv[i] == std::string("-sample-rate"))
        {
            int sample_rate = get_number_arg(argv[++i]);
            if (sample_rate <= 0)
            {
                std::cout << "Error, sample rate has to be positive." << std::endl;
                delete abw_enc;
                return 1;
            }
            std::cout << "c Sample rate is set to " << sample_rate << "." << std::endl;
            abw_enc->sample_rate = sample_rate;
        }
        else if (argv[i] == std::string("-report-rate"))
        {
            int report_rate = get_number_arg(argv[++i]);
            if (report_rate <= 0)
            {
                std::cout << "Error, sample rate has to be positive." << std::endl;
                delete abw_enc;
                return 1;
            }
            std::cout << "c Sample rate is set to " << report_rate << "." << std::endl;
            abw_enc->report_rate = report_rate;
        }
        else if (argv[i] == std::string("-split-size"))
        {
            int split_size = get_number_arg(argv[++i]);
            if (split_size < 0)
            {
                std::cout << "Error, split size has to be positive." << std::endl;
                delete abw_enc;
                return 1;
            }
            std::cout << "c Splitting clauses at length " << split_size << "." << std::endl;
            abw_enc->split_limit = split_size;
        }
        else if (argv[i] == std::string("-symmetry-break"))
        {
            abw_enc->symmetry_break_strategy = argv[++i];
        }
        else if (argv[i] == std::string("-process-count"))
        {
            abw_enc->process_count = get_number_arg(argv[++i]);
        }
        else
        {
            std::cerr << "Unrecognized option: " << argv[i] << std::endl;

            delete abw_enc;
            return 1;
        }
    }

    if (just_print_dimacs)
    {
        abw_enc->encode_and_print_abw_problem(spec_width);
    }
    else
    {
        abw_enc->encode_and_solve_abws();
    }

    delete abw_enc;
    return 0;
}
