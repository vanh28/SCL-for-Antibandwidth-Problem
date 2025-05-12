#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <utility>
#include <string>

namespace SATABP {

class Graph {
public:
  unsigned n;
  unsigned number_of_edges;
  std::string graph_name;
  std::vector<std::pair<int,int>> edges;

  explicit Graph(std::string file_name);
  void print_stat() const;
  int calculate_antibandwidth(const std::vector<int>& node_labels) const;
  int calculate_bandwidth(const std::vector<int>& node_labels) const;

  unsigned find_greatest_outdegree_node() const;
  unsigned find_smallest_outdegree_node() const;

  void filename(std::string& path);
};

class VarHandler {
public:
  VarHandler(int start_id, int input_size);
  int get_new_var();
  int last_var() const;
  int size() const;
private:
  int first_assigned_id;
  int next_to_assign;
  int last_intput_var;
};

}

#endif
