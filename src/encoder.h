#ifndef ENCODER_H
#define ENCODER_H

#include <vector>
#include <deque>
#include <utility> //pair

#include "clause_cont.h"

namespace SATABP
{

  typedef std::vector<int>::iterator vec_int_it;
  typedef std::deque<int>::iterator deq_int_it;

  class Encoder
  {
  public:
    virtual ~Encoder();

    Encoder(Encoder const &) = delete;
    Encoder &operator=(Encoder const &) = delete;

    std::string symmetry_break_point = "n";

    void encode_antibandwidth(unsigned w, const std::vector<std::pair<int, int>> &node_pairs);

    void print_clauses() const;
    void print_dimacs() const;
    int size() const;
    int vars_size() const;

    ClauseContainer *cv;

  protected:
    Encoder(Graph *g, ClauseContainer *clause_container, VarHandler *var_handler);

    Graph *g;
    VarHandler *vh;

    void encode_symmetry_break();
    void encode_symmetry_break_on_maxnode();
    void encode_symmetry_break_on_minnode();

  private:
    virtual void do_encode_antibandwidth(unsigned w, std::vector<std::pair<int, int>> const &node_pairs) = 0;
    virtual int do_vars_size() const = 0;
  };

}

#endif
