#include "abp_encoder.h"

#include <iostream>
#include <chrono>

#include "reduced_encoder.h"
#include "sequential_encoder.h"
#include "product_encoder.h"
#include "duplex_encoder.h"
#include "ladder_encoder.h"

namespace SATABP
{
    ABPEncoder::ABPEncoder(std::string symmetry_break_strategy, Graph *graph, int width) : symmetry_break_strategy(symmetry_break_strategy), width(width), graph(graph) {};

    ABPEncoder::~ABPEncoder() {};

    std::string ABPEncoder::get_signature()
    {
        return "[w = " + std::to_string(width) + "]";
    }

    /*
        Return the result of ABP:
        -   0 if graph only contains 1 vertex.
        -   10 if SAT (including w < 2 because it is always SAT).
        -   20 if UNSAT.
        -   -20 for undefined answers.
        -   -10 for incorrect SAT answers.
    */
    int ABPEncoder::encode_and_solve_abp()
    {
        std::cout << "c " + get_signature() + " Antibandwidth problem with w = " << width << " (" << graph->graph_name << "):" << std::endl;
        if (graph->n < 1)
        {
            std::cout << "c " + get_signature() + " The input graph is too small, there is nothing to encode here." << std::endl;
            SAT_res = 0; // should break loop
            return 0;
        }
        if (width < 2)
        {
            std::cout << "c " + get_signature() + " There is always at least 1 distance in any labelling. There is nothing to encode here." << std::endl;
            SAT_res = 10; // check solution can not be invoked
            return 10;
        }

        setup_for_solving();
        std::cout << "c " + get_signature() + " Encoding starts with w = " << width << ":" << std::endl;

        auto t1 = std::chrono::high_resolution_clock::now();
        enc->encode_antibandwidth(width, graph->edges);
        auto t2 = std::chrono::high_resolution_clock::now();
        auto encode_duration = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count();

        std::cout << "c " + get_signature() + " Encoding duration: " << encode_duration << "s" << std::endl;
        std::cout << "c " + get_signature() + " Number of clauses: " << cc->size() << std::endl;
        std::cout << "c " + get_signature() + " Number of irredundant clauses: " << solver->irredundant() << std::endl;
        std::cout << "c " + get_signature() + " Number of variables: " << vh->size() << std::endl;
        std::cout << "c " + get_signature() + " SAT Solving starts:" << std::endl;

        t1 = std::chrono::high_resolution_clock::now();
        SAT_res = solver->solve();
        t2 = std::chrono::high_resolution_clock::now();
        auto solving_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::cout << "c " + get_signature() + " Solving duration: " << solving_duration << " ms" << std::endl;
        std::cout << "c " + get_signature() + " Answer: " << std::endl;
        if (SAT_res == 10)
        {
            std::cout << "s " + get_signature() + " SAT (w = " << width << ")" << std::endl;
        }
        else if (SAT_res == 20)
            std::cout << "s " + get_signature() + " UNSAT (w = " << width << ")" << std::endl;
        else
        {
            std::cout << "s " + get_signature() + " Error at w = " << width << ", SAT result: " << SAT_res << std::endl;
            cleanup_solving();
            return -20;
        }

        if (enable_solution_verification && SAT_res == 10)
        {
            int solution_abp = verify_solution();
            if (solution_abp < width)
            {
                std::cerr << "c " + get_signature() + " Error, the solution is not correct, antibandwidth should be at least " << width << ", but it is " << solution_abp << "." << std::endl;

                cleanup_solving();
                return -10;
            }
        }

        cleanup_solving();

        // std::cout << "c " + get_signature() + " Closed." << std::endl;
        return SAT_res;
    };

    void ABPEncoder::encode_and_print_abp()
    {
        setup_for_print();

        enc->encode_antibandwidth(width, graph->edges);
        cc->print_clauses();

        cleanup_print();
    }

    int ABPEncoder::verify_solution()
    {
        std::vector<int> node_labels = std::vector<int>();
        if (!extract_node_labels(node_labels))
            return 0;
        int min_dist = graph->calculate_antibandwidth(node_labels);

        std::cout << "c " + get_signature() + " Solution check = " << min_dist << "." << std::endl;

        return min_dist;
    }

