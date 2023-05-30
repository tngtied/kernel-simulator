#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

bool kernel_mode;
bool kerflag[5] = { false, false, false, false, false };
//boot, sleep, fork_and_exec, wait, exit
int cycle_num;
int pid;
//the smallest unused pid

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

void enqueue(int liststat, int procstat, struct process* proc_in) {
	//destination status (not sleep/wait), process
	proc_in->status = procstat;
	proc_in->next = NULL;
	if (statlist[liststat] == NULL) {
		statlist[liststat] = proc_in;
	}
	else {
		struct process* temp_ptr = statlist[liststat];
		while (temp_ptr->next != NULL) { temp_ptr = temp_ptr->next; }
		temp_ptr->next = proc_in;
	}
} //kernel mode, command int 2

struct process* dequeue(int j) {
	struct process* temp_ptr = statlist[j];
	if (statlist[j]->next==NULL){
		statlist[j]=NULL;}
	else{ statlist[j] = statlist[j]->next;}
	//if statlist[j]->next == NULL, then NULL is assigned to statlist[j],
	//notifying that no process is in status j 
	temp_ptr->next = NULL;
	return temp_ptr;
}//return process pointer

void nextline(struct process* in){
	fgets(in->curr_comm, sizeof(in->curr_comm), in->pFile);
	in->curr_comm[strcspn(in->curr_comm, "\n")]=0;
}

void schedule() {
	enqueue(0, 0, dequeue(1));
}//kernel mode, command 2
struct process* temp_proc;

bool check_exit() {
	for (int i = 0; i < 5; i++) { 
		if (statlist[i]!=NULL || kerflag[i]==true) { return false; }
	}
	return true;
}

void wait(){
	struct process* temp = dequeue(0);
	if (temp->child>0){
		temp->status=4;
		enqueue(4, 4, temp);
	}
	else{ 
		enqueue(1, 1, temp); 
		nextline(temp);
	}
}

void run() {
	if (statlist[0]->data == -1) {
		char temp_str[5];
		strncpy(temp_str, &(statlist[0]->curr_comm)[4], strlen(statlist[0]->curr_comm) - 3);
		statlist[0]->data = atoi(temp_str);
	}
	statlist[0]->data -= 1;
}


struct process* fork_and_exec(struct process* parent_pin, char* file_name) {
	//parent process pointer, file name
	fimage* fptr = flist;
	while (fptr != NULL) {
		if (strcmp(file_name, fptr->name) == 0) { break; }
		fptr = fptr->next;
	}
	temp_proc = malloc(sizeof(struct process));
	temp_proc->name = (char*)malloc(sizeof(char)*(strlen(file_name)));

	strncpy (temp_proc->name, file_name, sizeof(file_name));

	temp_proc->name[flist->namelen]='\0';
	temp_proc->id = pid;
	temp_proc->status = 2;
	temp_proc->parent_proc = parent_pin;
	temp_proc->pFile = fopen(fptr->loc, "r");
	temp_proc->child = 0;

	fseek(temp_proc->pFile, 0, SEEK_SET);
	temp_proc->data = -1;
	fgets(temp_proc->curr_comm, sizeof(temp_proc->curr_comm), temp_proc->pFile);
	temp_proc->curr_comm[strcspn(temp_proc->curr_comm, "\n")]=0;
	

	enqueue(2, 2, temp_proc);
	//enqueue to new
	pid++;
	return temp_proc;
	//parent process의 자식 기록 linked list에 넣기 위해서
}
struct process boot_instance;

void boot() {
	boot_instance.id = 0;
	fork_and_exec(&boot_instance, "init");
	//insert "init" in new process
	return;
}//kernel mode, command int 0


void sleep(int j) {
	//sleep time
	struct process* obj_proc = dequeue(0);
	obj_proc->status = 5;
	//process inner status: 5, to differenciate from wait
	obj_proc->data = j - 1;
	enqueue(4, 5, obj_proc);
	//queued in the same category with wait, for the order of ready queue
}

