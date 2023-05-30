#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>

bool kernel_mode;
bool kerflag[5] = { false, false, false, false, false };
//boot, sleep, fork_and_exec, wait, exit
int cycle_num;
int pid;
//the smallest unused pid

struct page {
    int made;
    int frequency;
    int recent;
}


struct process {
	char* name;
	int id;
	struct process* parent_proc;
	int status;
	//0: running, 1: ready, 2: new, 3: terminated, 4: waiting, 5, sleep
	int child; //#of children processes
	char curr_comm[1024];
	int data;
	//additional data for some commands, -1 when uninitialized
	FILE* pFile;
	struct process* next; //needed to make linked list
};

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
	//run, ready, wait, new, terminated
}result_status;
