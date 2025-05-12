#ifndef LADDER_ENCODER
#define LADDER_ENCODER

#include "encoder.h"
#include <map>

namespace SATABP
{

    class LadderEncoder : public Encoder
    {
    public:
        LadderEncoder(Graph *g, ClauseContainer *cc, VarHandler *vh);
        virtual ~LadderEncoder();

    private:
        bool is_debug_mode = false;
        bool isUsingProductAndSEQ = true;

        int vertices_aux_var = 0;
        int labels_aux_var = 0;

        // Use to save aux vars of LABELS and VERTICES constraints
        std::map<int, int> aux_vars = {};

        // Use to save aux vars of OBJ-K constraints
        std::map<std::pair<int, int>, int> obj_k_aux_vars;

        // Number of LABELS and VERTICES constraints
        int num_l_v_constraints = 0;

        // Number of OBJ-K constraints
        int num_obj_k_constraints = 0;
        int num_obj_k_glue_staircase_constraint = 0;

        void do_encode_antibandwidth(unsigned w, const std::vector<std::pair<int, int>> &node_pairs) final;

        int do_vars_size() const final;

        int get_aux_var(int symbolicAuxVar);
        int get_obj_k_aux_var(int first, int last);

        void encode_vertices();
        void encode_labels();
        void encode_exactly_one_NSC(std::vector<int> listVars, int auxVar);
        void encode_exactly_one_product(const std::vector<int> &vars);
        void encode_amo_seq(const std::vector<int> &vars);

        void encode_obj_k(unsigned w);
        void encode_stair(int stair, unsigned w);
        void encode_window(int window, int stair, unsigned w);
        void glue_window(int window, int stair, unsigned w);
        void glue_stair(int stair1, int stair2, unsigned w);
    };
}

#endif