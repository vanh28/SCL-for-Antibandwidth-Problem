#include "antibandwidth_encoder.h"

#include <iostream>
#include <assert.h>
#include <chrono>

namespace SATABP
{

    AntibandwidthEncoder::AntibandwidthEncoder() {};

    AntibandwidthEncoder::~AntibandwidthEncoder()
    {
        delete g;
    };

    void AntibandwidthEncoder::read_graph(std::string graph_file_name)
    {
        g = new Graph(graph_file_name);
    };

    void AntibandwidthEncoder::encode_and_solve_abws()
    {
        switch (enc_strategy)
        {
        case from_lb:
            std::cout << "c Solving strategy: from LB to UB." << std::endl;
            encode_and_solve_abw_problems_from_lb();
            break;
        case from_ub:
            std::cout << "c Solving strategy: from UB to LB." << std::endl;
            encode_and_solve_abw_problems_from_ub();
            break;
        case bin_search:
            std::cout << "c Solving strategy: binary search between LB and UB." << std::endl;
            encode_and_solve_abw_problems_bin_search();
            break;
        default:
            std::cerr << "c Unrecognized encoder strategy " << enc_strategy << "." << std::endl;
            return;
        }
    };

    void AntibandwidthEncoder::encode_and_solve_abw_problems(int start_w, int step, int prev_res, int stop_w)
    {
        for (int w = start_w; (w > 0 && w != stop_w && w != w_cap); w += step)
        {
            bool error = encode_and_solve_antibandwidth_problem(w);
            if (error)
                break;
            if (SAT_res != prev_res)
                break; // reached UNSAT->SAT or SAT->UNSAT point
            prev_res = SAT_res;
        }
    };

    void AntibandwidthEncoder::encode_and_solve_abw_problems_from_lb()
    {
        int w_from, w_to;
        setup_bounds(w_from, w_to);
        encode_and_solve_abw_problems(w_from, +1, 10, w_to + 1);
    };

    void AntibandwidthEncoder::encode_and_solve_abw_problems_from_ub()
    {
        int w_from, w_to;
        setup_bounds(w_from, w_to);
        encode_and_solve_abw_problems(w_to, -1, 20, w_from - 1);
    };

    void AntibandwidthEncoder::encode_and_solve_abw_problems_bin_search()
    {
        int w_from, w_to;
        setup_bounds(w_from, w_to);

        int candidate_w = w_from;

        while (w_from <= w_to)
        {
            candidate_w = (w_from + w_to) / 2;
            encode_and_solve_antibandwidth_problem(candidate_w);
            if (SAT_res == 10)
            {
                w_from = candidate_w + 1;
            }
            else if (SAT_res == 20)
            {
                w_to = candidate_w - 1;
            }
            else
            {
                break;
            }
        }
    };

    bool AntibandwidthEncoder::encode_and_solve_antibandwidth_problem(int w)
    {
        std::cout << "c Antibandwidth problem with w = " << w << " ( " << g->graph_name << " ):" << std::endl;
        if (g->n < 1)
        {
            std::cout << "c The input graph is too small, there is nothing to encode here." << std::endl;
            SAT_res = 0; // should break loop
            return 0;
        }
        if (w < 2)
        {
            std::cout << "c There is always at least 1 distance in any labelling. There is nothing to encode here." << std::endl;
            SAT_res = 10; // check solution can not be invoked
            return 0;
        }

        setup_for_solving();
        std::cout << "c Encoding starts with w = " << w << ":" << std::endl;

        auto t1 = std::chrono::high_resolution_clock::now();
        enc->encode_antibandwidth(w, g->edges);
        auto t2 = std::chrono::high_resolution_clock::now();
        auto encode_duration = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count();

        std::cout << "c\tEncoding duration: " << encode_duration << "s" << std::endl;
        std::cout << "c\tNumber of clauses: " << cc->size() << std::endl;
        std::cout << "c\tNumber of irredundant clauses: " << solver->irredundant() << std::endl;
        std::cout << "c\tNumber of variables: " << vh->size() << std::endl;
        std::cout << "c SAT Solving starts:" << std::endl;

        t1 = std::chrono::high_resolution_clock::now();
        SAT_res = solver->solve();
        t2 = std::chrono::high_resolution_clock::now();
        auto solving_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::cout << "c\tSolving duration: " << solving_duration << " ms" << std::endl;
        std::cout << "c\tAnswer: " << std::endl;
        if (SAT_res == 10)
        {
            std::cout << "s SAT (w = " << w << ")" << std::endl;
        }
        else if (SAT_res == 20)
            std::cout << "s UNSAT (w = " << w << ")" << std::endl;
        else
        {
            std::cout << "s Error at w = " << w << ", SAT result: " << SAT_res << std::endl;
            cleanup_solving();
            return 1;
        }

        if (check_solution && SAT_res == 10)
        {
            int solution_abw = calculate_sat_solution();
            if (solution_abw < w)
            {
                std::cerr << "c Error, the solution is not correct, antibandwidth should be at least " << w << ", but it is " << solution_abw << "." << std::endl;

                cleanup_solving();
                return 1;
            }
        }

        cleanup_solving();

        std::cout << "c" << std::endl
                  << "c" << std::endl;
        return 0;
    };

    void AntibandwidthEncoder::encode_and_print_abw_problem(int w)
    {
        setup_for_print();

        enc->encode_antibandwidth(w, g->edges);
        cc->print_dimacs();

        cleanup_print();
    };

