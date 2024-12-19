#include "peer.h"

#include <mutex>

unordered_set<string> owned_segments;
mutex owned_segments_mutex;


pair<unordered_set<filename>, unordered_map<filename, vector<string>>>
read_file(const int& rank)
{
	ifstream peer_file("in" + to_string(rank) + ".txt");
	assert(peer_file.is_open() && "Error opening file");

	// {desired_filename1, desired_filename2, ...}
	unordered_set<filename> desired_files;
	// [filename : {hash1, hash2, ...}]
	unordered_map<filename, vector<string>> owned_filenames_hashes; 

	int num_owned_files;
	peer_file >> num_owned_files;
	
	for (int i = 0; i < num_owned_files; ++i) {
		string filename;
		int num_segments;
		peer_file >> filename >> num_segments;
		for (int j = 0; j < num_segments; ++j) {
			string hash;
			peer_file >> hash;
			owned_filenames_hashes[filename].push_back(hash);
			owned_segments.insert(hash);
		}
	}

	int num_desired_files;
	peer_file >> num_desired_files;
	
	for (int i = 0; i < num_desired_files; ++i) {
		string filename;
		peer_file >> filename;
		desired_files.insert(filename);
	}

	peer_file.close();

	return make_pair(desired_files, owned_filenames_hashes);
}


void send_data_to_tracker(const int& rank,
						  const unordered_map<filename, vector<string>>& owned_filenames_hashes)
{
	int num_owned_files = owned_filenames_hashes.size();
	MPI_Send(&num_owned_files, 1, MPI_INT, TRACKER_RANK, PEER_SEND_DATA_TAG, MPI_COMM_WORLD);
	
	for (const auto& [filename, hashes] : owned_filenames_hashes) {
		MPI_Send(filename.c_str(), filename.size(), MPI_CHAR, TRACKER_RANK, PEER_SEND_DATA_TAG, MPI_COMM_WORLD);
		
		int num_segments = hashes.size();
		MPI_Send(&num_segments, 1, MPI_INT, TRACKER_RANK, PEER_SEND_DATA_TAG, MPI_COMM_WORLD);
		
		for (const auto& hash : hashes) {
			MPI_Send(hash.c_str(), hash.size(), MPI_CHAR, TRACKER_RANK, PEER_SEND_DATA_TAG, MPI_COMM_WORLD);
			owned_segments.insert(hash);
		}
	}

	char tracker_response_msg[4];
	MPI_Bcast(tracker_response_msg, 4, MPI_CHAR, TRACKER_RANK, MPI_COMM_WORLD);

	cout << "Peer " << rank << " received " << tracker_response_msg << " from tracker\n";
}


