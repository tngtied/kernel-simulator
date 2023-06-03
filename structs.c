#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

bool kernel_mode;
bool kerflag[8] = { false };
//boot, fork_and_exec, wait, exit, mem_alloc
//mem_release, page_fault, protection_fault, 
int cycle_num;
int min_pid;
//the smallest unused process id

int (*frame_free_func)();

struct page {
	bool using;
	int pid;
	//process id of the original process
    int fid;
    // -1 if no frame allocated
	int pgid;
	int allocation_id;
	bool write;
	struct proc_list *child_procs;
	//if false, read-only mode
};

struct frame{
	bool using;
    int made;
    int frequency;
    int recent;
	bool accessed;
	struct page* pg_ptr;
};
struct frame frame_table[16];
//frame table, used system-wide
int frame_in_use;
typedef struct proc_list;

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
	int min_pgdex;
	//minimum page index unused
};

struct proc_list{
	struct process * p;
	struct proc_list * next;
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
