#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BLOCK_SIZE 256
#define NUM_BLOCKS 64
#define NUM_INODES 32
#define MAX_FILES 32
#define MAX_BLOCKS_PER_FILE 8
#define MAX_NAME_LEN 32

// File types
#define TYPE_FILE 1
#define TYPE_DIR 2

// Superblock structure
struct superblock {
    int num_blocks;
    int block_size;
    int num_inodes;
    int free_blocks;
    int free_inodes;
    int root_inode;
};

// Inode structure
struct inode {
    int inum;
    int type;
    int size;
    time_t ctime;
    time_t mtime;
    int blocks[MAX_BLOCKS_PER_FILE];
    int block_count;
};

// Directory entry structure
struct dirent {
    int inum;
    char name[MAX_NAME_LEN];
};

// Directory structure
struct directory {
    struct dirent entries[MAX_FILES];
    int count;
};

// File system structures
struct superblock sb;
struct inode inodes[NUM_INODES];
char data_blocks[NUM_BLOCKS][BLOCK_SIZE];
struct directory root_dir;

// Add directories array to track entries per inode (nested support)
struct directory directories[NUM_INODES];

// Path traversal and CLI command prototypes
int traverse_path(const char* path);
void cmd_touch(const char* path);
void cmd_mkdir(const char* path);
void cmd_ls(const char* path);
void cmd_tree(const char* path, int indent);
void cmd_read(const char* path);
void cmd_detail(const char* path);

// Function prototypes
void initialize_fs();
int create_inode(int type);
int allocate_block();
void create_root();
int find_inode_by_name(const char* name, int parent_dir);
void create_file(const char* name, const char* content);
void create_dir(const char* name);
void list_directory(const char* path);
void print_inode_info(int inum);
void print_fs_info();

int main() {
    initialize_fs();
    char line[1024];
    while (1) {
        printf("fs> "); if (!fgets(line,sizeof(line),stdin)) break;
        char cmd[16], arg[1024]={0}; int cnt = sscanf(line,"%15s %1023s",cmd,arg);
        if (cnt<1) continue;
        if (!strcmp(cmd,"touch")) cmd_touch(arg);
        else if (!strcmp(cmd,"mkdir")) cmd_mkdir(arg);
        else if (!strcmp(cmd,"ls")) cmd_ls(cnt>1?arg:"/");
        else if (!strcmp(cmd,"tree")) cmd_tree(cnt>1?arg:"/",0);
        else if (!strcmp(cmd,"read")) cmd_read(arg);
        else if (!strcmp(cmd,"detail")) cmd_detail(arg);
        else if (!strcmp(cmd,"exit")) break;
        else printf("Unknown command\n");
    }
    return 0;
}

void initialize_fs() {
    // Initialize superblock
    sb.num_blocks = NUM_BLOCKS;
    sb.block_size = BLOCK_SIZE;
    sb.num_inodes = NUM_INODES;
    sb.free_blocks = NUM_BLOCKS;
    sb.free_inodes = NUM_INODES;
    
    // Initialize inodes
    for (int i = 0; i < NUM_INODES; i++) {
        inodes[i].inum = -1; // Mark as free
        inodes[i].block_count = 0;
    }
    
    // Initialize root directory
    root_dir.count = 0;
    create_root();
    // Store root directory
    directories[sb.root_inode] = root_dir;
}

void create_root() {
    int root_inum = create_inode(TYPE_DIR);
    sb.root_inode = root_inum;
    
    // Add special directory entries
    struct dirent self = {root_inum, "."};
    struct dirent parent = {root_inum, ".."};
    root_dir.entries[root_dir.count++] = self;
    root_dir.entries[root_dir.count++] = parent;
}

int create_inode(int type) {
    if (sb.free_inodes <= 0) return -1;
    
    for (int i = 0; i < NUM_INODES; i++) {
        if (inodes[i].inum == -1) {
            inodes[i].inum = i;
            inodes[i].type = type;
            inodes[i].size = 0;
            inodes[i].ctime = time(NULL);
            inodes[i].mtime = inodes[i].ctime;
            inodes[i].block_count = 0;
            sb.free_inodes--;
            return i;
        }
    }
    return -1;
}

int allocate_block() {
    if (sb.free_blocks <= 0) return -1;
    
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (data_blocks[i][0] == 0) {
            data_blocks[i][0] = 1; // Mark as used
            sb.free_blocks--;
            return i;
        }
    }
    return -1;
}

// Traverse a POSIX path, return inode or -1
int traverse_path(const char* path) {
    if (strcmp(path, "/") == 0) return sb.root_inode;
    char buf[1024]; strncpy(buf, path, sizeof(buf));
    char* token = strtok(buf, "/");
    int curr = sb.root_inode;
    while (token) {
        struct directory* dir = &directories[curr];
        int found = -1;
        for (int i = 0; i < dir->count; i++) {
            if (strcmp(dir->entries[i].name, token) == 0) {
                found = dir->entries[i].inum;
                break;
            }
        }
        if (found < 0) return -1;
        curr = found;
        token = strtok(NULL, "/");
    }
    return curr;
}

