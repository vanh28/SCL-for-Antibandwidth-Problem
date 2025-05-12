#include "duplex_encoder.h"

#include <iostream>
#include <numeric>   //iota
#include <algorithm> //reverse
#include <assert.h>
#include <cmath> //floor,ceil

namespace SATABP
{
    DuplexEncoder::DuplexEncoder(Graph *g, ClauseContainer *cc, VarHandler *vh)
        : Encoder(g, cc, vh)
    {
        init_members();
    }

    void DuplexEncoder::init_members()
    {
        fwd_amo_roots = std::unordered_map<int, std::vector<int>>();
        bwd_amo_roots = std::unordered_map<int, std::vector<int>>();
        fwd_amz_roots = std::unordered_map<int, std::vector<int>>();
        bwd_amz_roots = std::unordered_map<int, std::vector<int>>();

        node_amz_literals = std::unordered_map<int, std::vector<std::vector<int>>>();
    };

    DuplexEncoder::~DuplexEncoder() {}

    int DuplexEncoder::do_vars_size() const
    {
        return vh->size();
    };

    void DuplexEncoder::do_encode_antibandwidth(unsigned w, const std::vector<std::pair<int, int>> &node_pairs)
    {
        num_l_v_constraints = 0;
        num_obj_k_constraints = 0;
        num_obj_k_glue_staircase_constraint = 0;
        num_l_v_aux_vars = 0;
        num_obj_k_aux_vars = 0;

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
        // std::cout << "c\tEncode symmetry breaking with option: " << symmetry_break_point << "." << std::endl;

        construct_window_bdds(w);
        glue_window_bdds();

        for (std::pair<int, int> nodes : node_pairs)
        {
            // std::cout << "(" << nodes.first << ", " << nodes.second << ")" << std::endl;
            glue_edge_windows(nodes.first - 1, nodes.second - 1);
        }
        node_amz_literals.clear();

        encode_column_eo();

        // std::cout << "c\tLabels and Vertices aux var: " << num_l_v_aux_vars << std::endl;
        // std::cout << "c\tLabels and Vertices constraints:  " << num_l_v_constraints << std::endl;
        // std::cout << "c\tObj k aux var: " << num_obj_k_aux_vars << std::endl;
        // std::cout << "c\tObj k constraints: " << num_obj_k_constraints << std::endl;
        // std::cout << "c\tObj k glue staircase constraints: " << num_obj_k_glue_staircase_constraint << std::endl;
    };

    void DuplexEncoder::seq_encode_column_eo()
    {
        for (unsigned i = 0; i < g->n; i++)
        {
            std::vector<int> label_node_eo(g->n);
            int j = 0;
            std::generate(label_node_eo.begin(), label_node_eo.end(), [this, &j, i]()
                          { return (j++ * g->n) + i + 1; });

            std::vector<int> or_clause;
            auto it_begin = label_node_eo.begin();
            auto it_end = label_node_eo.end();
            int prev = *it_begin;
            or_clause.push_back(prev);
            for (auto i_pos = std::next(it_begin), it_last = std::prev(it_end); i_pos != it_last; ++i_pos)
            {
                int curr = *i_pos;
                int next = vh->get_new_var();
                num_l_v_aux_vars++;
                cv->add_clause({-1 * prev, -1 * curr});
                num_l_v_constraints++;
                cv->add_clause({-1 * prev, next});
                num_l_v_constraints++;
                cv->add_clause({-1 * curr, next});
                num_l_v_constraints++;

                or_clause.push_back(curr);
                prev = next;
            }
            cv->add_clause({-1 * prev, -1 * (*std::prev(it_end))});
            num_l_v_constraints++;

            or_clause.push_back(*std::prev(it_end));
            cv->add_clause(or_clause);
            num_l_v_constraints++;
        }
    };

    void DuplexEncoder::encode_column_eo()
    {
        for (unsigned i = 0; i < g->n; i++)
        {
            std::vector<int> label_node_eo(g->n);
            int j = 0;
            std::generate(label_node_eo.begin(), label_node_eo.end(), [this, &j, i]()
                          { return (j++ * g->n) + i + 1; });
            product_encode_eo(label_node_eo);
        }
    };

