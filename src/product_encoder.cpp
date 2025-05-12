#include "product_encoder.h"

#include <numeric>   //iota
#include <algorithm> //generate
#include <cmath>     //floor,ceil
#include <deque>
#include <assert.h>
#include <iostream>
#include <iterator>

namespace SATABP
{

    ProductEncoder::ProductEncoder(Graph *g, ClauseContainer *cc, VarHandler *vh)
        : Encoder(g, cc, vh) {
          };

    ProductEncoder::~ProductEncoder() {};

    int ProductEncoder::do_vars_size() const
    {
        return vh->size();
    };

    void ProductEncoder::do_encode_antibandwidth(unsigned w, std::vector<std::pair<int, int>> const &node_pairs)
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

    void ProductEncoder::encode_labelling()
    {
        for (unsigned i = 0; i < g->n; i++)
        {
            std::vector<int> node_label_eo(g->n);
            std::iota(node_label_eo.begin(), node_label_eo.end(), (i * g->n) + 1);
            encode_eo(node_label_eo.begin(), node_label_eo.end());
        }

        for (unsigned i = 0; i < g->n; i++)
        {
            std::vector<int> label_node_eo(g->n);
            int j = 0;
            std::generate(label_node_eo.begin(), label_node_eo.end(), [this, &j, i]()
                          { return (j++ * g->n) + i + 1; });
            encode_eo(label_node_eo.begin(), label_node_eo.end());
        }
    };

    void ProductEncoder::encode_pair_amo(int w, int node1, int node2)
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

        while (amo_node1.back() <= amo_node1_to && amo_node2.back() <= amo_node2_to)
        {
            encode_glued_amo(amo_node1.begin(), amo_node1.end(), amo_node2.begin(), amo_node2.end());
            amo_node1.push_back(amo_node1.back() + 1);
            amo_node1.pop_front();
            amo_node2.push_back(amo_node2.back() + 1);
            amo_node2.pop_front();
        }
    };

    void ProductEncoder::encode_eo(vec_int_it it_begin, vec_int_it it_end)
    {
        unsigned constr_length = std::distance(it_begin, it_end);
        if (constr_length < 2)
            return;
        if (constr_length == 2)
        {
            int v1 = *it_begin;
            int v2 = *std::next(it_begin);
            cv->add_clause({v1, v2});
            cv->add_clause({-1 * v1, -1 * v2});
            return;
        }

        int p = std::ceil(std::sqrt(constr_length));
        int q = std::ceil((float)constr_length / (float)p);

        std::vector<int> u_vars;
        std::vector<int> v_vars;
        for (int i = 1; i <= p; ++i)
            u_vars.push_back(vh->get_new_var());
        for (int j = 1; j <= q; ++j)
            v_vars.push_back(vh->get_new_var());

        int i, j, curr;
        std::vector<int> or_clause = std::vector<int>();
        int idx = 0;
        for (auto i_pos = it_begin; i_pos != it_end; ++i_pos)
        {
            i = std::floor(idx / p);
            j = idx % p;
            curr = *i_pos;
            cv->add_clause({-1 * curr, v_vars[i]});
            cv->add_clause({-1 * curr, u_vars[j]});

            or_clause.push_back(curr);
            ++idx;
        }
        cv->add_clause(or_clause);

        encode_amo(u_vars.begin(), u_vars.end());
        encode_amo(v_vars.begin(), v_vars.end());
    };

    void ProductEncoder::encode_amo(vec_int_it it_begin, vec_int_it it_end)
    {
        unsigned constr_length = std::distance(it_begin, it_end);
        if (constr_length < 2)
            return;
        if (constr_length == 2)
        {
            int v1 = *it_begin;
            int v2 = *std::next(it_begin);
            cv->add_clause({v1, v2});
            cv->add_clause({-1 * v1, -1 * v2});
            return;
        }

        int p = std::ceil(std::sqrt(constr_length));
        int q = std::ceil((float)constr_length / (float)p);

        std::vector<int> u_vars;
        std::vector<int> v_vars;
        for (int i = 1; i <= p; ++i)
            u_vars.push_back(vh->get_new_var());
        for (int j = 1; j <= q; ++j)
            v_vars.push_back(vh->get_new_var());

        int i, j, curr, idx = 0;
        for (auto i_pos = it_begin; i_pos != it_end; ++i_pos)
        {
            i = std::floor(idx / p);
            j = idx % p;
            curr = *i_pos;
            cv->add_clause({-1 * curr, v_vars[i]});
            cv->add_clause({-1 * curr, u_vars[j]});
            ++idx;
        }
        encode_amo(u_vars.begin(), u_vars.end());
        encode_amo(v_vars.begin(), v_vars.end());
    };

    void ProductEncoder::encode_glued_amo(deq_int_it amo1_begin, deq_int_it amo1_end, deq_int_it amo2_begin, deq_int_it amo2_end)
    {
        unsigned constr_length = std::distance(amo1_begin, amo1_end);
        constr_length += std::distance(amo2_begin, amo2_end);

        assert(constr_length > 2);

        int p = std::ceil(std::sqrt(constr_length));
        int q = std::ceil((float)constr_length / (float)p);

        std::vector<int> u_vars;
        std::vector<int> v_vars;
        for (int i = 1; i <= p; ++i)
            u_vars.push_back(vh->get_new_var());
        for (int j = 1; j <= q; ++j)
            v_vars.push_back(vh->get_new_var());

        int i, j, curr, idx = 0;
        for (auto i_pos = amo1_begin, amo1_last = std::prev(amo1_end); i_pos != amo2_end;)
        {
            i = std::floor(idx / p);
            j = idx % p;
            curr = *i_pos;
            cv->add_clause({-1 * curr, v_vars[i]});
            cv->add_clause({-1 * curr, u_vars[j]});

            if (i_pos == amo1_last)
            {
                i_pos = amo2_begin;
            }
            else
            {
                ++i_pos;
            }
            ++idx;
        }
        encode_amo(u_vars.begin(), u_vars.end());
        encode_amo(v_vars.begin(), v_vars.end());
    };

}
