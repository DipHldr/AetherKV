#  Key–Value Storage Engine — CLI & API Documentation

## Overview

This project implements a lightweight persistent key–value storage engine written in C++.
It uses binary serialization and file offset indexing to store and retrieve values efficiently.

The engine supports both:

* CLI interaction
* Programmatic API access (YET TO IMPLEMENT)

---

## File Pointer Concepts

### Position Queries

```
tellp() → Returns current write pointer position
tellg() → Returns current read pointer position
```

### Pointer Movement

```
seekp() → Moves write pointer
seekg() → Moves read pointer
```

These are essential for:

* Appending records
* Locating stored values
* Offset-based indexing
* Efficient disk reads

---

## Supported Commands

```
***************************************
        COMMANDS TO IMPLEMENT       
---------------------------------------
  * SET key:value
  * GET key           → Retrieve value
  * DEL key           → Delete key
  * EXISTS key        → Check existence
  * KEYS              → List all keys
  * COMPACT           → Run compaction
  * STATS             → Database statistics
  * FLUSHALL          → Wipe database
***************************************
```

---

## CLI Multiline Mode

To store multiline values, use the sentinel syntax:

### Enter Multiline Mode

```
> SET key <<
```

### Provide Value

Enter multiple lines of content.

### Terminate Input

```
END
```

### Example

```
> SET notes <<
This is a multiline entry.
It spans several lines.
Useful for logs or configs.
END
```

---

## Retrieving Values

```
> GET key
```

Example:

```
> GET notes
```

---

## API Usage

For programmatic access:

```cpp
set(key, value);
```

The API bypasses CLI parsing and directly stores values.

Example:

```cpp
set("user", "tim");
```

---

## Example Test Cases

### Multiline Demo

```
SET demo << 
This is a demo multiline value.
It contains several lines of text.
You can store logs, configs, or notes like this.
END
```

---

### Special Characters

```
SET test1 <<
Special characters test:
!@#$%^&*()_+-=[]{}|;:',.<>/?
END
```

---

### JSON-style Content

```
SET test2 <<
JSON-like content:
{
  "user": {
    "id": 101,
    "profile": {
      "name": "Tim",
      "email": "tim@example.com",
      "skills": ["C++", "Distributed Systems", "Docker"],
      "education": {
        "degree": "B.Tech",
        "year": 2026,
        "university": {
          "name": "Tokyo Tech",
          "location": {
            "city": "Tokyo",
            "country": "Japan"
          }
        }
      }
    },
    "projects": [
      {
        "title": "KV Storage Engine",
        "tech_stack": {
          "language": "C++",
          "features": ["WAL", "CRC32", "Index Recovery"]
        }
      },
      {
        "title": "Video Processing Pipeline",
        "tech_stack": {
          "queue": "BullMQ",
          "storage": "MinIO",
          "transcoding": {
            "tool": "FFmpeg",
            "format": "HLS",
            "segments": [".ts", ".m3u8"]
          }
        }
      }
    ]
  }
}
END
```

---

## Design Notes

* Binary storage ensures predictable layout
* Offsets allow O(1) value lookup
* Multiline support avoids fragile delimiter parsing
* Append-only design prepares system for:

  * Tombstones
  * Compaction
  * LSM-style growth

---

## Future Enhancements

* Checksums for corruption detection
* Background compaction worker
* Binary index persistence
* Concurrent access support
* Memory-mapped IO optimization

---

## Notes

This project focuses on understanding:

* Serialization
* File pointer mechanics
* Storage engine fundamentals
* Disk-based indexing strategies

Built as a learning-first systems project.

---
