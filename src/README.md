## BitTorrent Protocol

### Description
An implementation of the BitTorrent protocol in Open MPI.

---

### Installation and Usage Instructions
1. Clone the Repository
2. Build the Application
	- `make`
3. Run the Application
    - `mpirun -np <num_processes> ./<executable_name>`

---

### Structure
- `src/` - Contains the source code for the BitTorrent protocol implementation.
- `src/Makefile` - Contains the instructions for building the application.
- `src/tracker.cpp` - Contains the implementation of the tracker.
- `src/peer.cpp` - Contains the implementation of the client.
- `src/tema2.cpp` - Contains the main function.

---

### Implementation Details

#### Initialization Logic
- Every peer process parses its configuration file(*in<process_rank>.txt*) and reads the information about the files it has to share and for the files it wants to download. After initialization phase, every peer sends to the tracker owned hashes.
- Tracker stores this information using this structure:

```
enum ClientRole {
	PEER,
	SEED
};

struct file_swarms_info {
	unordered_map<string, vector<string>> file_hashes;
    unordered_map<string, vector<pair<int, ClientRole>>> clients;
};
```
- file_hashes represents a map between the file name and the list of hashes of the file.
- clients represents a map between the file name and the list of pairs, where the first element of the pair is the process rank and the second element is the role of the client.

#### Downloading Logic
- Every peer process sends to the tracker a request for a file it wants to download. The tracker sends back the list of peers that have the file.
- The peer chooses uniformly random a peer different from itself or last chosen peer and sends a request for the file to that peer.
- Then, it waits for the confirmation/infirmation of the segment request.

#### Uploading Logic
- When a seed/peer receives a segment request from another peer, it checks in **owned_segments** unordered_set if it has the segment. If it has the segment, it sends ***ACK***, else it sends ***NACK***.

#### Closing Logic
- When a peer finishes downloading all desired files, tracker increments variable ***satisfied_clients***. When all clients are satisfied, tracker sends a message to all clients to close the connection.

---

<br>

<h5> &copy; 2024 Mario Sampetru</h5>
