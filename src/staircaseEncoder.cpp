#include "staircaseEncoder.h"
#include <iostream>
#include <math.h>

namespace SATABP
{

    StaircaseEncoder::StaircaseEncoder(ClauseContainer *clause_container, VarHandler *var_handler) : cc(clause_container), vh(var_handler){};

    StaircaseEncoder::~StaircaseEncoder(){};

    bool isDebugMode = true;

    int StaircaseEncoder::GetEncodedAuxVar(int symbolicAuxVar)
    {
        auto pair = auxVarConverter.find(symbolicAuxVar);

        if (pair != auxVarConverter.end())
        {
            // Key found, return the value
            return pair->second;
        }
        else
        {
            // Key not found, create and add key then return the value
            int encoderAuxVar = vh->get_new_var();
            auxVarConverter.insert({symbolicAuxVar, encoderAuxVar});
            return encoderAuxVar;
        }
        // return symbolicAuxVar;
    }

    void StaircaseEncoder::encode_and_solve_staircase(int n, int w, int initCondLength, int initCond[])
    {
        (void)initCondLength;
        (void)initCond;
        int k = w;

        int number_subset = ceil((float)n / k) - 1;

        // Because only the last subset can be an incomplete subset
        // while other subsets must be complete subsets, so we split
        // them into two parts then encoding each part separately

        // In our proposal, the left block of both complete and incomplete
        // subsets are encoded similarly

        if (isDebugMode)
            std::cout << "Duplex NSC for complete block: \n";
        for (int s = 0; s < number_subset - 1; s++)
        {
            if (isDebugMode)
                std::cout << "Of the subset " << s << "\n";
            if (isDebugMode)
                std::cout << "Left block: \n";
            for (int i = 1; i <= k - 1; i++)
            {
                cc->add_clause({-(s * k + i + 1), GetEncodedAuxVar(2 * s * k + k - i)});
                if (isDebugMode)
                    std::cout << -(s * k + i + 1) << " " << GetEncodedAuxVar(2 * s * k + k - i) << std::endl;
            }

            for (int ii = 1; ii <= k - 2; ii++)
            {
                cc->add_clause({-(GetEncodedAuxVar(2 * s * k + ii)), GetEncodedAuxVar(2 * s * k + ii + 1)});
                if (isDebugMode)
                    std::cout << -(GetEncodedAuxVar(2 * s * k + ii)) << " " << GetEncodedAuxVar(2 * s * k + ii + 1) << std::endl;
            }

            for (int iii = 0; iii <= k - 2; iii++)
            {
                cc->add_clause({-(s * k + iii + 1), -(GetEncodedAuxVar(2 * s * k + k - 1 - iii))});
                if (isDebugMode)
                    std::cout << -(s * k + iii + 1) << " " << -(GetEncodedAuxVar(2 * s * k + k - 1 - iii)) << std::endl;
            }

            if (isDebugMode)
                std::cout << "Right block \n";
            for (int i = 0; i <= k - 2; i++)
            {
                cc->add_clause({-(s * k + k + i + 1), GetEncodedAuxVar((2 * s + 1) * k + i + 1)});
                if (isDebugMode)
                    std::cout << -(s * k + k + i + 1) << " " << GetEncodedAuxVar((2 * s + 1) * k + i + 1) << std::endl;
            }

            for (int ii = 1; ii <= k - 2; ii++)
            {
                cc->add_clause({-(GetEncodedAuxVar((2 * s + 1) * k + ii)), GetEncodedAuxVar((2 * s + 1) * k + ii + 1)});
                if (isDebugMode)
                    std::cout << -(GetEncodedAuxVar((2 * s + 1) * k + ii)) << " " << GetEncodedAuxVar((2 * s + 1) * k + ii + 1) << std::endl;
            }

            for (int iii = 1; iii <= k - 1; iii++)
            {
                cc->add_clause({-(s * k + k + iii + 1), -(GetEncodedAuxVar((2 * s + 1) * k + iii))});
                if (isDebugMode)
                    std::cout << -(s * k + k + iii + 1) << " " << -(GetEncodedAuxVar((2 * s + 1) * k + iii)) << std::endl;
            }

            // Combining Duplex & NSC
            if (isDebugMode)
                std::cout << "Combine left & right block\n";
            for (int i = 1; i <= k - 1; i++)
            {
                cc->add_clause({-(GetEncodedAuxVar(2 * s * k + k - i)), -(GetEncodedAuxVar((2 * s + 1) * k + i))});
                if (isDebugMode)
                    std::cout << -(GetEncodedAuxVar(2 * s * k + k - i)) << " " << -(GetEncodedAuxVar((2 * s + 1) * k + i)) << std::endl;
            }
        }

        if (isDebugMode)
            std::cout << "Duplex NSC for the last subset: \n";
        for (int s = number_subset - 1; s < number_subset; s++)
        {
            if (isDebugMode)
                std::cout << "Of the subset " << s << "\n";
            if (isDebugMode)
                std::cout << "Left block: \n";
            for (int i = 1; i <= k - 1; i++)
            {
                cc->add_clause({-(s * k + i + 1), GetEncodedAuxVar(2 * s * k + k - i)});
                if (isDebugMode)
                    std::cout << -(s * k + i + 1) << " " << GetEncodedAuxVar(2 * s * k + k - i) << std::endl;
            }

            for (int ii = 1; ii <= k - 2; ii++)
            {
                cc->add_clause({-(GetEncodedAuxVar(2 * s * k + ii)), GetEncodedAuxVar(2 * s * k + ii + 1)});
                if (isDebugMode)
                    std::cout << -(GetEncodedAuxVar(2 * s * k + ii)) << " " << GetEncodedAuxVar(2 * s * k + ii + 1) << std::endl;
            }

            for (int iii = 0; iii <= k - 2; iii++)
            {
                cc->add_clause({-(s * k + iii + 1), -(GetEncodedAuxVar(2 * s * k + k - 1 - iii))});
                if (isDebugMode)
                    std::cout << -(s * k + iii + 1) << " " << -(GetEncodedAuxVar(2 * s * k + k - 1 - iii)) << std::endl;
            }

            int limit = n % k;
            if (limit == 0)
                limit = k;
            if (isDebugMode)
                std::cout << "Right block \n";
            if (isDebugMode)
                std::cout << "Size: " << limit << "\n";
            for (int i = 0; i <= k - 2; i++)
            {
                if (i == limit)
                    break;
                cc->add_clause({-(s * k + k + i + 1), GetEncodedAuxVar((2 * s + 1) * k + i + 1)});
                if (isDebugMode)
                    std::cout << -(s * k + k + i + 1) << " " << GetEncodedAuxVar((2 * s + 1) * k + i + 1) << std::endl;
            }

            for (int ii = 1; ii <= k - 2; ii++)
            {
                if (ii == limit)
                    break;
                cc->add_clause({-(GetEncodedAuxVar((2 * s + 1) * k + ii)), GetEncodedAuxVar((2 * s + 1) * k + ii + 1)});
                if (isDebugMode)
                    std::cout << -(GetEncodedAuxVar((2 * s + 1) * k + ii)) << " " << GetEncodedAuxVar((2 * s + 1) * k + ii + 1) << std::endl;
            }

            for (int iii = 1; iii <= k - 1; iii++)
            {
                if (iii == limit)
                    break;
                cc->add_clause({-(s * k + k + iii + 1), -(GetEncodedAuxVar((2 * s + 1) * k + iii))});
                if (isDebugMode)
                    std::cout << -(s * k + k + iii + 1) << " " << -(GetEncodedAuxVar((2 * s + 1) * k + iii)) << std::endl;
            }

            // Combining Duplex & NSC
            if (isDebugMode)
                std::cout << "Combine left & right block\n";
            for (int i = 1; i <= k - 1; i++)
            {
                cc->add_clause({-(GetEncodedAuxVar(2 * s * k + k - i)), -(GetEncodedAuxVar((2 * s + 1) * k + i))});
                if (isDebugMode)
                    std::cout << -(GetEncodedAuxVar(2 * s * k + k - i)) << " " << -(GetEncodedAuxVar((2 * s + 1) * k + i)) << std::endl;
                if (i == limit)
                    break;
            }
        }

        if (isDebugMode)
            std::cout << "\nPrint dimacs:\n";
        if (isDebugMode)
            cc->print_dimacs();

        if (isDebugMode)
            std::cout << "\nStaircase visualization:\n";
        for (int l = 1; l <= (int)n - k + 1; l++)
        {
            for (int i = 0; i < l - 1; i++)
            {
                if (isDebugMode)
                    std::cout << "\t";
            }
            for (int ll = l; ll <= l + k - 1; ll++)
            {
                if (isDebugMode)
                    std::cout << ll;
                if (ll < l + k - 1)
                {
                    if (isDebugMode)
                        std::cout << " +\t";
                }
                else
                {
                    if (isDebugMode)
                        std::cout << "\t<= 1\n";
                }
            }
        }
    }
}
