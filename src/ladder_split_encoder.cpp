#include "ladder_split_encoder.h"
#include "math_extension.h"

#include <iostream>
#include <numeric>
#include <algorithm>
#include <assert.h>
#include <cmath>

/*
    Find more details about New Sequential Counter Encoding in
    document named "New Sequential Counter Encoding for Cardinality
    Constraints" - PhD To Van Khanh et al.
*/
namespace SATABP
{
    LadderSplitEncoder::LadderSplitEncoder(Graph *g, ClauseContainer *cc, VarHandler *vh) : Encoder(g, cc, vh)
    {
    }

    LadderSplitEncoder::~LadderSplitEncoder() {}

    int LadderSplitEncoder::get_aux_var(int symbolicAuxVar)
    {
        auto pair = aux_vars.find(symbolicAuxVar);

        if (pair != aux_vars.end())
            return pair->second;

        int new_aux_var = vh->get_new_var();
        aux_vars.insert({symbolicAuxVar, new_aux_var});
        return new_aux_var;
    }

    int LadderSplitEncoder::get_obj_k_aux_var(int first, int last)
    {

        auto pair = obj_k_aux_vars.find({first, last});

        if (pair != obj_k_aux_vars.end())
            return pair->second;

        if (first == last)
            return first;

        int new_obj_k_aux_var = vh->get_new_var();
        obj_k_aux_vars.insert({{first, last}, new_obj_k_aux_var});
        return new_obj_k_aux_var;
    }

    int LadderSplitEncoder::do_vars_size() const
    {
        return vh->size();
    };

    void LadderSplitEncoder::do_encode_antibandwidth(unsigned w, const std::vector<std::pair<int, int>> &node_pairs)
    {
        aux_vars.clear();
        obj_k_aux_vars.clear();

        num_l_v_constraints = 0;
        num_obj_k_constraints = 0;
        num_obj_k_glue_staircase_constraint = 0;

        vertices_aux_var = g->n * g->n;
        labels_aux_var = vertices_aux_var + g->n * g->n;

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

        encode_vertices();
        // encode_labels();
        encode_obj_k(w);

        // Prevent error when build due to unused variables
        (void)node_pairs;
        (void)w;
        std::cout << "c\tLabels and Vertices aux var: " << aux_vars.size() << std::endl;
        std::cout << "c\tLabels and Vertices constraints:  " << num_l_v_constraints << std::endl;
        std::cout << "c\tObj k aux var: " << obj_k_aux_vars.size() << std::endl;
        std::cout << "c\tObj k constraints: " << num_obj_k_constraints << std::endl;
        std::cout << "c\tObj k glue staircase constraints: " << num_obj_k_glue_staircase_constraint << std::endl;
    };

    void LadderSplitEncoder::encode_vertices()
    {
        /*
            Encode that each label can only be assigned to one node.
            Assume there are 30 nodes, the constraints looks like:
            1 + 31 + 61 + ... + 871 = 1
            2 + 32 + 62 + ... + 872 = 1
            ...
            30 + 60 + 90 + ... + 900 = 1
        */
        for (unsigned i = 0; i < g->n; i++)
        {
            std::vector<int> node_vertices_eo(g->n);
            int j = 0;

            std::generate(node_vertices_eo.begin(), node_vertices_eo.end(), [this, &j, i]()
                          { return (j++ * g->n) + i + 1; });

            if (isUsingProductAndSEQ)
            {
                encode_exactly_one_product(node_vertices_eo);
            }
            else
                encode_exactly_one_NSC(node_vertices_eo, vertices_aux_var + i * g->n);
        }
    }

    void LadderSplitEncoder::encode_labels()
    {
        /*
            Encode that each vertex can only take one and only one label.
            Assume there are 30 nodes, the constraints looks like:
            1 + 2 + 3 + ... + 30 = 1
            31 + 32 + 33 + ... + 60 = 1
            ...
            871 + 872 + 873 + ... + 900 = 1
        */
        for (unsigned i = 0; i < g->n; i++)
        {
            std::vector<int> node_labels_eo(g->n);
            std::iota(node_labels_eo.begin(), node_labels_eo.end(), (i * g->n) + 1);

            if (isUsingProductAndSEQ)
            {
                encode_exactly_one_product(node_labels_eo);
            }
            else
                encode_exactly_one_NSC(node_labels_eo, labels_aux_var + i * g->n);
        }
    }

