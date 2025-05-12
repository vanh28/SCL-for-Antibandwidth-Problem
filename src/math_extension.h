#ifndef MATH_EXTENSION_H
#define MATH_EXTENSION_H

namespace SATABP
{
    class MathExtension
    {
    public:
        // Calculate factorial x!
        static int factorial(int n);

        // Calculate combination nCr
        static int combination(int n, int r);
    };
}
#endif