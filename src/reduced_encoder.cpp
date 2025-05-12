#include "reduced_encoder.h"

#include <numeric>   //iota
#include <algorithm> //generate
#include <assert.h>
#include <iostream>

namespace SATABP
{

    ReducedEncoder::ReducedEncoder(Graph *g, ClauseContainer *cc, VarHandler *vh)
        : Encoder(g, cc, vh) {
          };

    ReducedEncoder::~ReducedEncoder() {};

    int ReducedEncoder::do_vars_size() const
    {
        return vh->size();
    };

    void ReducedEncoder::do_encode_antibandwidth(unsigned w, const std::vector<std::pair<int, int>> &node_pairs)
    {
        if (symmetry_break_point == std::string("f"))
        {
            encode_symmetry_break();
        }
        else if (symmetry_break_point == std::string("h"))
        {
            encode_symmetry_break_on_maxnode();
        }
        else if (symmetry_break_point == std::string("l"))
        {
            encode_symmetry_break_on_minnode();
        }
        else
        {
            // No symmetry breaking
        }
        std::cout << "c\tEncode symmetry breaking with option: " << symmetry_break_point << "." << std::endl;

        encode_labelling();

        for (std::pair<int, int> nodes : node_pairs)
        {
            encode_pair_amo(w, nodes.first, nodes.second);
        }
    };

    // Enforces that each node takes maximum one label and each label belongs to maximum one node.
    void ReducedEncoder::encode_labelling()
    {
        /*
         * Encode that each node takes exactly one label as exactly-one constraints
         * 1 + 2 + 3 + ... + 29 + 30 <= 1
         * 31 + 32 + 33 + ... + 59 + 60 <= 1
         * ...
         * 871 + 872 + 873 + ... + 899 + 900 <= 1
         */
        for (unsigned i = 0; i < g->n; i++)
        {
            std::vector<int> node_label_eo(g->n);
            std::iota(node_label_eo.begin(), node_label_eo.end(), (i * g->n) + 1);
            encode_eo(node_label_eo.begin(), node_label_eo.end());
        }

        /*
         * Encode that each label is assigned to exactly one node as exactly-one constraints.
         * 1 + 31 + 61 + ... + 841 + 871 <= 1
         * 2 + 32 + 62 + ... + 842 + 872 <= 1
         * ...
         * 30 + 60 + 90 + ... + 870 + 900 <= 1
         */
        for (unsigned i = 0; i < g->n; i++)
        {
            std::vector<int> label_node_eo(g->n);
            int j = 0;
            std::generate(label_node_eo.begin(), label_node_eo.end(), [this, &j, i]()
                          { return (j++ * g->n) + i + 1; });
            encode_eo(label_node_eo.begin(), label_node_eo.end());
        }
    };

    void ReducedEncoder::encode_pair_amo(int w, int node1, int node2)
    {
        assert(node1 != node2);
        assert(0 < node1 && node1 <= g->n);
        assert(0 < node2 && node2 <= g->n);
        std::deque<int> amo_node1(w);
        std::deque<int> amo_node2(w);
        int amo_node1_from = (node1 - 1) * g->n + 1;
        int amo_node2_from = (node2 - 1) * g->n + 1;
        int amo_node1_to = node1 * g->n; // last variable belonging to node1
        int amo_node2_to = node2 * g->n;

        std::iota(amo_node1.begin(), amo_node1.end(), amo_node1_from);
        std::iota(amo_node2.begin(), amo_node2.end(), amo_node2_from);

        encode_glued_first_amo(amo_node1.begin(), amo_node1.end(), amo_node2.begin(), amo_node2.end());
        while (amo_node1.back() < amo_node1_to && amo_node2.back() < amo_node2_to)
        {
            encode_next_window(amo_node1.begin(), amo_node1.end(), amo_node2.begin(), amo_node2.end(), amo_node1.back() + 1, amo_node2.back() + 1);
            amo_node1.push_back(amo_node1.back() + 1);
            amo_node1.pop_front();
            amo_node2.push_back(amo_node2.back() + 1);
            amo_node2.pop_front();
        }
    };

    void ReducedEncoder::encode_eo(vec_int_it it_begin, vec_int_it it_end)
    {
        std::vector<int> or_clause;
        for (auto i_pos = it_begin, it_last = std::prev(it_end); i_pos != it_last; ++i_pos)
        {
            or_clause.push_back(*i_pos);
            for (auto j_pos = std::next(i_pos); j_pos != it_end; ++j_pos)
            {
                cv->add_clause({-1 * (*i_pos), -1 * (*j_pos)});
            }
        }
        or_clause.push_back(*std::prev(it_end));
        cv->add_clause(or_clause);
    };

    void ReducedEncoder::encode_glued_first_amo(deq_int_it amo1_begin, deq_int_it amo1_end, deq_int_it amo2_begin, deq_int_it amo2_end)
    {
        int i_count = 1;
        for (auto i_pos = amo1_begin; i_pos != amo1_end; ++i_pos, ++i_count)
        {
            int j_count = 1;
            for (auto j_pos = amo2_begin; j_pos != amo2_end; ++j_pos, ++j_count)
            {
                if (i_count != j_count)
                {
                    cv->add_clause({-1 * (*i_pos), -1 * (*j_pos)});
                }
            }
        }
    };

    void ReducedEncoder::encode_next_window(deq_int_it amo1_begin, deq_int_it amo1_end, deq_int_it amo2_begin, deq_int_it amo2_end, int new_g1, int new_g2)
    {
        for (auto i_pos = std::next(amo1_begin); i_pos != amo1_end; ++i_pos)
        {
            cv->add_clause({-1 * (*i_pos), -1 * new_g2});
        }
        for (auto i_pos = std::next(amo2_begin); i_pos != amo2_end; ++i_pos)
        {
            cv->add_clause({-1 * new_g1, -1 * (*i_pos)});
        }
    };

}