    // Product then encode by seq
    void DuplexEncoder::product_encode_eo(const std::vector<int> &vars)
    {
        if (vars.size() < 2)
            return;

        // If only have 2 vars, then use binomial encoding
        if (vars.size() == 2)
        {
            // simplifies to vars[0] /\ -1*vars[0], in case vars[0] == vars[1]
            cv->add_clause({vars[0], vars[1]});
            num_l_v_constraints++;
            cv->add_clause({-1 * vars[0], -1 * vars[1]});
            num_l_v_constraints++;
            return;
        }

        int len = vars.size();
        int p = std::ceil(std::sqrt(len));
        int q = std::ceil((float)len / (float)p);

        std::vector<int> u_vars;
        std::vector<int> v_vars;
        for (int i = 1; i <= p; ++i)
        {
            u_vars.push_back(vh->get_new_var());
            num_l_v_aux_vars++;
        }
        for (int j = 1; j <= q; ++j)
        {
            v_vars.push_back(vh->get_new_var());
            num_l_v_aux_vars++;
        }

        int i, j;
        std::vector<int> or_clause = std::vector<int>();
        for (unsigned idx = 0; idx < vars.size(); ++idx)
        {
            i = std::floor(idx / p);
            j = idx % p;

            cv->add_clause({-1 * vars[idx], v_vars[i]});
            num_l_v_constraints++;
            cv->add_clause({-1 * vars[idx], u_vars[j]});
            num_l_v_constraints++;

            // At least one
            or_clause.push_back(vars[idx]);
        }
        cv->add_clause(or_clause);
        num_l_v_constraints++;

        // Similar results (faster on small instances, slightly worse on large)
        // product_encode_amo(u_vars);
        // product_encode_amo(v_vars);

        seq_encode_amo(u_vars);
        seq_encode_amo(v_vars);
    };

    void DuplexEncoder::product_encode_amo(const std::vector<int> &vars)
    {
        if (vars.size() < 2)
            return;
        if (vars.size() == 2)
        {
            if (vars[0] == vars[1])
                return;
            cv->add_clause({-1 * vars[0], -1 * vars[1]});
            num_l_v_constraints++;
            return;
        }

        int len = vars.size();
        int p = std::ceil(std::sqrt(len));
        int q = std::ceil((float)len / (float)p);

        std::vector<int> u_vars;
        std::vector<int> v_vars;
        for (int i = 1; i <= p; ++i)
        {
            u_vars.push_back(vh->get_new_var());
            num_l_v_aux_vars++;
        }
        for (int j = 1; j <= q; ++j)
        {
            v_vars.push_back(vh->get_new_var());
            num_l_v_aux_vars++;
        }

        int i, j;

        for (unsigned idx = 0; idx < vars.size(); ++idx)
        {
            i = std::floor(idx / p);
            j = idx % p;

            cv->add_clause({-1 * vars[idx], v_vars[i]});
            num_l_v_constraints++;
            cv->add_clause({-1 * vars[idx], u_vars[j]});
            num_l_v_constraints++;
        }

        product_encode_amo(u_vars);
        product_encode_amo(v_vars);
    };

    void DuplexEncoder::seq_encode_amo(const std::vector<int> &vars)
    {
        if (vars.size() < 2)
            return;

        int prev = vars[0];

        for (unsigned idx = 1; idx < vars.size() - 1; ++idx)
        {
            int curr = vars[idx];
            int next = vh->get_new_var();
            num_l_v_aux_vars++;
            cv->add_clause({-1 * prev, -1 * curr});
            num_l_v_constraints++;
            cv->add_clause({-1 * prev, next});
            num_l_v_constraints++;
            cv->add_clause({-1 * curr, next});
            num_l_v_constraints++;

            prev = next;
        }
        cv->add_clause({-1 * prev, -1 * vars[vars.size() - 1]});
        num_l_v_constraints++;
    };

