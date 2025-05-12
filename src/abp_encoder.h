#ifndef ABP_ENCODER_H
#define ABP_ENCODER_H

#include "antibandwidth_encoder.h"

#include <string>
#include <vector>

#include "utils.h"
#include "clause_cont.h"
#include "cadical_clauses.h"
#include "encoder.h"

namespace SATABP
{
    class ABPEncoder
    {
    public:
        ABPEncoder(std::string symmetry_break_strategy, Graph *graph, int width);
        virtual ~ABPEncoder();

        EncoderStrategy enc_strategy = ladder; // Default strategy

        // Solver configurations
        bool force_phase = false;
        bool verbose = true;
        std::string sat_configuration = "sat";

        bool enable_solution_verification = true;
        int split_limit = 0;
        std::string symmetry_break_strategy = "n";

        int encode_and_solve_abp();
        void encode_and_print_abp();

    private:
        int SAT_res = 0;
        int width = 0;

        Graph *graph;
        VarHandler *vh;
        Encoder *enc;
        ClauseContainer *cc;
        CaDiCaL::Solver *solver;

        int verify_solution();
        bool extract_node_labels(std::vector<int> &node_labels);
        void setup_for_solving();
        void cleanup_solving();
        void setup_for_print();
        void cleanup_print();

        void setup_cadical();
        void setup_encoder();

        std::string get_signature();
    };
}

#endif