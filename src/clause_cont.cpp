#include "clause_cont.h"
#include <iostream>
#include <assert.h>

namespace SATABP {

ClauseContainer::ClauseContainer(VarHandler* v,unsigned split_limit) : vh(v), split_size(split_limit) {
    if (split_limit == 0) do_split = false;
    else do_split = true;
};

ClauseContainer::~ClauseContainer() {};

void ClauseContainer::add_clause(const Clause& c) {
    if (!do_split) {
        do_add_clause(c);
    } else {
        Clause long_clause = c;
        while (long_clause.size() > split_size) {
            int split_var = vh->get_new_var();

            Clause chunk(long_clause.begin(), long_clause.begin() + split_size);
            chunk.push_back(split_var);
            do_add_clause(chunk);

            Clause rest = {-1*split_var};
            rest.insert(rest.end(), long_clause.begin() + split_size , long_clause.end());
            long_clause = rest;
        }
        do_add_clause(long_clause);
    }
};

void ClauseContainer::print_clauses() const {
    do_print_clauses();
};

void ClauseContainer::print_dimacs() const {
    do_print_dimacs();
};

unsigned ClauseContainer::size() const {
    return do_size();
};

ClauseVector::ClauseVector(VarHandler* v, int split_size)
    : ClauseContainer(v,split_size) {
    clause_list = std::vector<Clause>();
};

ClauseVector::~ClauseVector() {};

void ClauseVector::do_add_clause(const Clause& c) {
  clause_list.push_back(c);
};

unsigned ClauseVector::do_size() const {
  return clause_list.size();
};

void ClauseVector::do_print_dimacs() const {
    std::cout << "p cnf " << vh->size() << " " << size() << std::endl;
    for(auto const& c : clause_list) {
        for (auto const& l : c) {
            std::cout << l << " ";
        }
        std::cout << "0" << std::endl;
    }
};

void ClauseVector::do_print_clauses() const {
    for(auto const& c : clause_list) {
        for (auto const& l : c) {
            std::cout << l << " ";
        }
        std::cout << "0" << std::endl;
    }
};

void ClauseVector::do_clear() {
    clause_list.clear();
};

}