    void DuplexEncoder::construct_window_bdds(int w)
    {
        number_of_windows = g->n / w;
        last_window_w = g->n % w;
        if (last_window_w != 0)
            number_of_windows++;

        for (unsigned i = 0; i < g->n; ++i)
        {
            fwd_amo_roots[i] = std::vector<int>();
            bwd_amo_roots[i] = std::vector<int>();
            fwd_amz_roots[i] = std::vector<int>();
            bwd_amz_roots[i] = std::vector<int>();

            for (unsigned gw = 0; gw < number_of_windows; ++gw)
            {
                bool last_window = (gw == number_of_windows - 1);
                int p1 = (i * g->n) + (gw * w) + 1;
                int p2 = p1 + w - 1;
                if (last_window)
                    p2 = (i + 1) * g->n;

                std::deque<unsigned int> window_vars(p2 - p1 + 1);
                std::iota(window_vars.begin(), window_vars.end(), p1);

                int fwd_amo_id;
                int fwd_amz_id;
                if (gw != number_of_windows - 1)
                {
                    fwd_amo_id = build_amo(window_vars);
                    fwd_amz_id = build_amz(window_vars);
                    fwd_amo_roots[i].push_back(fwd_amo_id);
                    fwd_amz_roots[i].push_back(fwd_amz_id);
                }

                int bwd_amo_id;
                int bwd_amz_id;
                if (gw != 0)
                {
                    std::reverse(std::begin(window_vars), std::end(window_vars));

                    bwd_amo_id = build_amo(window_vars);
                    bwd_amz_id = build_amz(window_vars);

                    bwd_amo_roots[i].push_back(bwd_amo_id);
                    bwd_amz_roots[i].push_back(bwd_amz_id);
                }

                if (gw == number_of_windows - 1)
                {
                    fwd_amo_roots[i].push_back(bwd_amo_id);
                    fwd_amz_roots[i].push_back(bwd_amz_id);
                }
                else if (gw == 0)
                {
                    bwd_amo_roots[i].push_back(fwd_amo_id);
                    bwd_amz_roots[i].push_back(fwd_amz_id);
                }
                else
                {
                    make_equal_bdds(fwd_amo_id, bwd_amo_id);
                    make_equal_bdds(fwd_amz_id, bwd_amz_id);
                }

                if (window_vars.size() > 1)
                {
                    cv->add_clause({fwd_amo_id});
                    num_obj_k_constraints++;
                }
            }

            assert(!fwd_amz_roots[i].empty());

            std::vector<int> amz_clause;
            for (unsigned f = 0; f < fwd_amz_roots[i].size() - 1; ++f)
            {
                amz_clause.push_back(-1 * fwd_amz_roots[i][f]);

                for (unsigned g = f + 1; g < fwd_amz_roots[i].size(); ++g)
                {
                    cv->add_clause({fwd_amz_roots[i][f], fwd_amz_roots[i][g]});
                    num_l_v_constraints++;
                }
            }
            amz_clause.push_back(-1 * fwd_amz_roots[i].back());
            if (!amz_clause.empty())
            {
                cv->add_clause(amz_clause);
                num_l_v_constraints++;
            }
        }
    };

    void DuplexEncoder::glue_window_bdds()
    {
        for (unsigned var_group = 0; var_group < g->n; ++var_group)
        {
            node_amz_literals[var_group] = std::vector<std::vector<int>>();
            for (unsigned curr_window = 0; curr_window < number_of_windows - 1; ++curr_window)
            {
                node_amz_literals[var_group].push_back({fwd_amz_roots[var_group][curr_window]});

                int next_window = curr_window + 1;

                int curr_fwd_amo = fwd_amo_roots[var_group][curr_window]; // bdd_amo_it
                int next_bwd_amo = bwd_amo_roots[var_group][next_window];

                int fwd_from = bh.bdds[curr_fwd_amo].i_from;
                int fwd_to = bh.bdds[curr_fwd_amo].i_to;

                int bwd_from = bh.bdds[next_bwd_amo].i_from;
                int bwd_to = bh.bdds[next_bwd_amo].i_to;

                assert(bwd_to == fwd_to + 1);

                if (fwd_from != fwd_to)
                {
                    cv->add_clause({curr_fwd_amo});
                    num_obj_k_constraints++;
                }
                if (bwd_from != bwd_to)
                {
                    cv->add_clause({next_bwd_amo});
                    num_obj_k_constraints++;
                }

                std::deque<int> fwd_window(fwd_to - fwd_from + 1);
                std::iota(fwd_window.begin(), fwd_window.end(), fwd_from);
                fwd_window.pop_front(); // the full window was already saved as unit clause

                std::deque<int> bwd_window;
                bwd_window.push_back(fwd_to + 1);

                std::pair<int, int> fwd_p, bwd_p;

                while (fwd_window.size() > 0 && bwd_window.back() <= bwd_from)
                {

                    fwd_p = std::pair<int, int>(fwd_window.front(), fwd_window.back());
                    bwd_p = std::pair<int, int>(bwd_window.back(), bwd_window.front());

                    int b1_amo, b1_amz, b2_amo, b2_amz;

                    bh.lookup_amo(fwd_p, b1_amo);
                    assert(b1_amo != 0);
                    bh.lookup_amz(fwd_p, b1_amz);
                    assert(b1_amz != 0);
                    bh.lookup_amo(bwd_p, b2_amo);
                    assert(b2_amo != 0);
                    bh.lookup_amz(bwd_p, b2_amz);
                    assert(b2_amz != 0);

                    if (fwd_window.size() > 1)
                    {
                        cv->add_clause({b1_amo});
                        num_obj_k_constraints++;
                    }
                    if (bwd_window.size() > 1)
                    {
                        cv->add_clause({b2_amo});
                        num_obj_k_constraints++;
                    }

                    cv->add_clause({b1_amz, b2_amz});
                    num_obj_k_constraints++;

                    node_amz_literals[var_group].push_back({b1_amz, b2_amz});

                    fwd_window.pop_front();
                    bwd_window.push_back(bwd_window.back() + 1);
                }
            }
            node_amz_literals[var_group].push_back({bwd_amz_roots[var_group][number_of_windows - 1]});
        }
    };

