#include "encoder.h"

#include <iostream>
#include <assert.h>
#include <limits>

namespace SATABP {

Encoder::Encoder(Graph* graph, ClauseContainer* clause_container, VarHandler* var_handler) :
    cv(clause_container), g(graph), vh(var_handler) { };

Encoder::~Encoder() {};

void Encoder::encode_antibandwidth(unsigned w, const std::vector<std::pair<int,int>>& node_pairs) {
    if (w < 1 || w > g->n) {
        std::cout << "c Non-valid value of w, nothing to encode." << std::endl;
        return;
    }
	do_encode_antibandwidth(w,node_pairs);
};

void Encoder::encode_symmetry_break() {
    // Negate the second half
    for(unsigned i = g->n; i > g->n-(g->n/2); i--) {
        cv->add_clause({-1*int(i)}); //narrowing, but we already failed if it is a too high unsigned
    }

    // // Negate the first half
    // for(unsigned i = 1; i <= g->n / 2; i++) {
    //     cv->add_clause({-1*int(i)}); //narrowing, but we already failed if it is a too high unsigned
    // }
};

void Encoder::encode_symmetry_break_on_maxnode() {
    unsigned max_node_id = g->find_greatest_outdegree_node();
    
    // Negate the second half
    for(unsigned i = max_node_id*g->n; i > (max_node_id*g->n)-(g->n/2); i--) {
        cv->add_clause({-1*int(i)}); //narrowing, but we already failed if it is a too high unsigned
    }

    // // Negate the first half
    // for(unsigned i = max_node_id * (g->n - 1) + 1; i <= max_node_id * (g->n - 1) + g->n / 2; i++) {
    //     cv->add_clause({-1*int(i)}); //narrowing, but we already failed if it is a too high unsigned
    // }

};

void Encoder::encode_symmetry_break_on_minnode() {
    unsigned max_node_id = g->find_smallest_outdegree_node();

    // Negate the second half
    for(unsigned i = max_node_id*g->n; i > (max_node_id*g->n)-(g->n/2); i--) {
        cv->add_clause({-1*int(i)}); //narrowing, but we already failed if it is a too high unsigned
    }

    // // Negate the first half
    // for(unsigned i = max_node_id * (g->n - 1) + 1; i <= max_node_id * (g->n - 1) + g->n / 2; i++) {
    //     cv->add_clause({-1*int(i)}); //narrowing, but we already failed if it is a too high unsigned
    // }
};


void Encoder::print_clauses() const {
    cv->print_clauses();
};

void Encoder::print_dimacs() const {
    cv->print_dimacs();
};

int Encoder::size() const {
    return cv->size();
};

int Encoder::vars_size() const {
    return do_vars_size();
};

}
