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



file_swarms_info receive_initial_data(const int numtasks);

void send_swarm_info(file_swarms_info& file_swarms);

void update_swarm_info(file_swarms_info& file_swarms, const int source_rank);

void handle_all_peers_finished_downloads(const int numtasks);

void tracker(int numtasks, int rank);


#endif // TRACKER_H
