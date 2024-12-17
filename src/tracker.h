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


enum ClientRole {
	PEER,
	SEED
};


struct segment_data {
    string hash;
    unordered_map<int, ClientRole> clients;
};


struct file_swarms_info {
	unordered_map<string, vector<segment_data>> file_segments;
};


void tracker(int numtasks, int rank);


#endif // TRACKER_H
