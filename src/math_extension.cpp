#include "math_extension.h"

namespace SATABP
{
    int MathExtension::factorial(int n)
    {
        if (n <= 1)
        {
            return 1;
        }
        else
        {
            return n * factorial(n - 1);
        }
    }

    int MathExtension::combination(int n, int r)
    {
        if (r > n || r < 0)
        {
            return 0;
        }

        int numerator = factorial(n);

        int denominator_r = factorial(r);

        int denominator_nr = factorial(n - r);

        return numerator / (denominator_r * denominator_nr);
    }
}