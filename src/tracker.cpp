#include "tracker.h"


file_swarms_info receive_initial_data(const int numtasks)
{
    file_swarms_info file_swarms;

    for (int i = 1; i < numtasks; i++) {
        int num_files;
        MPI_Recv(&num_files, 1, MPI_INT, i, PEER_SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // cout << "Tracker received " << num_files << " files from peer " << i << "\n";

        for (int j = 0; j < num_files; j++) {
            char filename[MAX_FILENAME];
            MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, i, PEER_SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            string filename_str(filename);

            int num_segments;
            MPI_Recv(&num_segments, 1, MPI_INT, i, PEER_SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // cout << "Tracker received " << num_segments << " segments for file " << filename << " from peer " << i << "\n";

            vector<string>& hashes = file_swarms.file_hashes[filename_str];

            for (int k = 0; k < num_segments; k++) {
                char segment[HASH_SIZE + 1];
                MPI_Recv(segment, HASH_SIZE, MPI_CHAR, i, PEER_SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                segment[HASH_SIZE] = '\0';

                string segment_hash(segment);

                bool found = false;
                for (const auto& hash : hashes) {
                    if (hash == segment_hash) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    hashes.push_back(segment_hash);
                }

				found = false;

				for (const auto& [client, role] : file_swarms.clients[filename_str]) {
					if (client == i) {
						found = true;
						break;
					}
				}

				if (!found) {
					file_swarms.clients[filename_str].push_back(make_pair(i, SEED));
				}

			}
        }
    }

    char ack[] = "ACK";
    MPI_Bcast(ack, 4, MPI_CHAR, TRACKER_RANK, MPI_COMM_WORLD);

    return file_swarms;
}


void send_swarm_info(file_swarms_info& file_swarms) {
    char filename[MAX_FILENAME] = {0};
    MPI_Status status;
    MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, MPI_ANY_SOURCE, SWARM_INFO_TAG, MPI_COMM_WORLD, &status);
    filename[MAX_FILENAME - 1] = '\0';

    int num_hashes = file_swarms.file_hashes[filename].size();
    MPI_Send(&num_hashes, 1, MPI_INT, status.MPI_SOURCE, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD);

    for (const auto& hash : file_swarms.file_hashes[filename]) {
        MPI_Send(hash.c_str(), HASH_SIZE, MPI_CHAR, status.MPI_SOURCE, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD);
    }

    int num_clients = file_swarms.clients[filename].size();
    MPI_Send(&num_clients, 1, MPI_INT, status.MPI_SOURCE, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD);

    for (const auto& [client, role] : file_swarms.clients[filename]) {
        MPI_Send(&client, 1, MPI_INT, status.MPI_SOURCE, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD);
        MPI_Send(&role, 1, MPI_INT, status.MPI_SOURCE, SEND_SWARM_INFO_TAG, MPI_COMM_WORLD);
    }

	file_swarms.clients[filename].push_back(make_pair(status.MPI_SOURCE, PEER));
}


void update_swarm_info(file_swarms_info& file_swarms, const int source_rank)
{

}


void handle_all_peers_finished_downloads(const int numtasks)
{
    int tag = ALL_PEERS_FINISHED_DOWNLOADS_TAG;
    for (int i = 1; i < numtasks; i++) {
        MPI_Send(&tag, 1, MPI_INT, i, PEER_REQUEST_TAG, MPI_COMM_WORLD);
    }
}


void tracker(int numtasks, int rank)
{
    file_swarms_info file_swarms = receive_initial_data(numtasks);

    // cout << "\nTracker received initial data from all peers\n";

    bool all_peers_finished_downloads = false;
    int satisfied_clients = 0;

    while (!all_peers_finished_downloads) {
        MPI_Status status;
        int tag;

        MPI_Recv(&tag, 1, MPI_INT, MPI_ANY_SOURCE, PEER_TO_TRACKER_MSG_TAG, MPI_COMM_WORLD, &status);

        switch (tag) {
            case SWARM_INFO_TAG:
                send_swarm_info(file_swarms);
                break;
            case UPDATE_SWARM_INFO_TAG:
                update_swarm_info(file_swarms, status.MPI_SOURCE);
                break;
            case PEER_FINISHED_ALL_DOWNLOADS_TAG:
                satisfied_clients++;
                if (satisfied_clients == numtasks - 1) {
                    handle_all_peers_finished_downloads(numtasks);
                    all_peers_finished_downloads = true;
                }
                break;
            default:
                cout << "Invalid tag\n";
                break;
        }
    }
}