    void LadderSplitEncoder::encode_exactly_one_NSC(std::vector<int> listVars, int auxVar)
    {
        // Exactly one variables in listVars is True
        // Using NSC to encode AMO and ALO, EO = AMO and ALO
        // Auxilian variables starts from auxVar + 1

        int listVarsSize = listVars.size();

        // Constraint 1: Xi -> R(i, 1) for i in [1, n - 1]
        // In CNF: not(Xi) or R(i, 1)
        for (int i = 1; i <= listVarsSize - 1; i++)
        {
            cv->add_clause({-listVars[i - 1], get_aux_var(auxVar + i)});
            num_l_v_constraints++;
        }

        // Constraint 2: R(i-1, 1) -> R(i, 1) for i in [2, n - 1]
        // In CNF: not(R(i-1, 1)) or R(i, 1)
        for (int i = 2; i <= listVarsSize - 1; i++)
        {
            cv->add_clause({-(get_aux_var(auxVar + i - 1)), get_aux_var(auxVar + i)});
            num_l_v_constraints++;
        }

        // Constraint 3: Since k = 1 (Exactly 1 constraint), this constraint is empty and then skipped.

        // Constraint 4: not(Xi) and not(R(i-1, 1)) -> not(R(i, 1)) for i in [2, n - 1]
        // In CNF: Xi or R(i-1, 1) or not (R(i, 1))
        for (int i = 2; i <= listVarsSize - 1; i++)
        {
            cv->add_clause({listVars[i - 1], get_aux_var(auxVar + i - 1), -(get_aux_var(auxVar + i))});
            num_l_v_constraints++;
        }

        // Constraint 5: not(X1) -> not(R(1,1))
        // In CNF: X1 or not(R(1,1))
        cv->add_clause({listVars[0], -(get_aux_var(auxVar + 1))});
        num_l_v_constraints++;

        // Constraint 6: Since k = 1 (Exactly 1 constraint), this constraint is empty and then skipped.

        // Constraint 7: (At Least k) R(n-1, 1) or Xn
        // In CNF: R(n-1, 1) or Xn
        cv->add_clause({get_aux_var(auxVar + listVarsSize - 1), listVars[listVarsSize - 1]});
        num_l_v_constraints++;

        // Constraint 8: (At Most k) Xi -> not(R(i-1,1)) for i in [k + 1, n]
        // In CNF: not(Xi) or not(R(i-1,1))
        for (int i = 2; i <= listVarsSize; i++)
        {
            cv->add_clause({-listVars[i - 1], -(get_aux_var(auxVar + i - 1))});
            num_l_v_constraints++;
        }
    }

    void LadderSplitEncoder::encode_exactly_one_product(const std::vector<int> &vars)
    {
        if (vars.size() < 2)
            return;
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
            int new_var = vh->get_new_var();
            u_vars.push_back(new_var);
            aux_vars.insert({new_var, new_var});
        }
        for (int j = 1; j <= q; ++j)
        {
            int new_var = vh->get_new_var();
            v_vars.push_back(new_var);
            aux_vars.insert({new_var, new_var});
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

            or_clause.push_back(vars[idx]);
        }
        cv->add_clause(or_clause);
        num_l_v_constraints++;

