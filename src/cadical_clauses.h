#ifndef CAD_CONT_H
#define CAD_CONT_H

#include "clause_cont.h"
#include "cadical.hpp"

namespace SATABP {

class CadicalClauseContainer : public ClauseContainer {
public:
  CadicalClauseContainer(VarHandler*, int split_size, CaDiCaL::Solver *solver);
  virtual ~CadicalClauseContainer();

private:
  CaDiCaL::Solver* cad_solver;
  unsigned clause_counter = 0;

  void do_add_clause(const Clause& c) final;
  unsigned do_size() const final;
  void do_print_dimacs() const final;
  void do_print_clauses() const final;
  void do_clear() final;
};

}

#endif
