#include "bdd.h"

#include <assert.h>
#include <iostream>

namespace SATABP {

BDD::BDD(int from, int to, bool ub) : bound(ub), i_from(from), i_to(to) {};
BDD::BDD() : bound(-1), i_from(0), i_to(0) {};

BDDHandler::BDDHandler() {
    amo_bdds = std::unordered_map<std::pair<int,int>,BDD_id,hash_pair>();
    amz_bdds = std::unordered_map<std::pair<int,int>,BDD_id,hash_pair>();
    bdds = std::unordered_map<BDD_id,BDD>();

    BDD false_bdd;
    false_bdd.id = 0;
    false_bdd.i_from = 0;
    false_bdd.i_to = 0;
    bdds[0] = false_bdd;
};


void BDDHandler::save_amo(BDD& new_bdd) {
    std::pair<int,int> p = std::pair<int,int>(new_bdd.i_from,new_bdd.i_to);
    auto pos = amo_bdds.find(p);
    assert(pos == amo_bdds.end());

    amo_bdds[p] = new_bdd.id;
    bdds[new_bdd.id] = new_bdd;
};

void BDDHandler::save_amz(BDD& new_bdd) {
    std::pair<int,int> p = std::pair<int,int>(new_bdd.i_from,new_bdd.i_to);
    auto pos = amz_bdds.find(p);
    assert(pos == amz_bdds.end());

    amz_bdds[p] = new_bdd.id;
    bdds[new_bdd.id] = new_bdd;
};

bool BDDHandler::lookup_amo(const std::pair<int,int>& from_to_pair, BDD_id& id) {
    auto pos = amo_bdds.find(from_to_pair);
    if (pos != amo_bdds.end()) {
        //std::cout << "Found AMO " << pos->second << std::endl;
        id = pos->second;
        return true;
    }

    id = 0;
    return false;
};

bool BDDHandler::lookup_amz(const std::pair<int,int>& from_to_pair, BDD_id& id) {
    auto pos = amz_bdds.find(from_to_pair);
    if (pos != amz_bdds.end()) {
        //std::cout << "Found AMZ " << pos->second << std::endl;
        id = pos->second;
        return true;
    }

    id = 0;
    return false;
};

void BDDHandler::print_node(BDD_id id) {
    BDD bdd = bdds[id];
    std::cout << "id: " << id << "[" << bdd.i_from << " -- " << bdd.i_to << "] <=" << bdd.bound << " ";
    std::cout << "(tc: " << bdd.true_child_id << " fc: " << bdd.false_child_id <<  ")" << std::endl;
};

void BDDHandler::print_bdd(BDD_id id) {
    print_node(id);
    if (bdds[id].true_child_id != -1) print_bdd(bdds[id].true_child_id);
    if (bdds[id].false_child_id != -1) print_bdd(bdds[id].false_child_id);
};

void BDDHandler::print_all_bdds() {
    for (auto& bdd: bdds) {
        print_node(bdd.second.id);
    }
};

}
