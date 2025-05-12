#ifndef REDUCED_H
#define REDUCED_H

#include "encoder.h"

namespace SATABP {

class ReducedEncoder : public Encoder {
public:
  ReducedEncoder(Graph* g, ClauseContainer* cc, VarHandler* vh);
  virtual ~ReducedEncoder();

private:
  void do_encode_antibandwidth(unsigned w, const std::vector<std::pair<int,int>>& node_pairs) final;

  void encode_labelling();
  void encode_pair_amo(int w, int node1, int node2);

  int do_vars_size() const final;

  void encode_eo(vec_int_it it_begin, vec_int_it it_end);
  void encode_glued_first_amo(deq_int_it amo1_begin, deq_int_it amo1_end, deq_int_it amo2_begin, deq_int_it amo2_end);
  void encode_next_window(deq_int_it amo1_begin, deq_int_it amo1_end, deq_int_it amo2_begin, deq_int_it amo2_end, int new_g1, int new_g2);
};

}

#endif
