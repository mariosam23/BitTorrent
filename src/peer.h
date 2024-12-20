#ifndef PEER_H
#define PEER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <utility>
#include <cassert>
#include <pthread.h>
#include <unordered_set>
#include <unordered_map>
#include <mpi.h>
#include <random>

#include "utils.h"


using namespace std;

typedef string filename;
typedef string segment;

struct download_thread_args {
	int rank;
	unordered_set<filename> desired_files;
};

struct upload_thread_args {
	int rank;
	unordered_map<filename, vector<segment>> owned_filenames_hashes;
};

pair<unordered_set<filename>, unordered_map<filename, vector<string>>> read_file(const int& rank);

void send_data_to_tracker(const int& rank,
						  const unordered_map<filename, vector<string>>& owned_filenames_hashes);

file_swarms_info receive_file_swarms_info(const int& rank, const unordered_set<filename>& desired_files);

int chose_uniform_random_peer(const vector<int>& peers, int last_chosen, const int my_rank);

void save_file(const int rank, const filename& downloaded_file, const vector<string>& hashes);

vector<int> request_update_swarm_info(const int rank, const filename& filename);

void send_segment_to_peer(const int& peer_rank);

void *download_thread_func(void *arg);

void *upload_thread_func(void *arg);

void peer(int numtasks, int rank);

#endif // PEER_H
