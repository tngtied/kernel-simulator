#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

bool kernel_mode;
bool kerflag[5] = { false };
//boot, fork_and_exec, wait, exit, mem_alloc
int cycle_num;
int min_pid;
//the smallest unused process id

struct page {
	bool using;
    int fid;
    // -1 if no frame allocated
	int pgid;
	int allocation_id;
};

struct frame{
	bool using;
    int made;
    int frequency;
    int recent;
	struct page* pg_ptr;
};
struct frame frame_table[16];
//frame table, used system-wide

struct process {
	char* name;
	int id;
	struct process* parent_proc;
	int status;
	//0: running, 1: ready, 2: new, 3: terminated, 4: waiting
	int child; //#of children processes
	char curr_comm[1024];
	int data;
	//additional data for some commands, -1 when uninitialized
	FILE* pFile;
	struct process* next; //needed to make linked list

	struct page * page_table[32];
	int min_pgid;
	int min_allocid;
	//minimum page/allocation id unused
};
struct process boot_instance;
typedef struct fimage fimage;
struct fimage {
	char* name;
	char* loc;
	int namelen;
	fimage* next;
};
struct fimage * flist;
//file read order

struct process* statlist[5] = { NULL };
//initialized in main(), array of struct pointers
//ready queue included in a linked list form
//0: running, 1: ready, 2: new, 3: terminated, 4: waiting

struct {
	int cycle;
	bool ker_mode;
	char* command;
	//has to be string pointer
}result_status;