    void AntibandwidthEncoder::setup_bounds(int &w_from, int &w_to)
    {
        lookup_bounds(w_from, w_to);

        if (overwrite_lb)
        {
            std::cout << "c LB " << w_from << " is overwritten with " << forced_lb << "." << std::endl;
            w_from = forced_lb;
        }
        if (overwrite_ub)
        {
            std::cout << "c UB " << w_to << " is overwritten with " << forced_ub << "." << std::endl;
            w_to = forced_ub;
        }
        if (w_from > w_to)
        {
            int tmp = w_from;
            w_from = w_to;
            w_to = tmp;
            std::cout << "c Flipped LB and UB to avoid LB > UB ";
            std::cout << "(LB = " << w_from << ", UB = " << w_to << ")." << std::endl;
        }

        assert((w_from <= w_to) && (w_from >= 1));
    };

    void AntibandwidthEncoder::lookup_bounds(int &lb, int &ub)
    {
        auto pos = abw_LBs.find(g->graph_name);
        if (pos != abw_LBs.end())
        {
            lb = pos->second;
            std::cout << "c LB-w = " << lb << " (LB in Sinnl - A note on computational approaches for the antibandwidth problem)." << std::endl;
        }
        else
        {
            lb = 1;
            std::cout << "c No predefined lower bound is found for " << g->graph_name << "." << std::endl;
            std::cout << "c LB-w = 1 (default value)." << std::endl;
        }

        pos = abw_UBs.find(g->graph_name);
        if (pos != abw_UBs.end())
        {
            ub = pos->second;
            if (verbose)
                std::cout << "c UB-w = " << ub << " (UB in Sinnl - A note on computational approaches for the antibandwidth problem)." << std::endl;
        }
        else
        {
            ub = g->n / 2 + 1;
            std::cout << "c No predefined upper bound is found for " << g->graph_name << "." << std::endl;
            std::cout << "c UB-w = " << ub << " (default value calculated as n/2+1)." << std::endl;
        }
    };

    void AntibandwidthEncoder::setup_for_solving()
    {
        setup_cadical();
        vh = new VarHandler(1, g->n);
        cc = new CadicalClauseContainer(vh, split_limit, solver);

        setup_encoder();
    };

    void AntibandwidthEncoder::cleanup_solving()
    {
        delete enc;
        delete cc;
        delete vh;
        delete solver;
    };

    void AntibandwidthEncoder::setup_for_print()
    {
        vh = new VarHandler(1, g->n);
        cc = new ClauseVector(vh, split_limit);

        setup_encoder();
    };

    void AntibandwidthEncoder::cleanup_print()
    {
        delete enc;
        delete cc;
        delete vh;
    };

    void AntibandwidthEncoder::setup_cadical()
    {
        std::cout << "c Initializing CaDiCaL ";
        solver = new CaDiCaL::Solver;

        std::cout << "(version " << solver->version() << ")." << std::endl;

        auto res = solver->configure(sat_configuration.data());
        if (verbose)
            std::cout << "c\tConfiguring CaDiCaL as --" << sat_configuration << " (" << res << ")." << std::endl;

        if (force_phase)
        {
            std::vector<std::string> force_phase_options{"--forcephase", "--phase=0", "--no-rephase"};
            for (unsigned i = 0; i < force_phase_options.size(); ++i)
            {
                res = solver->set_long_option(force_phase_options[i].data());
                if (verbose)
                    std::cout << "c\tCaDiCaL option " << force_phase_options[i] << " added (" << res << ")" << std::endl;
            }
        }
    };

    void AntibandwidthEncoder::setup_encoder()
    {
        switch (enc_choice)
        {
        case duplex:
            std::cout << "c Initializing a Duplex encoder with n = " << g->n << "." << std::endl;
            enc = new DuplexEncoder(g, cc, vh);
            enc->symmetry_break_point = symmetry_break_point;
            break;
        case reduced:
            std::cout << "c Initializing a Naive-Reduced encoder with n = " << g->n << "." << std::endl;
            enc = new ReducedEncoder(g, cc, vh);
            enc->symmetry_break_point = symmetry_break_point;
            break;
        case seq:
            std::cout << "c Initializing a Sequential encoder with n = " << g->n << "." << std::endl;
            enc = new SeqEncoder(g, cc, vh);
            enc->symmetry_break_point = symmetry_break_point;
            break;
        case product:
            std::cout << "c Initializing a 2-Product encoder with n = " << g->n << "." << std::endl;
            enc = new ProductEncoder(g, cc, vh);
            enc->symmetry_break_point = symmetry_break_point;
            break;
        case ladder:
            std::cout << "c Initializing a Ladder encoder with n = " << g->n << "." << std::endl;
            enc = new LadderEncoder(g, cc, vh);
            enc->symmetry_break_point = symmetry_break_point;
            break;
        default:
            std::cerr << "c Unrecognized encoder type " << enc_choice << "." << std::endl;
            return;
        }
    };

    int AntibandwidthEncoder::calculate_sat_solution()
    {
        std::cout << "c\tSolution check:" << std::endl
                  << "p calculated antibandwidth = ";

        std::vector<int> node_labels = std::vector<int>();
        if (!extract_node_labels(node_labels))
            return 0;
        int min_dist = g->calculate_antibandwidth(node_labels);
        std::cout << min_dist << "." << std::endl;

        return min_dist;
    };

    bool AntibandwidthEncoder::extract_node_labels(std::vector<int> &node_labels)
    {
        for (unsigned node = 0; node < g->n; ++node)
        {
            for (unsigned label = 1; label <= g->n; ++label)
            {
                int res = solver->val(node * g->n + label);
                if (res > 0)
                {
                    node_labels.push_back(label);
                }
            }
        }
        if (node_labels.size() > g->n)
        {
            std::cerr << "Error, the solution is not a labelling: more than one label assigned for one of the nodes." << std::endl;
            return false;
        }
        return true;
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
