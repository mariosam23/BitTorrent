#include "tracker.h"

void receive_initial_data(const int numtasks) {
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

            for (int k = 0; k < num_segments; k++) {
                char segment[HASH_SIZE];
                MPI_Recv(segment, HASH_SIZE, MPI_CHAR, i, PEER_SEND_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

        assert(MPI_Send("ACK\0", sizeof("ACK\0"), MPI_CHAR, i, ACK_TAG, MPI_COMM_WORLD) == MPI_SUCCESS);
    }
}

void tracker(int numtasks, int rank) {
    cout << "Tracker\n";

    receive_initial_data(numtasks);
}