        encode_amo_seq(u_vars);
        encode_amo_seq(v_vars);
    };

    void LadderSplitEncoder::encode_amo_seq(const std::vector<int> &vars)
    {
        if (vars.size() < 2)
            return;

        int prev = vars[0];

        for (unsigned idx = 1; idx < vars.size() - 1; ++idx)
        {
            int curr = vars[idx];
            int next = vh->get_new_var();
            aux_vars.insert({next, next});
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

    void LadderSplitEncoder::encode_obj_k(unsigned w)
    {
        if ((int)w % 2 == 0 ) {
            for (int i = 0; i < (int)g->n; i++)
            {
                encode_stair_even(i, w);
            } 

            for (auto edge : g->edges)
            {
                glue_stair_even(edge.first - 1, edge.second - 1, w);
            }
        } else {
            for (int i = 0; i < (int)g->n; i++)
            {
                encode_stair_odds(i, w);
            } 

            for (auto edge : g->edges)
            {
                glue_stair_odds(edge.first - 1, edge.second - 1, w);
            }
        }
    }

    void LadderSplitEncoder::encode_stair_even(int stair, unsigned w)
    {
        if (is_debug_mode)
            std::cout << "Encode stair " << stair << " with width " << w << std::endl;
        int first_stair_w, second_stair_w;
        first_stair_w = (int)w / 2;
        second_stair_w = (int)w - first_stair_w;

        int result = g->n / first_stair_w;
        if (g->n % first_stair_w != 0) {
            result++;
        }

        for (int gw = 0; gw < result; gw++)
        {
            if (is_debug_mode)
                std::cout << "Encode 1 window " << gw << std::endl;
            encode_full_window(gw, stair, first_stair_w, g->n);
        }
        

        for (int gw = 0; gw < ceil((float)(g->n/first_stair_w)); gw++)
        {
            if (is_debug_mode)
                std::cout << "Encode 1 window " << gw << std::endl;
            glue_full_window(gw, stair, first_stair_w, g->n);
        }
        
        int staircases_lines = (int)g->n - (int)w + 1;
        int window_first = first_stair_w;
        int window_second = second_stair_w;
        int vertice_start = stair * (int)g->n;
        for (int i = 1 ; i <= staircases_lines ;i++) {
            int first_start = vertice_start + i;
            int first_end = vertice_start + i + window_first - 1;
            int first_second_start = first_end + 1;
            int first_second_end = first_second_start + (first_stair_w - window_first) - 1;
            int second_start = vertice_start + i + first_stair_w;
            int second_end = second_start + window_second - 1;
            int second_second_start = second_end + 1;
            int second_second_end = second_second_start + (second_stair_w - window_second) - 1;
        

            if (window_first == first_stair_w && window_second == second_stair_w) {
                cv->add_clause({-get_obj_k_aux_var(first_start, first_end), -get_obj_k_aux_var(second_start, second_end)});
            } else if (window_first == first_stair_w && window_second != second_stair_w) {
                cv->add_clause({-get_obj_k_aux_var(first_start, first_end), -get_obj_k_aux_var(second_start, second_end)});
                cv->add_clause({-get_obj_k_aux_var(first_start, first_end), -get_obj_k_aux_var(second_second_start, second_second_end)});
            } else if (window_first != first_stair_w && window_second == second_stair_w) {
                cv->add_clause({-get_obj_k_aux_var(first_start, first_end), -get_obj_k_aux_var(second_start, second_end)});
                cv->add_clause({-get_obj_k_aux_var(first_second_start, first_second_end), -get_obj_k_aux_var(second_start, second_end)});
            } else {
                cv->add_clause({-get_obj_k_aux_var(first_start, first_end), -get_obj_k_aux_var(second_start, second_end)});
                cv->add_clause({-get_obj_k_aux_var(first_start, first_end), -get_obj_k_aux_var(second_second_start, second_second_end)});
                cv->add_clause({-get_obj_k_aux_var(first_second_start, first_second_end), -get_obj_k_aux_var(second_start, second_end)});
                cv->add_clause({-get_obj_k_aux_var(first_second_start, first_second_end), -get_obj_k_aux_var(second_second_start, second_second_end)});
            }

            window_first--;
            window_second--;
            if (window_first == 0) {
                window_first = first_stair_w;
            }
            if (window_second == 0) {
                window_second = second_stair_w;
            }
        }

        int new_width = (int)w / 2;
        std::vector<std::pair<int, int>> windows = {};
        int number_windows = ceil((float)g->n / new_width);

        for (int i = 0; i < number_windows; i++)
        {
            int stair_anchor = stair * (int)g->n;
            int window_anchor = i * (int)new_width;
            if (window_anchor + new_width > (int)g->n)
                windows.push_back({stair_anchor + window_anchor + 1, stair_anchor + g->n});
            else
                windows.push_back({stair_anchor + window_anchor + 1, stair_anchor + window_anchor + new_width});
        }

        std::vector<int> alo_clause = {};
        for (int i = 0; i < number_windows; i++)
        {
            int first_window_aux_var = get_obj_k_aux_var(windows[i].first, windows[i].second);
            alo_clause.push_back(first_window_aux_var);
            for (int j = i + 1; j < number_windows; j++)
            {
                int second_window_aux_var = get_obj_k_aux_var(windows[j].first, windows[j].second);
                cv->add_clause({-first_window_aux_var, -second_window_aux_var});
                num_l_v_constraints++;
            }
        }
        cv->add_clause(alo_clause);
        num_l_v_constraints++;
    }

    void LadderSplitEncoder::encode_stair_odds(int stair, unsigned w)
    {
        if (is_debug_mode)
            std::cout << "Encode stair " << stair << " with width " << w << std::endl;
        
        //7/3 g->n - w/2;
        for (int gw = 0; gw < ceil((float)g->n / w); gw++)
        {
            if (is_debug_mode)
                std::cout << "Encode window " << gw << std::endl;
            encode_window_odds(gw, stair, w);
        }

        for (int gw = 0; gw < ceil((float)g->n / w) - 1; gw++)
        {
            if (is_debug_mode)
                std::cout << "Glue window " << gw << " with window " << gw + 1 << std::endl;
            glue_window_odds(gw, stair, w);
        }

        std::vector<std::pair<int, int>> windows = {};
        int number_windows = ceil((float)g->n / w);

        for (int i = 0; i < number_windows; i++)
        {
            int stair_anchor = stair * (int)g->n;
            int window_anchor = i * (int)w;
            if (window_anchor + w > g->n)
                windows.push_back({stair_anchor + window_anchor + 1, stair_anchor + g->n});
            else
                windows.push_back({stair_anchor + window_anchor + 1, stair_anchor + window_anchor + w});
        }

        std::vector<int> alo_clause = {};
        for (int i = 0; i < number_windows; i++)
        {
            int first_window_aux_var = get_obj_k_aux_var(windows[i].first, windows[i].second);
            alo_clause.push_back(first_window_aux_var);
            for (int j = i + 1; j < number_windows; j++)
            {
                int second_window_aux_var = get_obj_k_aux_var(windows[j].first, windows[j].second);
                cv->add_clause({-first_window_aux_var, -second_window_aux_var});
                num_l_v_constraints++;
            }
        }
        cv->add_clause(alo_clause);
        num_l_v_constraints++;
    }

    void LadderSplitEncoder::encode_window_odds(int window, int stair, unsigned w)
    {
        if (window == 0)
        {
            // Encode the first window, which only have lower part
            int lastVar = stair * (int)g->n + window * (int)w + w;

            for (int i = w - 1; i >= 1; i--)
            {
                int var = stair * (int)g->n + window * (int)w + i;
                cv->add_clause({-var, get_obj_k_aux_var(var, lastVar)});
                num_obj_k_constraints++;
            }

            for (int i = w; i >= 2; i--)
            {
                int var = stair * (int)g->n + window * (int)w + i;
                cv->add_clause({-get_obj_k_aux_var(var, lastVar), get_obj_k_aux_var(var - 1, lastVar)});
                num_obj_k_constraints++;
            }

            for (int i = 1; i < (int)w; i++)
            {
                int var = stair * (int)g->n + window * (int)w + i;
                int main = get_obj_k_aux_var(var, lastVar);
                int sub = get_obj_k_aux_var(var + 1, lastVar);
                cv->add_clause({var, sub, -main});
                num_obj_k_constraints++;
            }

            for (int i = 1; i < (int)w; i++)
            {
                int var = stair * (int)g->n + window * (int)w + i;
                cv->add_clause({-var, -get_obj_k_aux_var(var + 1, lastVar)});
                num_obj_k_constraints++;
            }
        }
        else if (window == ceil((float)g->n / w) - 1)
        {
            // Encode the last window, which only have upper part and may have width lower than w
            int firstVar = stair * (int)g->n + window * (int)w + 1;

            if ((window + 1) * w > g->n)
            {
                int real_w = g->n % w;
                // Upper part
                for (int i = 2; i <= real_w; i++)
                {
                    int reverse_var = stair * (int)g->n + window * (int)w + i;
                    cv->add_clause({-reverse_var, get_obj_k_aux_var(firstVar, reverse_var)});
                    num_obj_k_constraints++;
                }

                for (int i = real_w - 1; i > 0; i--)
                {
                    int reverse_var = stair * (int)g->n + window * (int)w + real_w - i;
                    cv->add_clause({-get_obj_k_aux_var(firstVar, reverse_var), get_obj_k_aux_var(firstVar, reverse_var + 1)});
                    num_obj_k_constraints++;
                }

                for (int i = 0; i < (int)real_w - 1; i++)
                {
                    int var = stair * (int)g->n + window * (int)w + real_w - i;
                    int main = get_obj_k_aux_var(firstVar, var);
                    int sub = get_obj_k_aux_var(firstVar, var - 1);
                    cv->add_clause({sub, var, -main});
                    num_obj_k_constraints++;
                }

                for (int i = real_w; i > 1; i--)
                {
                    int reverse_var = stair * (int)g->n + window * (int)w + i;
                    cv->add_clause({-reverse_var, -get_obj_k_aux_var(firstVar, reverse_var - 1)});
                    num_obj_k_constraints++;
                }
            }
            else
            {
                // Upper part
                for (int i = 2; i <= (int)w; i++)
                {
                    int reverse_var = stair * (int)g->n + window * (int)w + i;
                    cv->add_clause({-reverse_var, get_obj_k_aux_var(firstVar, reverse_var)});
                    num_obj_k_constraints++;
                }

                for (int i = w - 1; i >= 1; i--)
                {
                    int reverse_var = stair * (int)g->n + window * (int)w + w - i;
                    cv->add_clause({-get_obj_k_aux_var(firstVar, reverse_var), get_obj_k_aux_var(firstVar, reverse_var + 1)});
                    num_obj_k_constraints++;
                }

                for (int i = 0; i < (int)w - 1; i++)
                {
                    int var = stair * (int)g->n + window * (int)w + w - i;
                    int main = get_obj_k_aux_var(firstVar, var);
                    int sub = get_obj_k_aux_var(firstVar, var - 1);
                    cv->add_clause({sub, var, -main});
                    num_obj_k_constraints++;
                }

                for (int i = (int)w; i > 1; i--)
                {
                    int reverse_var = stair * (int)g->n + window * (int)w + i;
                    cv->add_clause({-reverse_var, -get_obj_k_aux_var(firstVar, reverse_var - 1)});
                    num_obj_k_constraints++;
                }
            }
        }
        else
        {
            // Encode the middle windows, which have both upper and lower path, and always have width w

            // Upper part
            int firstVar = stair * (int)g->n + window * (int)w + 1;
            for (int i = 2; i <= (int)w; i++)
            {
                int reverse_var = stair * (int)g->n + window * (int)w + i;
                cv->add_clause({-reverse_var, get_obj_k_aux_var(firstVar, reverse_var)});
                num_obj_k_constraints++;
            }

            for (int i = w - 1; i >= 1; i--)
            {
                int reverse_var = stair * (int)g->n + window * (int)w + w - i;
                cv->add_clause({-get_obj_k_aux_var(firstVar, reverse_var), get_obj_k_aux_var(firstVar, reverse_var + 1)});
                num_obj_k_constraints++;
            }

            for (int i = 0; i < (int)w - 1; i++)
            {
                int var = stair * (int)g->n + window * (int)w + w - i;
                int main = get_obj_k_aux_var(firstVar, var);
                int sub = get_obj_k_aux_var(firstVar, var - 1);
                cv->add_clause({sub, var, -main});
                num_obj_k_constraints++;
            }

            for (int i = (int)w; i > 1; i--)
            {
                int reverse_var = stair * (int)g->n + window * (int)w + i;
                cv->add_clause({-reverse_var, -get_obj_k_aux_var(firstVar, reverse_var - 1)});
                num_obj_k_constraints++;
            }

            // Lower part
            int lastVar = stair * (int)g->n + window * (int)w + w;
            for (int i = w - 1; i >= 1; i--)
            {
                int var = stair * (int)g->n + window * (int)w + i;
                cv->add_clause({-var, get_obj_k_aux_var(var, lastVar)});
                num_obj_k_constraints++;
            }

            for (int i = w; i >= 2; i--)
            {
                int var = stair * (int)g->n + window * (int)w + i;
                cv->add_clause({-get_obj_k_aux_var(var, lastVar), get_obj_k_aux_var(var - 1, lastVar)});
                num_obj_k_constraints++;
            }

            for (int i = 1; i < (int)w; i++)
            {
                int var = stair * (int)g->n + window * (int)w + i;
                int main = get_obj_k_aux_var(var, lastVar);
                int sub = get_obj_k_aux_var(var + 1, lastVar);
                cv->add_clause({var, sub, -main});
                num_obj_k_constraints++;
            }

            // Can be disable
            // for (int i = 1; i < (int)w; i++)
            // {
            //     int var = stair * (int)g->n + window * (int)w + i;
            //     cv->add_clause({-var, -GetEncodedAuxVar(auxStartVarLP + var + 1)});
            //     num_obj_k_constraints++;
            // }
        }
    }

    /*
     * Glue adjacent windows with each other.
     * Using lower part of the previous window and upper part of the next window
     * as anchor points to glue.
     */

    
    void LadderSplitEncoder::glue_window_odds(int window, int stair, unsigned w)
    {
        /*  The stair look like this:
         *      Window 1        Window 2        Window 3        Window 4
         *      1   2   3   |               |               |
         *          2   3   |   4           |               |
         *              3   |   4   5       |               |
         *                  |   4   5   6   |               |
         *                  |       5   6   |   7           |
         *                  |           6   |   7   8       |
         *                  |               |   7   8   9   |
         *                  |               |       8   9   |   10
         *                  |               |           9   |   10  11
         *
         * If the next window has width of w, then we only encode w - 1 register bits (because
         * NSC only define w - 1 register bits), else we encode using number of register bit
         * equal to width of the next window.
         */
        if ((window + 2) * w > g->n)
        {
            int real_w = g->n % w;
            for (int i = 1; i <= real_w; i++)
            {
                int first_reverse_var = stair * (int)g->n + (window + 1) * (int)w + 1;
                int last_var = stair * (int)g->n + window * (int)w + w;

                int reverse_var = stair * (int)g->n + (window + 1) * (int)w + i;
                int var = stair * (int)g->n + window * (int)w + i + 1;

                cv->add_clause({-get_obj_k_aux_var(var, last_var), -get_obj_k_aux_var(first_reverse_var, reverse_var)});
                num_obj_k_constraints++;
            }
        }
        else
        {
            for (int i = 1; i < (int)w; i++)
            {
                int first_reverse_var = stair * (int)g->n + (window + 1) * (int)w + 1;
                int last_var = stair * (int)g->n + window * (int)w + w;

                int reverse_var = stair * (int)g->n + (window + 1) * (int)w + i;
                int var = stair * (int)g->n + window * (int)w + i + 1;

                cv->add_clause({-get_obj_k_aux_var(var, last_var), -get_obj_k_aux_var(first_reverse_var, reverse_var)});
                num_obj_k_constraints++;
            }
        }
    }
    /*
     * Encode each window separately.
     * The first window only has lower part.
     * The last window only has upper part.
     * Other windows have both upper part and lower part.
     */

    void LadderSplitEncoder::glue_full_window(int window,int stair, unsigned w, int n)
    {
        /*  The stair look like this:
         *      Window 1        Window 2        Window 3        Window 4
         *      1   2   3   |               |               |
         *          2   3   |   4           |               |
         *              3   |   4   5       |               |
         *                  |   4   5   6   |               |
         *                  |       5   6   |   7           |
         *                  |           6   |   7   8       |
         *                  |               |   7   8   9   |
         *                  |               |       8   9   |   10
         *                  |               |           9   |   10  11
         *
         * If the next window has width of w, then we only encode w - 1 register bits (because
         * NSC only define w - 1 register bits), else we encode using number of register bit
         * equal to width of the next window.
         */
        if ((window + 2) * (int)w > n)
        {
            int real_w = n % w;
            for (int i = 1; i <= real_w; i++)
            {
                int first_reverse_var = stair * (int)n + (window + 1) * (int)w + 1;
                int last_var =  stair *(int)n + window * (int)w + w;

                int reverse_var = stair *(int)n + (window + 1) * (int)w + i;
                int var =  stair *(int)n + window * (int)w + i + 1;

                cv->add_clause({-get_obj_k_aux_var(var, last_var), -get_obj_k_aux_var(first_reverse_var, reverse_var)});
               
            }
        }
        else
        {
            for (int i = 1; i < (int)w; i++)
            {
                int first_reverse_var = stair * (int)n + (window + 1) * (int)w + 1;
                int last_var = stair * (int)n + window * (int)w + w;

                int reverse_var = stair *(int)n + (window + 1) * (int)w + i;
                int var =  stair *(int)n + window * (int)w + i + 1;

                cv->add_clause({-get_obj_k_aux_var(var, last_var), -get_obj_k_aux_var(first_reverse_var, reverse_var)});
            }
        }
    }

    void LadderSplitEncoder::encode_full_window(int window, int stair, unsigned w, int n)
    {
        if (window == 0)
        {
            // Encode the first window, which only have lower part
            int lastVar = stair * (int)n + window * (int)w + w;

            for (int i = w - 1; i >= 1; i--)
            {
                int var = stair * (int)n + window * (int)w + i;
                cv->add_clause({-var, get_obj_k_aux_var(var, lastVar)});
                
            }

            for (int i = w; i >= 2; i--)
            {
                int var = stair * (int)n + window * (int)w + i;
                cv->add_clause({-get_obj_k_aux_var(var, lastVar), get_obj_k_aux_var(var - 1, lastVar)});
                
            }

            for (int i = 1; i < (int)w; i++)
            {
                int var = stair * (int)n + window * (int)w + i;
                int main = get_obj_k_aux_var(var, lastVar);
                int sub = get_obj_k_aux_var(var + 1, lastVar);
                cv->add_clause({var, sub, -main});
                
            }

            for (int i = 1; i < (int)w; i++)
            {
                int var = stair * (int)n + window * (int)w + i;
                cv->add_clause({-var, -get_obj_k_aux_var(var + 1, lastVar)});
                
            }
        }
        else if (window == ceil((float)n / w) - 1)
        {
            // Encode the last window, which only have upper part and may have width lower than w
            int firstVar = stair * (int)n + window * (int)w + 1;

            if ((window + 1) * (int)w > n)
            {
                int real_w = n % w;
                // Upper part
                for (int i = 2; i <= real_w; i++)
                {
                    int reverse_var = stair * (int)n + window * (int)w + i;
                    cv->add_clause({-reverse_var, get_obj_k_aux_var(firstVar, reverse_var)});
                    
                }

                for (int i = real_w - 1; i > 0; i--)
                {
                    int reverse_var = stair * (int)n + window * (int)w + real_w - i;
                    cv->add_clause({-get_obj_k_aux_var(firstVar, reverse_var), get_obj_k_aux_var(firstVar, reverse_var + 1)});
                    
                }

                for (int i = 0; i < (int)real_w - 1; i++)
                {
                    int var = stair * (int)n + window * (int)w + real_w - i;
                    int main = get_obj_k_aux_var(firstVar, var);
                    int sub = get_obj_k_aux_var(firstVar, var - 1);
                    cv->add_clause({sub, var, -main});
                    
                }

                for (int i = real_w; i > 1; i--)
                {
                    int reverse_var = stair * (int)n + window * (int)w + i;
                    cv->add_clause({-reverse_var, -get_obj_k_aux_var(firstVar, reverse_var - 1)});
                    
                }
            }
            else
            {
                // Upper part
                for (int i = 2; i <= (int)w; i++)
                {
                    int reverse_var = stair * (int)n + window * (int)w + i;
                    cv->add_clause({-reverse_var, get_obj_k_aux_var(firstVar, reverse_var)});
                    
                }

                for (int i = w - 1; i >= 1; i--)
                {
                    int reverse_var = stair * (int)n + window * (int)w + w - i;
                    cv->add_clause({-get_obj_k_aux_var(firstVar, reverse_var), get_obj_k_aux_var(firstVar, reverse_var + 1)});
                    
                }

                for (int i = 0; i < (int)w - 1; i++)
                {
                    int var = stair * (int)n + window * (int)w + w - i;
                    int main = get_obj_k_aux_var(firstVar, var);
                    int sub = get_obj_k_aux_var(firstVar, var - 1);
                    cv->add_clause({sub, var, -main});
                    
                }

                for (int i = (int)w; i > 1; i--)
                {
                    int reverse_var = stair * (int)n + window * (int)w + i;
                    cv->add_clause({-reverse_var, -get_obj_k_aux_var(firstVar, reverse_var - 1)});
                    
                }
            }
        }
        else
        {
            // Encode the middle windows, which have both upper and lower path, and always have width w

            // Upper part
            int firstVar = stair * (int)n + window * (int)w + 1;
            for (int i = 2; i <= (int)w; i++)
            {
                int reverse_var = stair * (int)n + window * (int)w + i;
                cv->add_clause({-reverse_var, get_obj_k_aux_var(firstVar, reverse_var)});
                
            }

            for (int i = w - 1; i >= 1; i--)
            {
                int reverse_var = stair * (int)n + window * (int)w + w - i;
                cv->add_clause({-get_obj_k_aux_var(firstVar, reverse_var), get_obj_k_aux_var(firstVar, reverse_var + 1)});
                
            }

            for (int i = 0; i < (int)w - 1; i++)
            {
                int var = stair * (int)n + window * (int)w + w - i;
                int main = get_obj_k_aux_var(firstVar, var);
                int sub = get_obj_k_aux_var(firstVar, var - 1);
                cv->add_clause({sub, var, -main});
                
            }

            for (int i = (int)w; i > 1; i--)
            {
                int reverse_var = stair * (int)n + window * (int)w + i;
                cv->add_clause({-reverse_var, -get_obj_k_aux_var(firstVar, reverse_var - 1)});
                
            }

            // Lower part
            int lastVar = stair * (int)n + window * (int)w + w;
            for (int i = w - 1; i >= 1; i--)
            {
                int var = stair * (int)n + window * (int)w + i;
                cv->add_clause({-var, get_obj_k_aux_var(var, lastVar)});
                
            }

            for (int i = w; i >= 2; i--)
            {
                int var = stair * (int)n + window * (int)w + i;
                cv->add_clause({-get_obj_k_aux_var(var, lastVar), get_obj_k_aux_var(var - 1, lastVar)});
                
            }

            for (int i = 1; i < (int)w; i++)
            {
                int var = stair * (int)n + window * (int)w + i;
                int main = get_obj_k_aux_var(var, lastVar);
                int sub = get_obj_k_aux_var(var + 1, lastVar);
                cv->add_clause({var, sub, -main});
                
            }

            // Can be disable
            // for (int i = 1; i < (int)w; i++)
            // {
            //     int var = stair * (int)n + window * (int)w + i;
            //     cc->add_clause({-var, -GetEncodedAuxVar(auxStartVarLP + var + 1)});
            //     
            // }
        }
    }

    

    void LadderSplitEncoder::glue_stair_even(int stair1, int stair2, unsigned w)
    {
        if (is_debug_mode)
            std::cout << "Glue stair " << stair1 << " with stair " << stair2 << std::endl;
        int number_step = g->n - w + 1;
        int first_stair_w = (int)w / 2;
        int second_stair_w = (int)w - first_stair_w;
        int vertice_start_stair1 = stair1 * (int)g->n;
        int vertice_start_stair2 = stair2 * (int)g->n;
        int window_first = first_stair_w;
        int window_second = second_stair_w;
        for (int i = 1; i <= number_step; i++)
        {
            int first_start_stair1 = vertice_start_stair1 + i;
            int first_end_stair1 = vertice_start_stair1 + i + window_first - 1;
            int first_second_start_stair1 = first_end_stair1 + 1;
            int first_second_end_stair1 = first_second_start_stair1 + (first_stair_w - window_first) - 1;
            int second_start_stair1 = vertice_start_stair1 + i + first_stair_w;
            int second_end_stair1 = second_start_stair1 + window_second - 1;
            int second_second_start_stair1 = second_end_stair1 + 1;
            int second_second_end_stair1 = second_second_start_stair1 + (second_stair_w - window_second) - 1;

            int first_start_stair2 = vertice_start_stair2 + i;
            int first_end_stair2 = vertice_start_stair2 + i + window_first - 1;
            int first_second_start_stair2 = first_end_stair2 + 1;
            int first_second_end_stair2 = first_second_start_stair2 + (first_stair_w - window_first) - 1;
            int second_start_stair2 = vertice_start_stair2 + i + first_stair_w;
            int second_end_stair2 = second_start_stair2 + window_second - 1;
            int second_second_start_stair2 = second_end_stair2 + 1;
            int second_second_end_stair2 = second_second_start_stair2 + (second_stair_w - window_second) - 1;

            if (window_first == first_stair_w && window_second == second_stair_w) {
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(first_start_stair2, first_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(second_start_stair2, second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(first_start_stair2, first_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(second_start_stair2, second_end_stair2)});
            } else if (window_first == first_stair_w && window_second != second_stair_w) {
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(first_start_stair2, first_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(second_start_stair2, second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(second_second_start_stair2, second_second_end_stair2)});

                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(first_start_stair2, first_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(second_start_stair2, second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(second_second_start_stair2, second_second_end_stair2)});

                cv->add_clause({-get_obj_k_aux_var(second_second_start_stair1, second_second_end_stair1), -get_obj_k_aux_var(first_start_stair2, first_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_second_start_stair1, second_second_end_stair1), -get_obj_k_aux_var(second_start_stair2, second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_second_start_stair1, second_second_end_stair1), -get_obj_k_aux_var(second_second_start_stair2, second_second_end_stair2)});
            } else if (window_first != first_stair_w && window_second == second_stair_w) {
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(first_second_start_stair2, first_second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(second_start_stair2, second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(second_second_start_stair2, second_second_end_stair2)});

                cv->add_clause({-get_obj_k_aux_var(first_second_start_stair1, first_second_end_stair1), -get_obj_k_aux_var(first_start_stair2, first_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_second_start_stair1, first_second_end_stair1), -get_obj_k_aux_var(second_start_stair2, second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_second_start_stair1, first_second_end_stair1), -get_obj_k_aux_var(second_second_start_stair2, second_second_end_stair2)});

                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(first_start_stair2, first_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(first_second_start_stair2, first_second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(second_second_start_stair2, second_second_end_stair2)});
            } else {
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(first_start_stair2, first_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(first_second_start_stair2, first_second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(second_start_stair2, second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_start_stair1, first_end_stair1), -get_obj_k_aux_var(second_second_start_stair2, second_second_end_stair2)});

                cv->add_clause({-get_obj_k_aux_var(first_second_start_stair1, first_second_end_stair1), -get_obj_k_aux_var(first_start_stair2, first_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_second_start_stair1, first_second_end_stair1), -get_obj_k_aux_var(first_second_start_stair2, first_second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_second_start_stair1, first_second_end_stair1), -get_obj_k_aux_var(second_start_stair2, second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(first_second_start_stair1, first_second_end_stair1), -get_obj_k_aux_var(second_second_start_stair2, second_second_end_stair2)});

                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(first_start_stair2, first_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(first_second_start_stair2, first_second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(second_start_stair2, second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_start_stair1, second_end_stair1), -get_obj_k_aux_var(second_second_start_stair2, second_second_end_stair2)});

                cv->add_clause({-get_obj_k_aux_var(second_second_start_stair1, second_second_end_stair1), -get_obj_k_aux_var(first_start_stair2, first_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_second_start_stair1, second_second_end_stair1), -get_obj_k_aux_var(first_second_start_stair2, first_second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_second_start_stair1, second_second_end_stair1), -get_obj_k_aux_var(second_start_stair2, second_end_stair2)});
                cv->add_clause({-get_obj_k_aux_var(second_second_start_stair1, second_second_end_stair1), -get_obj_k_aux_var(second_second_start_stair2, second_second_end_stair2)});  
            }

            window_first--;
            window_second--;
            if (window_first == 0) {
                window_first = first_stair_w;
            }
            if (window_second == 0) {
                window_second = second_stair_w;
            }
        }
    }

    void LadderSplitEncoder::glue_stair_odds(int stair1, int stair2, unsigned w)
    {
        if (is_debug_mode)
            std::cout << "Glue stair " << stair1 << " with stair " << stair2 << std::endl;
        int number_step = g->n - w + 1;
        for (int i = 0; i < number_step; i++)
        {
            int mod = i % w;
            int subset = i / w;
            if (mod == 0)
            {
                int firstVar = get_obj_k_aux_var(stair1 * g->n + subset * w + 1, stair1 * g->n + subset * w + w);
                int secondVar = get_obj_k_aux_var(stair2 * g->n + subset * w + 1, stair2 * g->n + subset * w + w);
                cv->add_clause({-firstVar, -secondVar});
                num_obj_k_constraints++;
                num_obj_k_glue_staircase_constraint++;
            }
            else
            {
                int firstVar = get_obj_k_aux_var(stair1 * g->n + subset * w + 1 + mod, stair1 * g->n + subset * w + w);
                int secondVar = get_obj_k_aux_var(stair1 * g->n + subset * w + w + 1, stair1 * g->n + subset * w + w + mod);
                int thirdVar = get_obj_k_aux_var(stair2 * g->n + subset * w + 1 + mod, stair2 * g->n + subset * w + w);
                int forthVar = get_obj_k_aux_var(stair2 * g->n + subset * w + w + 1, stair2 * g->n + subset * w + w + mod);
                cv->add_clause({-firstVar, -thirdVar});
                num_obj_k_constraints++;
                num_obj_k_glue_staircase_constraint++;
                cv->add_clause({-firstVar, -forthVar});
                num_obj_k_constraints++;
                num_obj_k_glue_staircase_constraint++;
                cv->add_clause({-secondVar, -thirdVar});
                num_obj_k_constraints++;
                num_obj_k_glue_staircase_constraint++;
                cv->add_clause({-secondVar, -forthVar});
                num_obj_k_constraints++;
                num_obj_k_glue_staircase_constraint++;
            }
        }
    }

}
