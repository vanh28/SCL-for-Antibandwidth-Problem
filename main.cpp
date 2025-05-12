#include <iostream>
#include <signal.h>
#include "src/staircaseEncoder.h"

using namespace SATABP;

const int ver{8};

static void SIGINT_exit(int);

static void (*signal_SIGINT)(int);
static void (*signal_SIGXCPU)(int);
static void (*signal_SIGSEGV)(int);
static void (*signal_SIGTERM)(int);
static void (*signal_SIGABRT)(int);

static void SIGINT_exit(int signum)
{
    signal(SIGINT, signal_SIGINT);
    signal(SIGXCPU, signal_SIGXCPU);
    signal(SIGSEGV, signal_SIGSEGV);
    signal(SIGTERM, signal_SIGTERM);
    signal(SIGABRT, signal_SIGABRT);

    std::cout << "c Signal interruption." << std::endl;

    fflush(stdout);
    fflush(stderr);

    raise(signum);
}

int main(int argc, char **argv)
{
    (void)argc;

    signal_SIGINT = signal(SIGINT, SIGINT_exit);
    signal_SIGXCPU = signal(SIGXCPU, SIGINT_exit);
    signal_SIGSEGV = signal(SIGSEGV, SIGINT_exit);
    signal_SIGTERM = signal(SIGTERM, SIGINT_exit);
    signal_SIGABRT = signal(SIGABRT, SIGINT_exit);

    // First argument
    int numberVars = atoi(argv[1]); // Convert first argument to int

    // Second argument
    int windowsWidth = atoi(argv[2]); // Convert second argument to int

    // Third argument
    int numberInitConds = atoi(argv[3]);
    int initConds[numberInitConds];

    VarHandler *vh = new VarHandler(1, numberVars);
    ClauseContainer *cc = new ClauseVector(vh, 0);

    StaircaseEncoder *abw_enc = new StaircaseEncoder(cc, vh);

    for (int i = 0; i < numberInitConds; i++)
    {
        initConds[i] = atoi(argv[4 + i]);
    }

    abw_enc->encode_and_solve_staircase(numberVars, windowsWidth, numberInitConds, initConds);

    delete abw_enc;
}
