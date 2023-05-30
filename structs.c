#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

bool kernel_mode;
bool kerflag[5] = { false, false, false, false };
//boot, fork_and_exec, wait, exit
int cycle_num;
int pid;
//the smallest unused pid

struct page {
    int made;
    int frequency;
    int recent;
    int fid;
    // -1 if no frame allocated
    //pid of the page equals index in pages[] list
};
struct page pages[32];

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
	//run, ready, wait, new, terminated
}result_status;
