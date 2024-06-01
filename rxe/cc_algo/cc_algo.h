#ifndef CC_ALGO_H
#define CC_ALGO_H


// if use timely algorithm
// #define RXE_USE_TIMELY_ALGO
// #define RXE_USE_DCQCN_ALGO
// #define RXE_USE_HPCC_ALGO

#ifdef RXE_USE_TIMELY_ALGO
#include "timely.h"
#endif

#ifdef RXE_USE_DCQCN_ALGO
#include "dcqcn.h"
#endif

#ifdef RXE_USE_HPCC_ALGO
#include "hpcc.h"
#endif

#endif  