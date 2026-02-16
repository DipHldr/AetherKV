# C++ | High-Performance Persistent KV Store

A minimalist, high-performance Key-Value storage engine built in Modern C++ using an **Append-Only Log (AOL)** architecture. This project implements binary serialization, length-prefixed records, and an in-memory hash index for $O(1)$ disk access.



##  Features
* **Binary Storage:** Uses raw byte-stream I/O (`.write`/`.read`) for maximum performance and zero-copy overhead.
* **Length-Prefixed Serialization:** Handles complex, multi-line, and binary values without delimiter collisions.
* **O(1) Retrieval:** Maintains an in-memory `unordered_map` index storing byte offsets for instant data lookup.
* **Crash Resilience:** Append-only logic ensures that existing data is never overwritten during new writes.
* **Buffer Management:** Manual `.flush()` control to balance I/O throughput with data persistence.

---

##  Technical Architecture

The engine operates on a **Producer-Consumer** model between memory and disk:

1.  **The Index (Memory):** A hash table storing `std::string` keys and `long long` byte offsets.
2.  **The Log (Disk):** A `.bin` file where every entry follows the format:
    * `Key Length` (4-byte `uint32_t`)
    * `Key Data` (Raw bytes)
    * `Value Length` (4-byte `uint32_t`)
    * `Value Data` (Raw bytes/Opaque Blobs)



---

##  Getting Started

### Prerequisites
* C++17 or higher
* CMake (Recommended) or G++

### Installation & Build
```bash
git clone [https://github.com/yourusername/cpp-kv-store.git](https://github.com/yourusername/cpp-kv-store.git)
cd cpp-kv-store
g++ -std=c++17 main.cpp -o kvstore
```
## Basic Usage

Run the executable to enter the interactive CLI.

* **SET**: Enter a `Key` followed by the `Value`.
* **Multiline**: Supports pasting large blocks (terminated by your chosen sentinel like `END`).
* **EXIT**: Type `exit` to gracefully flush buffers and close the engine.

---

##  Performance Concepts Applied

### Formatted vs. Raw I/O

Unlike standard text storage (`<<`), this engine uses `.write()` to dump RAM directly to disk. This bypasses costly ASCII conversions and ensures that a `uint32_t` always occupies exactly 4 bytes.

### File Pointers & Seeking

The engine utilizes `tellp()` and `seekg()` to manage pointers. By opening the file with `ios::ate`, the system determines the global byte offset immediately, ensuring the index is always accurate across sessions.

---

##  Roadmap

* [ ] Implement Logging for crash failures

* [ ] **LSM-Tree Compaction**
  Implement a background worker to merge logs and remove stale keys.

* [ ] **Checksums**
  Add CRC32 validation to detect data corruption on disk.

* [ ] Actually Read from the binary file 

* [ ] **Thread Safety**
  Implement Reader-Writer locks for multi-threaded access.

* [ ] **Node.js Addon**
  Create a C++ binding to use this engine as a high-speed backend for Express apps.

---


##  License

Distributed under the MIT License. See `LICENSE` for more information.

---

##  Lessons Learned

* Discovered the behavior of `ios::app` and how it affects write pointer positioning.
* Learned why binary storage is preferable over text storage when precise offsets matter.
* Understood how raw `.write()` operations avoid formatting overhead and preserve fixed-size layouts.
* Explored persistence design tradeoffs between simplicity and robustness.

---

## Future Roadmap

The current implementation provides a foundation for a log-structured key–value storage engine. The following roadmap outlines the architectural direction required to evolve the system into a production-grade, distributed database platform.

---

### 1. Advanced Indexing & Memory Hierarchy

**Probabilistic Filtering (Bloom Filters)**
Future segments will include per-segment Bloom Filters to mitigate negative lookup costs. By consulting a compact in-memory bit array prior to disk access, the engine can definitively skip segments that cannot contain a requested key, significantly reducing read amplification and improving lookup latency.

**Sparse Indexing & Fence Posts**
As datasets scale, maintaining a full in-memory key index becomes impractical. The engine will transition to sparse indexing by storing “fence posts” — representative keys marking logical disk boundaries (e.g., every 4 KB block). This approach enables efficient disk range localization while minimizing memory footprint.

**Persistent Binary Index Snapshots**
To reduce startup recovery time, the MemTable index will be periodically serialized into compact binary snapshots. During restart, the engine can restore state in constant time and replay only the write-ahead log entries generated after the most recent snapshot.

---

### 2. Storage Lifecycle & Efficiency

**Background Compaction & LSM Tiering**
A multi-threaded compaction subsystem will be introduced to merge fragmented log segments into structured levels (e.g., Level-0 → Level-1). This process will:

* Eliminate stale key versions
* Remove tombstones
* Reclaim disk space
* Maintain read efficiency

This aligns the engine with Log-Structured Merge Tree (LSM) principles.

**Transition to SSTables**
Segments will evolve into Sorted String Tables (SSTables). A sorted MemTable structure (SkipList or AVL tree) will ensure flushed segments are ordered, enabling:

* Binary search lookup
* Efficient range queries
* Predictable disk access patterns

**Binary Compression**
Segment-level compression (Zstd or Snappy) will be integrated to optimize storage density while preserving acceptable I/O performance characteristics.

---

### 3. Distribution & High Availability

**Native Networking Layer**
The storage engine will be encapsulated within a high-performance C++ socket server leveraging `epoll` or `io_uring`. A custom binary protocol inspired by RESP will support remote clients and multiplexed connections.

**Leader–Follower Replication**
An asynchronous replication protocol will propagate WAL entries to follower nodes, enabling redundancy and fault tolerance. This design lays the groundwork for durability guarantees and failover mechanisms.

**Horizontal Sharding**
Consistent hashing will distribute keyspace across nodes, allowing the system to scale beyond a single machine and function as a distributed storage cluster.

---

### 4. Transactional Integrity

**ACID-Oriented Extensions**
Future research will explore transactional support through:

* Multi-Version Concurrency Control (MVCC)
* Two-Phase Commit coordination
* Snapshot isolation models

These mechanisms will allow atomic multi-key operations and stronger consistency guarantees across distributed components.

---

This roadmap reflects an ongoing effort to transform the project from a single-node experimental engine into a scalable, reliable distributed storage system through iterative architectural evolution and applied systems research.