    void DuplexEncoder::glue_edge_windows(int node1, int node2)
    {
        assert(node_amz_literals[node1].size() == node_amz_literals[node2].size());
        for (unsigned i = 0; i < node_amz_literals[node1].size(); ++i)
        {
            std::vector<int> node1_amz_clause = node_amz_literals[node1][i];
            std::vector<int> node2_amz_clause = node_amz_literals[node2][i];
            assert(node1_amz_clause.size() == node2_amz_clause.size());
            for (unsigned c = 0; c < node1_amz_clause.size(); ++c)
            {
                for (unsigned d = 0; d < node2_amz_clause.size(); ++d)
                {
                    cv->add_clause({node1_amz_clause[c], node2_amz_clause[d]});
                    num_obj_k_constraints++;
                    num_obj_k_glue_staircase_constraint++;
                }
            }
        }
    };

    BDD_id DuplexEncoder::build_amo(std::deque<unsigned int> vars)
    {
        BDD_id lookup;
        int from = vars.front();
        int to = vars.back();
        std::pair<int, int> from_to_pair(from, to);

        if (bh.lookup_amo(from_to_pair, lookup))
        {
            return lookup;
        }

        BDD new_bdd = BDD(from, to, 1);
        BDD_id true_child, false_child;

        if (vars.size() == 1)
        {
            new_bdd.id = from;
            true_child = 0;
            false_child = 0;
        }
        else
        {
            new_bdd.id = vh->get_new_var();
            num_obj_k_aux_vars++;
            vars.pop_front();
            false_child = build_amo(vars);
            true_child = build_amz(vars);

            cv->add_clause({-1 * from, -1 * new_bdd.id, true_child});
            num_obj_k_constraints++;
            if (vars.size() > 1)
            {
                cv->add_clause({new_bdd.id * -1, false_child});
                num_obj_k_constraints++;
            }
        }

        new_bdd.true_child_id = true_child;
        new_bdd.false_child_id = false_child;

        bh.save_amo(new_bdd);
        return new_bdd.id;
    };

    BDD_id DuplexEncoder::build_amz(std::deque<unsigned int> vars)
    {
        BDD_id lookup;
        int from = vars.front();
        int to = vars.back();
        std::pair<int, int> from_to_pair(from, to);

        if (bh.lookup_amz(from_to_pair, lookup))
        {
            return lookup;
        }

        BDD new_bdd = BDD(from, to, 0);
        BDD_id true_child, false_child;

        if (vars.size() == 1)
        {
            new_bdd.id = -1 * from;
            true_child = 0;  //\bot BDD
            false_child = 0; //\top BDD
        }
        else
        {
            new_bdd.id = vh->get_new_var();
            num_obj_k_aux_vars++;
            true_child = 0; //\bot BDD
            vars.pop_front();
            false_child = build_amz(vars);

            cv->add_clause({-1 * from, -1 * new_bdd.id});
            num_obj_k_constraints++;
            cv->add_clause({from, -1 * new_bdd.id, false_child});
            num_obj_k_constraints++;
            cv->add_clause({from, new_bdd.id, -1 * false_child});
            num_obj_k_constraints++;
        }

        new_bdd.true_child_id = true_child;
        new_bdd.false_child_id = false_child;

        bh.save_amz(new_bdd);
        return new_bdd.id;
    };

    void DuplexEncoder::make_equal_bdds(BDD_id b1, BDD_id b2)
    {
        if (b1 == b2)
            return;
        if (b1 == -1)
        {
            cv->add_clause({b2});
            num_obj_k_constraints++;
            return;
        }

        if (b2 == -1)
        {
            cv->add_clause({b1});
            num_obj_k_constraints++;
            return;
        }

        assert(b1 > 0 && b2 > 0);

        cv->add_clause({-1 * b1, b2});
        num_obj_k_constraints++;
        cv->add_clause({b1, -1 * b2});
        num_obj_k_constraints++;
    };

}
