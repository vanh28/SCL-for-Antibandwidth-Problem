#ifndef DUPLEX_H
#define DUPLEX_H

#include "encoder.h"
#include "bdd.h"

#include <deque>
#include <unordered_map>

namespace SATABP
{

  class DuplexEncoder : public Encoder
  {
  public:
    DuplexEncoder(Graph *g, ClauseContainer *cc, VarHandler *vh);
    virtual ~DuplexEncoder();

  private:
    BDDHandler bh;

    std::unordered_map<int, std::vector<int>> fwd_amo_roots;
    std::unordered_map<int, std::vector<int>> bwd_amo_roots;
    std::unordered_map<int, std::vector<int>> fwd_amz_roots;
    std::unordered_map<int, std::vector<int>> bwd_amz_roots;

    std::unordered_map<int, std::vector<std::vector<int>>> node_amz_literals;
    unsigned number_of_windows;
    unsigned last_window_w;

    // Number of LABELS and VERTICES's aux vars and constraints
    int num_l_v_constraints = 0;
    int num_l_v_aux_vars = 0;
    // Number of OBJ-K's vars and constraints
    int num_obj_k_constraints = 0;
    int num_obj_k_aux_vars = 0;
    int num_obj_k_glue_staircase_constraint = 0;

    void init_members();

    void do_encode_antibandwidth(unsigned w, const std::vector<std::pair<int, int>> &node_pairs) final;

    int do_vars_size() const final;

    void construct_window_bdds(int w);
    void glue_window_bdds();

    void glue_edge_windows(int node1, int node2);
    void make_equal_bdds(BDD_id b1, BDD_id b2);
    void encode_column_eo();
    void seq_encode_column_eo();

    void product_encode_eo(const std::vector<int> &vars);
    void product_encode_amo(const std::vector<int> &vars);
    void seq_encode_amo(const std::vector<int> &vars);

    // Not & on purpose!
    BDD_id build_amo(std::deque<unsigned int> vars);
    BDD_id build_amz(std::deque<unsigned int> vars);
  };

}

#endif
