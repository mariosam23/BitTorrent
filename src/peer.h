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

// void read_file(const int& rank);

void *download_thread_func(void *arg);

void *upload_thread_func(void *arg);

void peer(int numtasks, int rank);

#endif // PEER_H