// CLI command implementations
void cmd_touch(const char* path) {
    printf("Enter content (end with empty line):\n");
    char content[4096] = {0};
    char linebuf[1024];
    while (fgets(linebuf, sizeof(linebuf), stdin)) {
        if (strcmp(linebuf, "\n") == 0) break;
        strncat(content, linebuf, sizeof(content) - strlen(content) - 1);
    }
    create_file(path, content);
}
void cmd_mkdir(const char* path) { create_dir(path); }
void cmd_ls(const char* path) {
    int inum = traverse_path(path);
    if (inum < 0 || inodes[inum].type != TYPE_DIR) { printf("No such directory: %s\n", path); return; }
    struct directory* dir = &directories[inum];
    for (int i = 0; i < dir->count; i++) printf("%s\n", dir->entries[i].name);
}
void cmd_tree(const char* path, int indent) {
    int inum = traverse_path(path);
    if (inum < 0 || inodes[inum].type != TYPE_DIR) return;
    struct directory* dir = &directories[inum];
    for (int i = 0; i < dir->count; i++) {
        // skip . and ..
        if (!strcmp(dir->entries[i].name, ".") || !strcmp(dir->entries[i].name, "..")) continue;
        for (int j = 0; j < indent; j++) printf("  ");
        printf("%s\n", dir->entries[i].name);
        int child = dir->entries[i].inum;
        if (inodes[child].type == TYPE_DIR) {
            char sub[1024]; snprintf(sub, sizeof(sub), "%s/%s", path[strlen(path)-1]=='/'? path: path, dir->entries[i].name);
            cmd_tree(sub, indent+1);
        }
    }
}
void cmd_read(const char* path) {
    int inum = traverse_path(path);
    if (inum < 0 || inodes[inum].type != TYPE_FILE) { printf("Cannot read %s\n", path); return; }
    struct inode* f = &inodes[inum]; char buf[BLOCK_SIZE+1];
    for (int i = 0; i < f->block_count; i++) {
        int b = f->blocks[i]; int n = f->size - i*BLOCK_SIZE; n = n>BLOCK_SIZE?BLOCK_SIZE:n;
        memcpy(buf, data_blocks[b], n); buf[n]='\0'; printf("%s", buf);
    }
    printf("\n");
}
void cmd_detail(const char* path) {
    int inum = traverse_path(path);
    if (inum < 0) { printf("No such file or dir: %s\n", path); return; }
    print_inode_info(inum);
}

// Modify create_file for nested support
void create_file(const char* name, const char* content) {
    char pathbuf[1024]; strncpy(pathbuf, name, sizeof(pathbuf));
    char* base = strrchr(pathbuf, '/'); int parent = sb.root_inode;
    char fname[MAX_NAME_LEN];
    if (base) { *base='\0'; parent = traverse_path(pathbuf); strncpy(fname, base+1, MAX_NAME_LEN); }
    else strncpy(fname, name, MAX_NAME_LEN);
    if (parent<0) { printf("Invalid path\n"); return; }
    
    // Create new inode
    int file_inum = create_inode(TYPE_FILE);
    if (file_inum == -1) {
        printf("Error: No free inodes\n");
        return;
    }
    
    // Write content to data blocks
    size_t content_len = strlen(content);
    size_t remaining = content_len;
    const char* pos = content;
    
    while (remaining > 0) {
        int block = allocate_block();
        if (block == -1) {
            printf("Error: No free blocks\n");
            return;
        }
        
        inodes[file_inum].blocks[inodes[file_inum].block_count++] = block;
        size_t to_copy = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;
        memcpy(data_blocks[block], pos, to_copy);
        
        pos += to_copy;
        remaining -= to_copy;
        inodes[file_inum].size += to_copy;
    }
    
    inodes[file_inum].mtime = time(NULL);
    // add to directory array
    struct dirent new_entry = {file_inum, ""}; strncpy(new_entry.name, fname, MAX_NAME_LEN);
    directories[parent].entries[directories[parent].count++] = new_entry;
    printf("Created file '%s'\n", name);
}

// Modify create_dir for nested support
void create_dir(const char* name) {
    char pathbuf[1024]; strncpy(pathbuf, name, sizeof(pathbuf));
    char* base = strrchr(pathbuf, '/'); int parent = sb.root_inode;
    char dname[MAX_NAME_LEN];
    if (base) { *base='\0'; parent = traverse_path(pathbuf); strncpy(dname, base+1, MAX_NAME_LEN); }
    else strncpy(dname, name, MAX_NAME_LEN);
    if (parent<0) { printf("Invalid path\n"); return; }
    
    // Create new inode for directory
    int dir_inum = create_inode(TYPE_DIR);
    if (dir_inum == -1) {
        printf("Error: No free inodes\n");
        return;
    }
    
    // init new directory
    struct directory nd = {.count=0};
    struct dirent self={dir_inum, "."}, pr={parent, ".."}; nd.entries[nd.count++]=self; nd.entries[nd.count++]=pr;
    directories[dir_inum]=nd;
    
    // add to parent
    struct dirent ne={dir_inum, ""}; strncpy(ne.name, dname, MAX_NAME_LEN);
    directories[parent].entries[directories[parent].count++]=ne;
    printf("Created directory '%s'\n", name);
}

void list_directory(const char* path) { cmd_ls(path); }

void print_inode_info(int inum) {
    if (inum < 0 || inum >= NUM_INODES) {
        printf("Invalid inode number\n");
        return;
    }
    
    struct inode *i = &inodes[inum];
    printf("Inode number: %d\n", i->inum);
    printf("Type: %s\n", (i->type == TYPE_DIR) ? "Directory" : "File");
    printf("Size: %d bytes\n", i->size);
    printf("Created: %s", ctime(&i->ctime));
    printf("Modified: %s", ctime(&i->mtime));
    printf("Data blocks: ");
    for (int j = 0; j < i->block_count; j++) {
        printf("%d ", i->blocks[j]);
    }
    printf("\n");
}

void print_fs_info() {
    printf("Superblock Information:\n");
    printf("  Total blocks: %d\n", sb.num_blocks);
    printf("  Block size: %d bytes\n", sb.block_size);
    printf("  Free blocks: %d\n", sb.free_blocks);
    printf("  Total inodes: %d\n", sb.num_inodes);
    printf("  Free inodes: %d\n", sb.free_inodes);
    printf("  Root inode: %d\n", sb.root_inode);
}