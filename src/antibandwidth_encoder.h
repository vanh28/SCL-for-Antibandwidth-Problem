#ifndef ABW_ENCODER_H
#define ABW_ENCODER_H

#include <string>
#include <vector>
#include <unordered_map>

#include "utils.h"

#include "reduced_encoder.h"
#include "sequential_encoder.h"
#include "product_encoder.h"
#include "duplex_encoder.h"
#include "ladder_encoder.h"
#include "ladder_split_encoder.h"

#include "clause_cont.h"
#include "cadical_clauses.h"

namespace SATABP
{

  enum EncoderType
  {
    duplex,
    reduced,
    seq,
    product,
    ladder,
    ladder_split
  };
  enum EncoderStrategy
  {
    from_lb,
    from_ub,
    bin_search,
  };

  class AntibandwidthEncoder
  {
  public:
    AntibandwidthEncoder();
    virtual ~AntibandwidthEncoder();

    EncoderType enc_choice = duplex;
    EncoderStrategy enc_strategy = from_lb;

    bool verbose = true;
    bool check_solution = true;

    bool force_phase = false;
    std::string sat_configuration = "sat";

    int split_limit = 0;
    std::string symmetry_break_point = "n";
    int w_cap = 500;

    bool overwrite_lb = false;
    bool overwrite_ub = false;
    int forced_lb = 0;
    int forced_ub = 0;

    void read_graph(std::string graph_file_name);
    void encode_and_solve_abws();
    void encode_and_print_abw_problem(int w);

  protected:
    Graph *g;
    VarHandler *vh;
    Encoder *enc;
    ClauseContainer *cc;
    CaDiCaL::Solver *solver;

    int SAT_res = 0;

  private:
    void encode_and_solve_abw_problems_from_lb();
    void encode_and_solve_abw_problems_from_ub();
    void encode_and_solve_abw_problems_bin_search();

    void encode_and_solve_abw_problems(int w_from, int w_to, int prev_res, int stop_w);
    bool encode_and_solve_antibandwidth_problem(int w);

    int calculate_sat_solution();
    bool extract_node_labels(std::vector<int> &node_labels);

    void setup_for_solving();
    void cleanup_solving();
    void setup_for_print();
    void cleanup_print();

    void setup_cadical();
    void setup_encoder();
    void lookup_bounds(int &lb, int &ub);
    void setup_bounds(int &w_from, int &w_to);

    static std::unordered_map<std::string, int> abw_LBs;
    static std::unordered_map<std::string, int> abw_UBs;
  };

}

#endif
