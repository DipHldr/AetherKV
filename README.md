# C++ | High-Performance Persistent KV Store

A minimalist, high-performance Key-Value storage engine built in Modern C++ using an **Append-Only Log (AOL)** architecture. This project implements binary serialization, length-prefixed records, and an in-memory hash index for $O(1)$ disk access.


## Architecture Overview

### Storage Model
The engine follows a log-structured design, treating disk storage as an immutable sequence of events:
* **storage.bin**: The Write-Ahead Log (WAL) and primary source of truth.
* **table.bin**: A binary snapshot of the in-memory index for fast boots.
* **MemTable Index**: An in-memory `unordered_map` mapping `std::string` keys to `uint64_t` file offsets.



### Binary Record Format
Records are stored in a packed binary format to ensure zero-copy overhead and cross-platform portability.

| Field | Size | Description |
| :--- | :--- | :--- |
| **CRC** | 4 Bytes | IEEE 802.3 integrity checksum |
| **MAGIC** | 4 Bytes | File identifier (`0x4B565331`) |
| **TYPE** | 2 Bytes | Record Type (SET=1, DELETE=2) |
| **KEY_LEN** | 4 Bytes | Length of the Key |
| **VALUE_LEN** | 4 Bytes | Length of the Value |
| **DATA** | Variable | Raw Key + Raw Value bytes |


## Why This Project?
This project was built to master the internals of modern storage engines like **Redis AOF**, **LevelDB**, and **RocksDB**. It demonstrates a deep understanding of:
* Binary serialization & memory alignment.
* Log-structured file I/O.
* Data integrity and error recovery in systems programming.






##  Features
* **Binary Storage:** Uses raw byte-stream I/O (`.write`/`.read`) for maximum performance and zero-copy overhead.
* **Length-Prefixed Serialization:** Handles complex, multi-line, and binary values without delimiter collisions.
* **O(1) Retrieval:** Maintains an in-memory `unordered_map` index storing byte offsets for instant data lookup.
* **Crash Resilience:** Append-only logic ensures that existing data is never overwritten during new writes.
* **Buffer Management:** Manual `.flush()` control to balance I/O throughput with data persistence.


## Durability & Crash Recovery

### Recovery Model
On startup, the engine follows a multi-tier recovery strategy:
1.  **Fast Boot**: Attempt to load the `table.bin` index snapshot.
2.  **Replay Logic**: If the snapshot is missing or corrupted, the engine performs a sequential scan of `storage.bin`.
3.  **Tombstone Handling**: Deletions are stored as "Tombstone" records. During replay, a DELETE record removes the key from the in-memory index.

### Resynchronization Strategy
If the engine encounters a corrupted segment, it performs **Byte-wise Magic Scanning**. It skips the corrupted byte and "hunts" for the next `MAGIC` preamble to resume indexing, ensuring maximum data salvage.



## Data Integrity
The engine protects against "bit-rot" and partial writes using **Streaming CRC32 Checksums**:
* **Incremental Calculation**: Checksums are computed over the Header, Key, and Value using $O(1)$ space.
* **Verification**: Every `GET` request triggers a full integrity check. If a mismatch is detected, the engine rejects the record to prevent returning corrupted data.



##  Performance Characteristics

| Operation | Complexity | Description |
| :--- | :--- | :--- |
| **SET** | $O(1)$ | Single sequential append to WAL |
| **GET** | $O(1)$ | Offset-based seek to raw disk location |
| **DEL** | $O(1)$ | Append a tombstone record |
| **RECOVERY**| $O(N)$ | Sequential log scan (N = total records) |
| **SNAPSHOT**| $O(K)$ | Serializing active keys (K = active keys) |

## Internal Flow & Lifecycle

To ensure maximum performance and data safety, the engine follows a strict request-response lifecycle:

### **SET Operation**
1. **Serialization**: Convert key and value into a contiguous byte buffer.
2. **Integrity**: Compute the CRC32 checksum over the record metadata and payload.
3. **Commit**: Perform a sequential `write()` to the end of `storage.bin` (WAL).
4. **Index Update**: Store the resulting file offset in the in-memory **MemTable**.

### **GET Operation**
1. **Lookup**: Retrieve the 64-bit file offset from the MemTable ($O(1)$).
2. **Direct I/O**: `seekg()` to the offset and read the fixed-size Header.
3. **Validation**: Read the payload and verify the checksum.
4. **Return**: Deliver the value to the client only if the CRC is valid.



## Example Binary Record Layout

Below is a hex representation of a `SET` operation for key: `user` and value: `tim`.

