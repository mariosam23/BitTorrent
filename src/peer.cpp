#include "peer.h"


pair<unordered_set<filename>, unordered_map<filename, vector<segment>>>
read_file(const int& rank)
{
	ifstream peer_file("in" + to_string(rank) + ".txt");
	assert(peer_file.is_open() && "Error opening file");

	// {desired_filename1, desired_filename2, ...}
	unordered_set<filename> desired_files;
	// [filename : {segment1, segment2, ...}]
	unordered_map<filename, vector<segment>> owned_filenames_segments; 

	int num_owned_files;
	peer_file >> num_owned_files;
	for (int i = 0; i < num_owned_files; ++i) {
		string filename;
		int num_segments;
		peer_file >> filename >> num_segments;
		for (int j = 0; j < num_segments; ++j) {
			string segment;
			peer_file >> segment;
			owned_filenames_segments[filename].push_back(segment);
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

	return make_pair(desired_files, owned_filenames_segments);
}


void send_data_to_tracker(const int& rank,
						  const unordered_map<filename, vector<segment>>& owned_filenames_segments)
{
	int num_owned_files = owned_filenames_segments.size();
	MPI_Send(&num_owned_files, 1, MPI_INT, TRACKER_RANK, PEER_SEND_DATA_TAG, MPI_COMM_WORLD);
	
	for (const auto& [filename, segments] : owned_filenames_segments) {
		MPI_Send(filename.c_str(), filename.size(), MPI_CHAR, TRACKER_RANK, PEER_SEND_DATA_TAG, MPI_COMM_WORLD);
		int num_segments = segments.size();
		MPI_Send(&num_segments, 1, MPI_INT, TRACKER_RANK, PEER_SEND_DATA_TAG, MPI_COMM_WORLD);
		for (const auto& segment : segments) {
			MPI_Send(segment.c_str(), segment.size(), MPI_CHAR, TRACKER_RANK, PEER_SEND_DATA_TAG, MPI_COMM_WORLD);
		}
	}

	cout << "Peer " << rank << " sent data to tracker. Now waiting for response...\n";

	char tracker_responnse_msg[10];
	MPI_Recv(tracker_responnse_msg, 10, MPI_CHAR, TRACKER_RANK, ACK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	cout << "Peer " << rank << " received " << tracker_responnse_msg << " from tracker\n";
}


void *download_thread_func(void *arg)
{
	download_thread_args *args = (download_thread_args*) arg;
	int rank = args->rank;
	unordered_set<filename> desired_files = args->desired_files;

    return NULL;
}


void *upload_thread_func(void *arg)
{
	upload_thread_args *args = (upload_thread_args*) arg;
	int rank = args->rank;
	auto owned_filenames_segments = args->owned_filenames_segments;

    return NULL;
}


void peer(int numtasks, int rank) {
    cout << "Peer " << rank << "\n";
	auto [desired_files, owned_filenames_segments] = read_file(rank);
	send_data_to_tracker(rank, owned_filenames_segments);

    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

	download_thread_args download_args = {rank, desired_files};
	upload_thread_args upload_args = {rank, owned_filenames_segments};

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
 