#ifndef TRACKER_H
#define TRACKER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <pthread.h>
#include "utils.h"
#include <unordered_set>
#include <unordered_map>
#include <mpi.h>

using namespace std;





struct file_swarms_info {
	unordered_map<string, vector<string>> file_hashes;
    unordered_map<string, vector<pair<int, ClientRole>>> clients;
};


void tracker(int numtasks, int rank);


#endif // TRACKER_H