```text
[ CRC32 ]  [ MAGIC ]   [TYPE]  [K_LEN]  [V_LEN]  [ KEY ]  [ VAL ]
7F 3A 2B 11 4B 56 53 31 01 00   04 00 00 00 03 00 00 00 75 73 65 72 74 69 6D
CRC32: 7F 3A 2B 11 (Calculated checksum)
MAGIC: 4B 56 53 31 (KVS1)
TYPE: 01 00 (SET)
KEY_LEN: 04 00 00 00 (4 bytes)
VAL_LEN: 03 00 00 00 (3 bytes)
```


##  CLI & Multiline Mode
The engine includes a robust CLI supporting complex data types:
* **Standard**: `SET key value`
* **Multiline**: Use the `<<` sentinel for JSON or logs.
    ```
    > SET my_config <<
    {
      "version": "1.0",
      "port": 8080
    }
    END
    ```





##  Getting Started

### Prerequisites
* C++17 or higher
* CMake (Recommended) or G++

### Installation & Build
```bash
git clone [https://github.com/DipHldr/AetherKV.git](https://github.com/DipHldr/AetherKV.git)
cd AetherKV
g++ -std=c++17 main.cpp -o kvstore 
OR
make run
```
## Basic Usage

Run the executable to enter the interactive CLI.

* **SET**: Enter a `Key` followed by the `Value`.
* **Multiline**: Supports pasting large blocks (terminated by your chosen sentinel like `END`).
* **EXIT**: Type `exit` to gracefully flush buffers and close the engine.



## Systems-Level Design Decisions

### Raw Binary I/O

Unlike standard text storage (`<<`), this engine uses `.write()` to dump RAM directly to disk. This bypasses costly ASCII conversions and ensures that a `uint32_t` always occupies exactly 4 bytes.

### Offset-Based Indexing

The engine utilizes `tellp()` and `seekg()` to manage pointers. By opening the file with `ios::ate`, the system determines the global byte offset immediately, ensuring the index is always accurate across sessions.



## Engineering Roadmap

### Phase 1: Storage Efficiency (Current Focus)
- [ ] **Background Compaction**: Implement a compaction worker to reclaim space by merging segments and removing shadowed keys/tombstones.
- [ ] **Bloom Filters**: Integrate per-segment Bloom Filters to eliminate unnecessary disk I/O for non-existent keys.

- [ ] **SSTables (Sorted String Tables)**: Transition from a pure log to sorted immutable segments to enable range queries and efficient merging.

### Phase 2: High Performance & Concurrency
- [ ] **Thread Safety**: Implement a Reader-Writer lock mechanism to support multi-threaded access.
- [ ] **Memory-Mapped I/O (mmap)**: Leverage `mmap` for kernel-level caching and faster data throughput.
- [ ] **Node.js Bindings**: Create an N-API wrapper to use AetherKV as a high-speed persistence layer for JavaScript backends.

### Phase 3: Distributed Evolution
- [ ] **LSM-Tree Tiering**: Organize segments into leveled tiers (Level-0 to Level-N) to optimize read/write amplification.

- [ ] **Networking Layer**: Encapsulate the engine in a TCP server using `io_uring` for high-concurrency remote access.
- [ ] **Replication**: Implement Leader-Follower WAL replication for high availability.


## Lessons Learned

During the development of AetherKV, several critical systems-level insights were gained:

* **I/O Behavior**: Deep-dived into `ios::app` vs `ios::ate`. Learned that `ios::app` forces every write to the end of the file regardless of `seekp()`, which is ideal for a Write-Ahead Log (WAL).
* **Binary vs. Text**: Validated that binary storage is not just about speed; it's about **fixed-size layouts**. Without ASCII formatting, byte offsets remain deterministic across different platforms.
* **Mechanical Sympathy**: Understood that `.write()` avoids the overhead of locale-aware formatting in `std::ostream`, allowing the CPU to move buffers directly to the OS page cache.
* **Durability Trade-offs**: Explored the balance between "Safe" (calling `fsync` after every write) and "Fast" (letting the OS manage the buffer), ultimately choosing an append-only design to mitigate corruption risks.


## Distributed System Concepts (Future Research)

### 1. Advanced Memory Hierarchy
As datasets scale beyond RAM, the engine will implement **Sparse Indexing**. Instead of mapping every key, we store "Fence Posts" (representative keys) for every 4KB block, allowing us to find data with minimal memory overhead.

### 2. Consistency & Sharding
To scale horizontally, AetherKV will utilize **Consistent Hashing** to distribute keys across a cluster. We are also exploring **Multi-Version Concurrency Control (MVCC)** to support snapshot isolation and ACID transactions without blocking reads.


##  License

Distributed under the MIT License. See `LICENSE` for more information.



