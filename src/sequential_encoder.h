#ifndef SEQ_H
#define SEQ_H

#include "encoder.h"

namespace SATABP {

class SeqEncoder : public Encoder {
public:
  SeqEncoder(Graph* g, ClauseContainer* cc, VarHandler* vh);
  virtual ~SeqEncoder();

private:
  void do_encode_antibandwidth(unsigned w, const std::vector<std::pair<int,int>>& node_pairs) final;
  int do_vars_size() const final;

  void encode_labelling();
  void encode_pair_amo(int w, int node1, int node2);

  void encode_eo(vec_int_it it_begin, vec_int_it it_end);
  void encode_glued_amo(deq_int_it amo1_begin, deq_int_it amo1_end, deq_int_it amo2_begin, deq_int_it amo2_end);
};

}

#endif
