# Simple Linux-like Filesystem Simulation

This project implements a basic interactive command-line filesystem simulation in C. It mimics some core functionalities of a Linux filesystem, such as creating files and directories, listing contents, and viewing file details, all within a self-contained in-memory structure.

## Features

*   **Interactive CLI:** Provides a simple shell interface (`fs>`) to interact with the simulated filesystem.
*   **Hierarchical Directory Structure:** Supports creating directories within other directories (nested directories).
*   **File Creation with Content:** The `touch` command allows interactive input of multi-line content for new files.
*   **Directory and File Operations:**
    *   `touch <path>`: Creates a new file at the specified path. Prompts for content interactively. If path is not absolute, it's relative to the current working directory.
    *   `mkdir <path>`: Creates a new directory at the specified path. If path is not absolute, it's relative to the current working directory.
    *   `ls [path]`: Lists the contents (files and directories) of the specified path. Defaults to the current working directory if no path is provided.
    *   `tree [path]`: Displays a tree-like structure of files and directories starting from the specified path. Defaults to the current working directory if no path is provided.
    *   `read <path>`: Displays the content of the specified file. If path is not absolute, it's relative to the current working directory.
    *   `detail <path>`: Shows detailed inode information for the specified file or directory, including type, size, creation/modification times, and data block locations. If path is not absolute, it's relative to the current working directory.
    *   `cd <path>`: Changes the current working directory to the specified path. Supports relative (`.`, `..`) and absolute paths.
    *   `pwd`: Prints the current working directory path.
    *   `rm <path>`: Removes the specified file. If path is not absolute, it's relative to the current working directory.
    *   `rmdir <path>`: Removes the specified directory and its contents recursively. If path is not absolute, it's relative to the current working directory.
*   **Current Working Directory:** The shell maintains a current working directory, and commands operate relative to this directory unless an absolute path is given.
*   **In-Memory Simulation:** The entire filesystem (superblock, inodes, data blocks, directory structures) is simulated in memory and is lost when the program exits.
*   **Basic Filesystem Primitives:**
    *   Superblock: Manages overall filesystem metadata.
    *   Inodes: Store metadata for each file and directory.
    *   Data Blocks: Store file content and directory entries.
    *   Directory Entries: Link names to inodes within a directory.

## Building and Running

1.  **Prerequisites:** You'll need a C compiler (like GCC).
2.  **Compilation:**
    ```bash
    gcc main.c -o filesystem_sim
    ```
3.  **Running:**
    ```bash
    ./filesystem_sim
    ```
    This will start the interactive shell:
    ```
    fs>
    ```

## Usage Examples

```
fs> mkdir documents
Created directory 'documents'
fs> mkdir documents/reports
Created directory 'documents/reports'
fs> cd documents
fs> pwd
/documents
fs> touch reports/annual_q1.txt
Enter content (end with empty line):
This is the Q1 annual report.
It contains important financial data.

Created file 'reports/annual_q1.txt'
fs> ls
reports
.
..
fs> cd reports
fs> pwd
/documents/reports
fs> ls
annual_q1.txt
.
..
fs> cd /
fs> tree
documents
  reports
    annual_q1.txt
  .
  ..
.
..
fs> read documents/reports/annual_q1.txt
This is the Q1 annual report.
It contains important financial data.

fs> detail documents/reports/annual_q1.txt
Inode number: 3
Type: File
Size: 60 bytes
Created: Sat Jun 08 HH:MM:SS 2025
Modified: Sat Jun 08 HH:MM:SS 2025
Data blocks: 0 
fs> rm documents/reports/annual_q1.txt
Removed file 'documents/reports/annual_q1.txt'
fs> rmdir documents/reports
Removed directory 'documents/reports'
fs> ls documents
.
..
fs> exit
```

## Filesystem Internals (Simplified)

*   **Superblock (`struct superblock`):** Contains global information like total blocks, block size, total inodes, free blocks/inodes, and the root inode number.
*   **Inodes (`struct inode`):** Each file or directory has an inode. It stores metadata:
    *   `inum`: Inode number (identifier).
    *   `type`: `TYPE_FILE` or `TYPE_DIR`.
    *   `size`: Size of the file in bytes.
    *   `ctime`, `mtime`: Creation and modification timestamps.
    *   `blocks[]`: Array of data block numbers where the file's content (or directory's entries) are stored.
    *   `block_count`: Number of data blocks used.
*   **Directory Entry (`struct dirent`):** Represents an entry within a directory.
    *   `inum`: Inode number of the file/directory this entry points to.
    *   `name[]`: Name of the file/directory.
*   **Directory Structure (`struct directory`):** An array of `dirent` structures.
    *   `entries[]`: List of files and subdirectories.
    *   `count`: Number of entries in the directory.
    *   Each directory implicitly contains `.` (self) and `..` (parent) entries.
*   **Data Blocks (`data_blocks[][]`):** A 2D char array representing the raw storage space. Used to store file content and serialized directory structures.
*   **Global Arrays:**
    *   `inodes[]`: Array holding all inode structures.
    *   `directories[]`: Array holding the directory structure for each inode that is a directory. This enables nested directory support.
    *   `cwd_inode`: Integer storing the inode number of the current working directory.
    *   `cwd_path[]`: Character array storing the string path of the current working directory.

## Limitations

*   **In-Memory:** The filesystem is not persistent. All data is lost when the program terminates.
*   **Fixed Sizes:** The number of inodes, blocks, maximum files per directory, maximum blocks per file, and maximum name length are defined by macros and are fixed at compile time.
*   **No Permissions:** Does not implement file/directory permissions.
*   **Limited Path Resolution:** Path traversal is basic and assumes well-formed paths. While `cd` handles `.` and `..`, complex relative paths might not be fully robust.
*   **Error Handling:** Error handling is basic (e.g., "No free inodes", "Invalid path").
*   **Single User:** Designed for a single interactive session.
*   **Block Allocation:** `allocate_block` simply marks the first byte of a block as used, which is a very simplistic way to track usage.
*   **Directory Data Storage:** For directories, the `struct directory` itself is copied into a data block. This is a simplification.
