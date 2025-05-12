#include "cadical_clauses.h"

#include <iostream>
#include <assert.h>

namespace SATABP
{

    CadicalClauseContainer::CadicalClauseContainer(VarHandler *v, int split_size, CaDiCaL::Solver *solver)
        : ClauseContainer(v, split_size)
    {
        cad_solver = solver;
    };

    CadicalClauseContainer::~CadicalClauseContainer(){};

    void CadicalClauseContainer::do_add_clause(const Clause &c)
    {
        for (int lit : c)
        {
            cad_solver->add(lit);
        }

        cad_solver->add(0);
        clause_counter += 1;
    };

    unsigned CadicalClauseContainer::do_size() const
    {
        // cad_solver->irredundant() != clause_counter. Here the encoding clause size is interesting.
        return clause_counter;
    };

    void CadicalClauseContainer::do_print_dimacs() const
    {
        std::cout << "c Print formula as dimacs: Not supported function with CaDiCaL.";
    };

    void CadicalClauseContainer::do_print_clauses() const
    {
        std::cout << "c Print formula as dimacs: Not supported function with CaDiCaL.";
    };

    void CadicalClauseContainer::do_clear()
    {
        std::cout << "c Clear clause-set: Not supported function with CaDiCaL.";
    };

}