void exit_virtual_proc() {
	statlist[0]->status = 3;
	enqueue(3, 3, dequeue(0));
}

struct {
	int cycle;
	bool ker_mode;
	char* command;
	//has to be string pointer
	//run, ready, wait, new, terminated
}result_status;


bool check_ready(struct process* proc_in) {
	if (statlist[4] == NULL) { return false; }
	
	//if (wait or sleep) process doesn't exist
	if (proc_in->status == 5) {
		//if it was sleep
		if (proc_in->data == 0){return true;}
		else{
			proc_in->data-=1;
			return false;
		}
	 }
	//if process was sleeping and the timer is 0

	struct process* proc_term = statlist[3];
	//terminated process
	if (proc_in->status==4 && proc_in->child==0){return true;}
	while (proc_term != NULL) {
		if (proc_in->id == proc_term->parent_proc->id && proc_in->status==4) { return true; }
		proc_term = proc_term->next;
	}
	return false;

}

void update_procstat(bool mode_set, char* comm_in) {
	result_status.ker_mode = mode_set;
	if (result_status.command != NULL) { free(result_status.command); }
	result_status.command = (char*)malloc(sizeof(char) * (strlen(comm_in) + 1));
	strcpy(result_status.command, comm_in);
}

void print_cycle() {
	stdout = fopen("result", "a");
	struct process* print_ptr;
	if (result_status.cycle != 0) { 
		fflush(stdout);
		fprintf(stdout, "\n\n"); }

	//print cycle
	fflush(stdout);
	fprintf(stdout, "[cycle #%d]\n", result_status.cycle);

	//print mode
	fflush(stdout);
	fprintf(stdout, "1. mode: ");
	fflush(stdout);
	if (result_status.ker_mode) { fprintf(stdout, "kernel\n"); }
	else { fprintf(stdout, "user\n"); }

	//print command
	fflush(stdout);
	fprintf(stdout, "2. command: %s\n", result_status.command);

	//print running
	fflush(stdout);
	fprintf(stdout, "3. running: ");
	fflush(stdout);
	if (statlist[0] == NULL) { fprintf(stdout, "none\n"); }
	else { fprintf(stdout, "%d(%s, %d)\n", statlist[0]->id, statlist[0]->name, statlist[0]->parent_proc->id); }

	//print ready
	fflush(stdout);
	fprintf(stdout, "4. ready:");
	fflush(stdout);
	if (statlist[1] == NULL) { fprintf(stdout, " none\n"); }
	else {
		print_ptr = statlist[1];
		while (print_ptr != NULL) {
			fprintf(stdout, " %d", print_ptr->id);
			fflush(stdout);
			print_ptr= print_ptr->next;
		}
		fprintf(stdout, "\n");
	}
	fflush(stdout);

	//print waiting
	fprintf(stdout, "5. waiting:");
	fflush(stdout);
	if (statlist[4] == NULL) { fprintf(stdout, " none\n"); }
	else {
		print_ptr = statlist[4];
		char sw_char;
		while (print_ptr != NULL) {
			if (print_ptr->status == 4) { sw_char = "W"[0]; }
			else { sw_char = "S"[0]; }
			fprintf(stdout, " %d(%c)", print_ptr->id, sw_char);
			fflush(stdout);
			print_ptr=print_ptr->next;
		}
		fprintf(stdout, "\n");
	}
	fflush(stdout);

	//print new
	fprintf(stdout, "6. new:");
	fflush(stdout);
	if (statlist[2] == NULL) { fprintf(stdout, " none\n"); }
	else {
		print_ptr = statlist[2];
		while (print_ptr != NULL) {
			fprintf(stdout, " %d(%s, %d)", print_ptr->id, print_ptr->name, print_ptr->parent_proc->id);
			fflush(stdout);
			print_ptr=print_ptr->next;
		}
		fprintf(stdout, "\n");
	}
	fflush(stdout);

	//print terminated
	fprintf(stdout, "7. terminated:");
	fflush(stdout);
	if (statlist[3] == NULL) { 
		fprintf(stdout, " none"); 
	}
	else {
		print_ptr = statlist[3];
		while (print_ptr != NULL) {
			fprintf(stdout, " %d(%s, %d)", print_ptr->id, print_ptr->name, print_ptr->parent_proc->id);
			fflush(stdout);
			print_ptr=print_ptr->next;
		}
	}
	fclose(stdout);
}