    bool ABPEncoder::extract_node_labels(std::vector<int> &node_labels)
    {
        for (unsigned node = 0; node < graph->n; ++node)
        {
            for (unsigned label = 1; label <= graph->n; ++label)
            {
                int res = solver->val(node * graph->n + label);
                if (res > 0)
                {
                    node_labels.push_back(label);
                }
            }
        }
        if (node_labels.size() > graph->n)
        {
            std::cerr << "e" + get_signature() + " Error, the solution is not a labelling: more than one label assigned for one of the nodes." << std::endl;
            return false;
        }
        return true;
    }

    void ABPEncoder::setup_for_solving()
    {
        setup_cadical();
        vh = new VarHandler(1, graph->n);
        cc = new CadicalClauseContainer(vh, split_limit, solver);

        setup_encoder();
    }

    void ABPEncoder::cleanup_solving()
    {
        delete enc;
        enc = nullptr;
        delete cc;
        cc = nullptr;
        delete vh;
        vh = nullptr;
        delete solver;
        solver = nullptr;
    }

    void ABPEncoder::setup_for_print()
    {
        vh = new VarHandler(1, graph->n);
        cc = new ClauseVector(vh, split_limit);

        setup_encoder();
    }

    void ABPEncoder::cleanup_print()
    {
        delete enc;
        enc = nullptr;
        delete cc;
        cc = nullptr;
        delete vh;
        vh = nullptr;
    }

    void ABPEncoder::setup_cadical()
    {
        solver = new CaDiCaL::Solver;
        std::cout << "c " + get_signature() + " Initializing CaDiCaL (version " << solver->version() << ")." << std::endl;

        auto res = solver->configure(sat_configuration.data());
        if (verbose)
            std::cout << "c " + get_signature() + " Configuring CaDiCaL as --" << sat_configuration << " (" << res << ")." << std::endl;

        if (force_phase)
        {
            std::vector<std::string> force_phase_options{"--forcephase", "--phase=0", "--no-rephase"};
            for (unsigned i = 0; i < force_phase_options.size(); ++i)
            {
                res = solver->set_long_option(force_phase_options[i].data());
                if (verbose)
                    std::cout << "c " + get_signature() + " CaDiCaL option " << force_phase_options[i] << " added (" << res << ")" << std::endl;
            }
        }
    }

    void ABPEncoder::setup_encoder()
    {
        switch (enc_strategy)
        {
        case duplex:
            std::cout << "c " + get_signature() + " Initializing a Duplex encoder with n = " << graph->n << "." << std::endl;
            enc = new DuplexEncoder(graph, cc, vh);
            enc->symmetry_break_point = symmetry_break_strategy;
            break;
        case reduced:
            std::cout << "c " + get_signature() + " Initializing a Naive-Reduced encoder with n = " << graph->n << "." << std::endl;
            enc = new ReducedEncoder(graph, cc, vh);
            enc->symmetry_break_point = symmetry_break_strategy;
            break;
        case seq:
            std::cout << "c " + get_signature() + " Initializing a Sequential encoder with n = " << graph->n << "." << std::endl;
            enc = new SeqEncoder(graph, cc, vh);
            enc->symmetry_break_point = symmetry_break_strategy;
            break;
        case product:
            std::cout << "c " + get_signature() + " Initializing a 2-Product encoder with n = " << graph->n << "." << std::endl;
            enc = new ProductEncoder(graph, cc, vh);
            enc->symmetry_break_point = symmetry_break_strategy;
            break;
        case ladder:
            std::cout << "c " + get_signature() + " Initializing a Ladder encoder with n = " << graph->n << "." << std::endl;
            enc = new LadderEncoder(graph, cc, vh);
            enc->symmetry_break_point = symmetry_break_strategy;
            break;
        default:
            std::cerr << "c " + get_signature() + " Unrecognized encoder type " << enc_strategy << "." << std::endl;
            return;
        }
    }
}
