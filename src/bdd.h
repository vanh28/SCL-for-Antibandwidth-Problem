#ifndef BDD_H
#define BDD_H

#include <unordered_map>
#include <utility>
#include <functional>
#include <cstdint>
#include <cstddef>

namespace SATABP
{

  typedef int BDD_id;

  struct hash_pair final
  {
    template <class TFirst, class TSecond>
    size_t operator()(const std::pair<TFirst, TSecond> &p) const noexcept
    {
      uintmax_t hash = std::hash<TFirst>{}(p.first);
      hash <<= sizeof(uintmax_t) * 4;
      hash ^= std::hash<TSecond>{}(p.second);
      return std::hash<uintmax_t>{}(hash);
    }
  };

  class BDD
  {
  public:
    BDD();
    BDD(int from, int to, bool bound);

    BDD_id id = 0;
    BDD_id false_child_id = 0;
    BDD_id true_child_id = 0;

    bool bound;
    int i_from;
    int i_to;
  };

  class BDDHandler
  {
  public:
    BDDHandler();
    std::unordered_map<BDD_id, BDD> bdds;

    void save_amo(BDD &new_bdd);
    void save_amz(BDD &new_bdd);

    bool lookup_amo(const std::pair<int, int> &from_to_pair, BDD_id &id);
    bool lookup_amz(const std::pair<int, int> &from_to_pair, BDD_id &id);

    void print_node(BDD_id id);
    void print_bdd(BDD_id id);
    void print_all_bdds();

  private:
    std::unordered_map<std::pair<int, int>, BDD_id, hash_pair> amo_bdds;
    std::unordered_map<std::pair<int, int>, BDD_id, hash_pair> amz_bdds;
  };

}

#endif