void cycle() {
	bool ker_exit_flag = false;

	//(1) sleep/wait time update
	result_status.cycle = cycle_num;

	struct process* sw_ptr = statlist[4];
	while (sw_ptr != NULL && check_ready(sw_ptr)) {
		sw_ptr->data=-1;
		nextline(sw_ptr);
		sw_ptr = sw_ptr->next;
		enqueue(1, 1, dequeue(4));
	}

	struct process* sw_bef_ptr = statlist[4];
	if (sw_ptr!=NULL){ 	sw_ptr=sw_ptr->next;}
	while (sw_ptr != NULL) {
		if (check_ready(sw_ptr)) {
			nextline(sw_ptr);
			sw_bef_ptr->next = sw_ptr->next;
			sw_ptr->next = NULL;
			enqueue(1, 1, sw_ptr);
		}
		else{
			sw_ptr=sw_ptr->next;
			sw_bef_ptr=sw_bef_ptr->next;
		}
	}

	//(2) new update
	sw_ptr = statlist[2];
	while (sw_ptr != NULL && sw_ptr->status == 2) {
		enqueue(1, 1, dequeue(2));
	}

	//(3) terminated update-> at kernel execution
	if (check_exit()){ return ;}

	//(4) command execution
	if (kernel_mode) {
		int exec_comm;
		for (exec_comm = 0; exec_comm < 5; exec_comm++) {
			//boot, sleep, fork_and_exec, wait, exit
			if (kerflag[exec_comm] == true) {
				kerflag[exec_comm] = false;
				break;
			}
		}

		if (exec_comm == 0) {//boot
			boot();
			kernel_mode = true;
			update_procstat(true, "boot");
		}
		else if (exec_comm == 1) {//sleep
			update_procstat(true, "system call");
		}
		else if (exec_comm == 2) { //fork and exec
			char* temp_str = (char*)malloc(sizeof(char) * (strlen(statlist[0]->curr_comm) - 13));
			strncpy(temp_str, &(statlist[0]->curr_comm)[14], strlen(statlist[0]->curr_comm)-13 );

			fork_and_exec(statlist[0], temp_str);
			statlist[0]->child+=1;
			if (statlist[0]!=NULL){ nextline(statlist[0]); }
			enqueue(1, 1, dequeue(0));

			update_procstat(true, "system call");
			//no mode switch, leads to schedule/idle

		}
		else if (exec_comm == 3) { //wait
			wait();
			update_procstat(true, "system call");
		}
		else if (exec_comm == 4) { //exit
			sw_ptr = statlist[3];
			ker_exit_flag = true;
			update_procstat(true, "system call");

		}
		else {
			//exec_comm == 5
			if (statlist[1] == NULL) {
				update_procstat(true, "idle");
			}
			//프로그램 완전종료랑 구분가능하게만들기
			else {
				schedule();
				update_procstat(true, "schedule");
				kernel_mode = false;
			}
		}
	}
	else {
	//user mode
		if (strncmp(statlist[0]->curr_comm, "run", 3) == 0) {
			run();
			
			update_procstat(false, statlist[0]->curr_comm);
			if (statlist[0]->data == 0) {
				statlist[0]->data = -1;
				nextline(statlist[0]);
			}
			kernel_mode = false;
		}
		else if (strncmp(statlist[0]->curr_comm, "wait", 4) == 0) {
			update_procstat(false, statlist[0]->curr_comm);
			kernel_mode=true;
			kerflag[3] = true;
		}
		else if (strncmp(statlist[0]->curr_comm, "exit", 4) == 0) {
			kernel_mode = true;
			kerflag[4] = true;
			update_procstat(false, "exit");
		}
		else if (strncmp(statlist[0]->curr_comm, "sleep", 5) == 0) {
			update_procstat(false, statlist[0]->curr_comm);
			kerflag[1] = true;
		}
		else if (strncmp(statlist[0]->curr_comm, "fork_and_exec", 13) == 0) {
			kerflag[2] = true;
			kernel_mode = true;
			update_procstat(false, statlist[0]->curr_comm);
		}
	}
	print_cycle();
	if (kerflag[1]) {
		char temp_str[5];
		strncpy(temp_str, &(statlist[0]->curr_comm)[5], strlen(statlist[0]->curr_comm) - 4);
		int temp_data = atoi(temp_str);
		sleep(temp_data);
		//현재 프로세스를 sleep시키는데, 이러면 당연하게도 실제 s/w list에 영향이 간다.
		//이거 그냥 무식하게 밑에 밀자. if문 하나 더 넣어서
		kernel_mode = true;
	}
	if (kerflag[4]){ exit_virtual_proc(); }
	if (ker_exit_flag){
		while (sw_ptr != NULL && sw_ptr->status == 3) {
			sw_ptr->next = NULL;
			if (sw_ptr->parent_proc!=NULL){ sw_ptr->parent_proc->child-=1;}
			sw_ptr->parent_proc=NULL;
			sw_ptr->child = 0;
			if (sw_ptr->pFile!=NULL){ fclose(sw_ptr->pFile); }
			struct process * proc_tofree=dequeue(3);
			free(proc_tofree);
			sw_ptr = statlist[3];
		}
	}
	cycle_num++;
}

