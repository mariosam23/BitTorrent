#ifndef UTILS_H
#define UTILS_H

using namespace std;

#include <unordered_map>
#include <vector>
#include <string>

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

constexpr auto MAX_DOWNLOADED_SEGMENTS = 10;

constexpr auto PEER_SEND_DATA_TAG = 1;
constexpr auto ACK_TAG = 2;
constexpr auto SWARM_INFO_TAG = 3;
constexpr auto UPDATE_SWARM_INFO_TAG = 4;
constexpr auto FINISHED_FILE_DOWNLOAD_TAG = 5;
constexpr auto PEER_FINISHED_ALL_DOWNLOADS_TAG = 6;
constexpr auto ALL_PEERS_FINISHED_DOWNLOADS_TAG = 7;
constexpr auto FINALIZE_EXECUTION_TAG = 8;
constexpr auto REQUEST_SEGMENT_TAG = 9;
constexpr auto PEER_TO_TRACKER_MSG_TAG = 10;
constexpr auto SEND_SWARM_INFO_TAG = 11;
constexpr auto PEER_REQUEST_TAG = 12;
constexpr auto NACK_TAG = 13;


enum ClientRole {
	PEER,
	SEED
};

struct file_swarms_info {
	unordered_map<string, vector<string>> file_hashes;
    unordered_map<string, vector<pair<int, ClientRole>>> clients;
};

#endif
