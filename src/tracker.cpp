#include "tracker.h"


file_swarms_info receive_initial_data(const int numtasks)
{
    file_swarms_info file_swarms;

    for (int i = 1; i < numtasks; i++) {
        int num_files;
        MPI_Recv(&num_files, 1, MPI_INT, i, PEER_SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        cout << "Tracker received " << num_files << " files from peer " << i << "\n";

        for (int j = 0; j < num_files; j++) {
            char filename[MAX_FILENAME];
            MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, i, PEER_SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int num_segments;
            MPI_Recv(&num_segments, 1, MPI_INT, i, PEER_SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            cout << "Tracker received " << num_segments << " segments for file " << filename << " from peer " << i << "\n";

            vector<segment_data>& segments = file_swarms.file_segments[filename];

            for (int k = 0; k < num_segments; k++) {
                char segment[HASH_SIZE];
                MPI_Recv(segment, HASH_SIZE, MPI_CHAR, i, PEER_SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                segment[HASH_SIZE] = '\0';

                segment_data segment_data;
                segment_data.hash = segment;
                segment_data.clients[i] = SEED;

                if (segments.empty()) {
                    segments.push_back(segment_data);
                } else {
                    bool found = false;
                    for (auto& segment : segments) {
                        if (segment.hash == segment_data.hash) {
                            segment.clients[i] = SEED;
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        segments.push_back(segment_data);
                    }
                }
            }
        }
    }

    char ack[] = "ACK";
    MPI_Bcast(ack, 4, MPI_CHAR, TRACKER_RANK, MPI_COMM_WORLD);

    return file_swarms;
}


void send_swarm_info()
{
    cout << "\n\nTracker received request for swarm info\n\n";
}


void update_swarm_info()
{

}


void handle_finished_file_download()
{
    
}


void handle_peer_finished_all_downloads()
{
    
}


void handle_all_peers_finished_downloads()
{
    
}


void tracker(int numtasks, int rank)
{
    file_swarms_info file_swarms = receive_initial_data(numtasks);

    cout << "\nTracker received initial data from all peers\n";

    bool all_peers_finished_downloads = false;
    int clients = 0;

    while (!all_peers_finished_downloads) {
        clients++;
        MPI_Status status;
        MPI_Recv(NULL, 0, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        int source = status.MPI_SOURCE;
        int tag = status.MPI_TAG;

        switch (tag) {
            case SWARM_INFO_TAG:
                send_swarm_info();
                break;
            // case UPDATE_SWARM_INFO_TAG:
            //     update_swarm_info();
            //     break;
            // case FINISHED_FILE_DOWNLOAD_TAG:
            //     handle_finished_file_download();
            //     break;
            // case PEER_FINISHED_ALL_DOWNLOADS_TAG:
            //     handle_peer_finished_all_downloads();
            //     break;
            // case ALL_PEERS_FINISHED_DOWNLOADS_TAG:
            //     handle_all_peers_finished_downloads();
            //     all_peers_finished_downloads = true;
            //     break;
            default:
                cout << "Invalid tag\n";
                break;
        }
        if (clients == numtasks - 1) {
            all_peers_finished_downloads = true;
        }
    }

}