int main(int argc, char* argv[]) {
	stdout = fopen("result", "w");
	fclose(stdout);

	cycle_num = 0;
	kernel_mode = 1;
	pid = 1;
	char* address_original = argv[0];
	char* address_input = argv[1];
	
	DIR* d = opendir(address_input);
	int path_len = strlen(address_input);

	if (!d) { return 0; }

	struct dirent* usrprog_entry;

	for (int i = 0; i < 5; i++) { statlist[i] = NULL; }
	//initializing statlist, make sure data entry is null to indicate empty

	flist = (struct fimage*)malloc(sizeof(struct fimage));
	struct fimage* fimag_ptr = flist;
	bool fflag = false;
	while ((usrprog_entry = readdir(d)) != NULL) {
		if ((strcmp(usrprog_entry->d_name, ".")==0)|| (strcmp(usrprog_entry->d_name, "..")==0)){ continue; }

		if (fflag) { 
			fimag_ptr->next = (struct fimage*) malloc(sizeof(struct fimage)); 
			fimag_ptr = fimag_ptr->next;
		}

		fimag_ptr->name = (char*)malloc(sizeof(usrprog_entry->d_name + 1));
		fimag_ptr->loc = (char*)malloc(sizeof(char) * (strlen(address_input) + strlen(usrprog_entry->d_name) + 1));
		strcat(fimag_ptr->loc, address_input);
		strcat(fimag_ptr->loc, "/");
		strcat(fimag_ptr->loc, usrprog_entry->d_name);
		strcat(fimag_ptr->name, usrprog_entry->d_name);
		fimag_ptr->namelen = strlen(fimag_ptr->name);
		fimag_ptr->name[fimag_ptr->namelen]='\0';

		fflag = true;
	}

	kernel_mode = true;
	kerflag[0] = true;
	//boot signal

	cycle();
	while (!check_exit()) { cycle(); }
	//process id && loader read in with readdir has to be different => differenciate

	return 0;
}