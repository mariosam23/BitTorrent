#ifndef TRACKER_H
#define TRACKER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <pthread.h>
#include "utils.h"
#include <mpi.h>

using namespace std;

void tracker(int numtasks, int rank);


#endif // TRACKER_H
