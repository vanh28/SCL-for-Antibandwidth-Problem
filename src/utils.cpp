#include "utils.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <stdlib.h> //abs

namespace SATABP
{

    Graph::Graph(std::string file_name) : edges(std::vector<std::pair<int, int>>())
    {
        std::ifstream input_file_stream(file_name);
        std::string line;

        if (!input_file_stream.is_open())
        {
            std::cout << "c Error, could not open file '" << file_name << "'. " << std::endl;
            n = 0;
            graph_name = file_name;
            return;
        }
        filename(file_name);
        graph_name = file_name;
        while (std::getline(input_file_stream, line))
        {
            std::istringstream line_stream(line);
            std::string term;
            std::vector<int> terms;
            while (line_stream >> term)
            {
                bool has_only_digits = (term.find_first_not_of("0123456789") == std::string::npos);
                if (has_only_digits)
                {
                    terms.push_back(std::stoi(term));
                }
                else
                {
                    terms.clear();
                    break;
                }
            }
            if (terms.size() == 3)
            {
                n = terms[0];
                number_of_edges = terms[2];
            }
            else if (terms.size() == 2)
            { // SELF LOOPS ARE NOT FILTERED HERE!
                if (terms[0] <= terms[1])
                    edges.emplace_back(terms[0], terms[1]);
                else
                    edges.emplace_back(terms[1], terms[0]);
            }
        }
        input_file_stream.close();
        assert(edges.size() == number_of_edges);
    };

    void Graph::filename(std::string &path)
    {
        if (path.find_last_of("/") != std::string::npos)
        {
            path.erase(0, path.find_last_of("/") + 1);
        }
    };

    void Graph::print_stat() const
    {
        std::cout << "Graph constructed with " << n << " nodes. Edges" << std::endl;
        for (std::pair<int, int> edge : edges)
        {
            std::cout << edge.first << " - " << edge.second << std::endl;
        }
    };

    int Graph::calculate_antibandwidth(const std::vector<int> &node_labels) const
    {
        int min_dist = n;
        for (std::pair<int, int> edge : edges)
        {
            int label1 = node_labels[edge.first - 1];
            int label2 = node_labels[edge.second - 1];
            int distance = abs(label1 - label2);

            if (distance < min_dist)
            {
                min_dist = distance;
            }
        }

        return min_dist;
    };

    int Graph::calculate_bandwidth(const std::vector<int> &node_labels) const
    {
        int max_dist = 0;
        for (std::pair<int, int> edge : edges)
        {
            int label1 = node_labels[edge.first - 1];
            int label2 = node_labels[edge.second - 1];
            int distance = abs(label1 - label2);

            if (distance > max_dist)
            {
                max_dist = distance;
            }
        }

        return max_dist;
    };

    unsigned Graph::find_greatest_outdegree_node() const
    {
        assert(n > 0);
        std::vector<int> out_degrees = std::vector<int>(n, 0);
        for (std::pair<int, int> edge : edges)
        {
            out_degrees[edge.first - 1] += 1;
            out_degrees[edge.second - 1] += 1;
        }

        int max_node = 1;
        int max_odegree = out_degrees[0];
        for (unsigned i = 1; i < out_degrees.size(); ++i)
        {
            if (out_degrees[i] > max_odegree)
            {
                max_node = i + 1;
                max_odegree = out_degrees[i];
            }
        }
        return max_node;
    };

    unsigned Graph::find_smallest_outdegree_node() const
    {
        assert(n > 0);
        std::vector<int> out_degrees = std::vector<int>(n, 0);
        for (std::pair<int, int> edge : edges)
        {
            out_degrees[edge.first - 1] += 1;
            out_degrees[edge.second - 1] += 1;
        }

        int min_node = 1;
        int min_odegree = out_degrees[0];
        for (unsigned i = 1; i < out_degrees.size(); ++i)
        {
            if (out_degrees[i] < min_odegree)
            {
                min_node = i + 1;
                min_odegree = out_degrees[i];
            }
        }
        return min_node;
    };

    /*
     * input_size: number of nodes -> number of variables will be input_size
     */
    VarHandler::VarHandler(int start_id, int input_size = 0) : first_assigned_id(start_id)
    {
        if (input_size > 0)
        {
            next_to_assign = first_assigned_id + input_size;
            last_intput_var = next_to_assign - 1;
        }
        else
        {
            last_intput_var = 0;
            next_to_assign = start_id;
        }
    };

    int VarHandler::get_new_var()
    {
        return next_to_assign++;
    };

    int VarHandler::last_var() const
    {
        return next_to_assign - 1;
    };

    int VarHandler::size() const
    {
        return next_to_assign - first_assigned_id;
    };

}
