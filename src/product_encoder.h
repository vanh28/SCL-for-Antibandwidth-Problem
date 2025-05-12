#ifndef PRODUCT_H
#define PRODUCT_H

#include "encoder.h"

namespace SATABP {

class ProductEncoder : public Encoder {
public:
  ProductEncoder(Graph* g, ClauseContainer* cc, VarHandler* vh);
  virtual ~ProductEncoder();

private:
  void do_encode_antibandwidth(unsigned w,std::vector<std::pair<int,int>> const& node_pairs) final;
  int do_vars_size() const final;

  void encode_labelling();
  void encode_pair_amo(int w, int node1, int node2);

  void encode_eo(vec_int_it it_begin, vec_int_it it_end);
  void encode_amo(vec_int_it it_begin, vec_int_it it_end);
  void encode_glued_amo(deq_int_it amo1_begin, deq_int_it amo1_end, deq_int_it amo2_begin, deq_int_it amo2_end);
};

}

#endif
