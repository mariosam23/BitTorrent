#include "peer.h"


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
		}
	}

	char tracker_response_msg[4];
	MPI_Bcast(tracker_response_msg, 4, MPI_CHAR, TRACKER_RANK, MPI_COMM_WORLD);

	cout << "Peer " << rank << " received " << tracker_response_msg << " from tracker\n";
}


vector<pair<string, vector<int>>> receive_file_swarms_info
(const int& rank, const unordered_set<filename>& desired_files)
{
	vector<pair<string, vector<int>>> file_hashes;

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
		}

		int num_clients;
		MPI_Recv(&num_clients, 1, MPI_INT, TRACKER_RANK, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		for (int i = 0; i < num_clients; ++i) {
			int client_rank;
			ClientRole role;
			MPI_Recv(&client_rank, 1, MPI_INT, TRACKER_RANK, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(&role, 1, MPI_INT, TRACKER_RANK, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	}



	return file_hashes;
}


int chose_uniform_random_peer(const vector<int>& peers, const int last_chosen, const int my_rank)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, peers.size() - 1);

	int chosen = dis(gen);
	int steps = 0;
	while (chosen == last_chosen || chosen == my_rank) {
		chosen = dis(gen);
		steps++;

		// if last_chosen is the only peer available, return it
		if (steps > 10) {
			return last_chosen;
		}
	}

	return chosen;
}


void *download_thread_func(void *arg)
{
	download_thread_args *args = (download_thread_args*) arg;
	int rank = args->rank;
	unordered_set<filename> desired_files = args->desired_files;

	vector<pair<string, vector<int>>> file_hashes = receive_file_swarms_info(rank, desired_files); 



	int tag = PEER_FINISHED_ALL_DOWNLOADS_TAG;
	MPI_Send(&tag, 1, MPI_INT, TRACKER_RANK, PEER_TO_TRACKER_MSG_TAG, MPI_COMM_WORLD);



    return NULL;
}


void *upload_thread_func(void *arg)
{
	upload_thread_args *args = (upload_thread_args*) arg;
	int rank = args->rank;
	auto owned_filenames_hashes = args->owned_filenames_hashes;



	// wait for all peers to finish downloads
	MPI_Recv(NULL, 0, MPI_INT, TRACKER_RANK, ALL_PEERS_FINISHED_DOWNLOADS_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

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