file_swarms_info receive_file_swarms_info
(const int& rank, const unordered_set<filename>& desired_files)
{
	file_swarms_info received_file_swarms_info;

	for (const auto& filename : desired_files) {
		// cout << "Peer " << rank << " downloading file " << filename << "\n";

		// Send request to tracker for swarm info
		int tag = SWARM_INFO_TAG;
		MPI_Send(&tag, 1, MPI_INT, TRACKER_RANK, PEER_TO_TRACKER_MSG_TAG, MPI_COMM_WORLD);

		// Receive swarm info from tracker
		MPI_Send(filename.c_str(), MAX_FILENAME, MPI_CHAR, TRACKER_RANK, SWARM_INFO_TAG, MPI_COMM_WORLD);

		int num_hashes;
		MPI_Recv(&num_hashes, 1, MPI_INT, TRACKER_RANK, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		cout << "Client " << rank << " received " << num_hashes << " hashes for file " << filename << "\n";
		for (int i = 0; i < num_hashes; ++i) {
			char hash[HASH_SIZE];
			MPI_Recv(hash, HASH_SIZE, MPI_CHAR, TRACKER_RANK, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			hash[HASH_SIZE] = '\0';
			received_file_swarms_info.file_hashes[filename].push_back(hash);
		}

		int num_clients;
		MPI_Recv(&num_clients, 1, MPI_INT, TRACKER_RANK, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		for (int i = 0; i < num_clients; ++i) {
			int client_rank;
			ClientRole role;
			MPI_Recv(&client_rank, 1, MPI_INT, TRACKER_RANK, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(&role, 1, MPI_INT, TRACKER_RANK, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			received_file_swarms_info.clients[filename].push_back(make_pair(client_rank, role));
		}
	}

	return received_file_swarms_info;
}


int chose_uniform_random_peer(const vector<int>& peers, int last_chosen, const int my_rank) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, peers.size() - 1);

    int chosen_index;
    int chosen_peer;
    int steps = 0;

    do {
        chosen_index = dis(gen);
        chosen_peer = peers[chosen_index];

        steps++;

        // If no valid peer is found after a reasonable number of attempts, fallback to last chosen
        if (steps > 10) {
            return last_chosen >= 0 ? last_chosen : peers[0];
        }
    } while (chosen_peer == last_chosen || chosen_peer == my_rank);

    return chosen_peer;
}


void save_file(const int rank, const filename& downloaded_file, const vector<string>& hashes)
{
	string filename = "client" + to_string(rank) + "_" + downloaded_file;
	ofstream file(filename);
	assert(file.is_open() && "Error opening file");

	for (const auto& hash : hashes) {
		file << hash << "\n";
	}

	file.close();
}


vector<int> request_update_swarm_info(const int rank, const filename& filename)
{
	int tag = UPDATE_SWARM_INFO_TAG;
	MPI_Send(&tag, 1, MPI_INT, TRACKER_RANK, PEER_TO_TRACKER_MSG_TAG, MPI_COMM_WORLD);

	MPI_Send(filename.c_str(), MAX_FILENAME, MPI_CHAR, TRACKER_RANK, UPDATE_SWARM_INFO_TAG, MPI_COMM_WORLD);

	int num_clients;
	MPI_Recv(&num_clients, 1, MPI_INT, TRACKER_RANK, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	vector<int> clients_ranks;
	for (int i = 0; i < num_clients; ++i) {
		int client_rank;
		MPI_Recv(&client_rank, 1, MPI_INT, TRACKER_RANK, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		clients_ranks.push_back(client_rank);
	}

	return clients_ranks;
}


void *download_thread_func(void *arg)
{
	download_thread_args *args = (download_thread_args*) arg;
	int rank = args->rank;
	unordered_set<filename> desired_files = args->desired_files;

	auto desired_segments_data = receive_file_swarms_info(rank, desired_files); 

	unordered_map<string, vector<string>> filename_achieved_segments;

	for (const auto& file : desired_files) {
		vector<string> hashes = desired_segments_data.file_hashes[file];
		vector<pair<int, ClientRole>> clients = desired_segments_data.clients[file];
		vector<int> clients_ranks;
		for (const auto& [client_rank, role] : clients) {
			clients_ranks.push_back(client_rank);
		}

		int last_chosen = -1;

		for (size_t i = 0; i < hashes.size(); ++i) {
			if (i % MAX_FILES == 0) {
				clients_ranks.clear();
				clients_ranks = request_update_swarm_info(rank, file);
			}

			string hash = hashes[i];

			int chosen_peer = chose_uniform_random_peer(clients_ranks, last_chosen, rank);
			last_chosen = chosen_peer;
			int tag = REQUEST_SEGMENT_TAG;
			MPI_Send(&tag, 1, MPI_INT, chosen_peer, PEER_REQUEST_TAG, MPI_COMM_WORLD);

			MPI_Send(hash.c_str(), HASH_SIZE, MPI_CHAR, chosen_peer, REQUEST_SEGMENT_TAG, MPI_COMM_WORLD);

			MPI_Recv(&tag, 1, MPI_INT, chosen_peer, ACK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			if (tag == ACK_TAG) {
				owned_segments.insert(hash);
				filename_achieved_segments[file].push_back(hash);
			} else {
				// try again
				i--;
			}
		}

		cout << filename_achieved_segments[file].size() << " segments downloaded for file " << file << "\n";

		save_file(rank, file, filename_achieved_segments[file]);
	}


	int tag = PEER_FINISHED_ALL_DOWNLOADS_TAG;
	MPI_Send(&tag, 1, MPI_INT, TRACKER_RANK, PEER_TO_TRACKER_MSG_TAG, MPI_COMM_WORLD);


    return NULL;
}


void send_segment_to_peer(const int& peer_rank)
{
	char segment[HASH_SIZE];
	MPI_Recv(segment, HASH_SIZE, MPI_CHAR, peer_rank, REQUEST_SEGMENT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);


	int tag = ACK_TAG;
	if (owned_segments.find(segment) != owned_segments.end()) {
		cout << "NOT FOUND << \n";
		tag = NACK_TAG;
	}

	MPI_Send(&tag, 1, MPI_INT, peer_rank, ACK_TAG, MPI_COMM_WORLD);
}


void *upload_thread_func(void *arg)
{
	upload_thread_args *args = (upload_thread_args*) arg;
	auto owned_filenames_hashes = args->owned_filenames_hashes;


	while (true) {
		int tag;
		MPI_Status status;
		MPI_Recv(&tag, 1, MPI_INT, MPI_ANY_SOURCE, PEER_REQUEST_TAG, MPI_COMM_WORLD, &status);
	
		switch (tag) {
			case REQUEST_SEGMENT_TAG: {
				send_segment_to_peer(status.MPI_SOURCE);
				break;
			}
			case ALL_PEERS_FINISHED_DOWNLOADS_TAG: {
				return NULL;
			}
		}
	}

    return NULL;
}


void peer(int numtasks, int rank) {
	auto [desired_files, owned_filenames_hashes] = read_file(rank);
	send_data_to_tracker(rank, owned_filenames_hashes);
	
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

	download_thread_args download_args = {rank, desired_files};
	upload_thread_args upload_args = {rank, owned_filenames_hashes};

	r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &download_args);
    if (r) {
        cout << "Error creating download thread\n";
        exit(-1);
    }

	r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) &upload_args);
    if (r) {
        cout << "Error creating upload thread\n";
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        cout << "Error joining download thread\n";
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        cout << "Error joining upload thread\n";
        exit(-1);
    }
}
