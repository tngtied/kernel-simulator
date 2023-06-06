#include "commands.c"


void cycle() {
	bool ker_exit_flag = false;

	//(1) wait time update
	result_status.cycle = cycle_num;

	//(2) new update
	// o N번째 cycle에 ready가 되는 프로세스의 종류는 다음과 같이 두가지가 있다.
	// ▪ (1) 프로세스 상태 갱신 단계에서 ready가 되는 프로세스 (New→Ready) ▪ (2)시스템콜또는폴트핸들러처리과정에서ready가되는프로세스
	// o (1)과 (2)는 서로 다른 시점에 ready queue에 삽입되므로, ‘동시’가 아님에 유의한다.
	//이거아직안함
	struct process* sw_ptr = statlist[2];
	while (sw_ptr != NULL && sw_ptr->status == 2) {
		enqueue(1, 1, dequeue(2));
	}

	//(3) terminated update-> at kernel execution
	if (check_exit()){ return ;}

	//(4) command execution
	if (kernel_mode) {
		int exec_comm;
		for (exec_comm = 0; exec_comm < 9; exec_comm++) {
			//boot, fork_and_exec, wait, exit, mem_alloc
			//mem_release, page_fault, protection_fault, 
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
		else if (exec_comm == 1) { //fork and exec
			char* temp_str = (char*)malloc(sizeof(char) * (strlen(statlist[0]->curr_comm) - 13));
			strncpy(temp_str, &(statlist[0]->curr_comm)[14], strlen(statlist[0]->curr_comm)-13 );

			fork_and_exec(statlist[0], temp_str);
			statlist[0]->child+=1;
			if (statlist[0]!=NULL){ nextline(statlist[0]); }
			enqueue(1, 1, dequeue(0));

			update_procstat(true, "system call");
			//no mode switch, leads to schedule/idle
		}
		else if (exec_comm == 2) { //wait
			wait();
			update_procstat(true, "system call");
		}
		else if (exec_comm == 3) { //exit

			sw_ptr = statlist[4];
			//waiting process ptr
			if (sw_ptr != NULL && check_ready(sw_ptr)) {
				sw_ptr->data=-1;
				nextline(sw_ptr);
				sw_ptr = sw_ptr->next;
				enqueue(1, 1, dequeue(4));
			}else{
				struct process* sw_bef_ptr = statlist[4];
				if (sw_ptr!=NULL){ sw_ptr=sw_ptr->next;}
				while (sw_ptr != NULL) {
					if (check_ready(sw_ptr)) {
						nextline(sw_ptr);
						sw_bef_ptr->next = sw_ptr->next;
						sw_ptr->next = NULL;
						enqueue(1, 1, sw_ptr);
						break;
					}
					else{
						sw_ptr=sw_ptr->next;
						sw_bef_ptr=sw_bef_ptr->next;
					}
				}
			}

			sw_ptr = statlist[3];
			ker_exit_flag = true;
			update_procstat(true, "system call");
		}
		else if (exec_comm == 4){//memory_allocate
			memory_allocate();
			nextline(statlist[0]);
			enqueue(1, 1, dequeue(0));
			update_procstat(true, "system_call");
		}
		else if (exec_comm==5){//memory release
			char temp_str[5];
			strncpy(temp_str, &(statlist[0]->curr_comm)[4], strlen(statlist[0]->curr_comm) - 11);
			int temp_i = atoi(temp_str);
			
			memory_release(temp_i);
			nextline(statlist[0]);
			enqueue(1,1,dequeue(0));
			update_procstat(true, "system call");
		}
		else if (exec_comm == 6){//page fault
			page_fault_handle();
			update_procstat(true, "fault");

			statlist[0]->data = -1;
			enqueue(1, 1, dequeue(0));
		}else if (exec_comm==7){//protection fault child
			protection_fault_handle_child();
			update_procstat(true, "fault");

			statlist[0]->data=-1;
			enqueue(1,1,dequeue(0));
		}
		else if (exec_comm==8){//protection fault parent
			protection_fault_handle_parent();
			update_procstat(true, "fault");

			statlist[0]->data=-1;
			enqueue(1,1,dequeue(0));
		}
		else {
			//exec_comm == 8
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
		else if (strncmp(statlist[0]->curr_comm, "fork_and_exec", 13) == 0) {
			kerflag[1] = true;
			kernel_mode = true;
			update_procstat(false, statlist[0]->curr_comm);
		}
		else if (strncmp(statlist[0]->curr_comm, "wait", 4) == 0) {
			update_procstat(false, statlist[0]->curr_comm);
			kernel_mode=true;
			kerflag[2] = true;
		}
		else if (strncmp(statlist[0]->curr_comm, "exit", 4) == 0) {
			kernel_mode = true;
			kerflag[3] = true;
			update_procstat(false, "exit");
		}
		else if (strncmp(statlist[0]->curr_comm, "memory_allocate", 15)==0){
		
			printf("copying: %s\n", statlist[0]->curr_comm);
			kerflag[4]=true;
			kernel_mode = true;
			update_procstat(false, statlist[0]->curr_comm);
		}
		else if (strncmp(statlist[0]->curr_comm, "memory_read", 11)==0){
			update_procstat(false, statlist[0]->curr_comm);
			char temp_str[5];
			strncpy(temp_str, &(statlist[0]->curr_comm)[4], strlen(statlist[0]->curr_comm) - 11);
			statlist[0]->data = atoi(temp_str);

			if (memory_read(statlist[0]->data)==1){
				kerflag[6]=true;
				//page fault flag
				kernel_mode = true;
			}else{
				statlist[0]->data = -1;
				nextline(statlist[0]);
			}
		}
		else if (strncmp(statlist[0]->curr_comm, "memory_release", 14)==0){
			kerflag[5]=true;
			kernel_mode = true; 
			update_procstat(false, statlist[0]->curr_comm);
		}
		else if (strncmp(statlist[0]->curr_comm, "memory_write", 12)==0){
			update_procstat(false, statlist[0]->curr_comm);

			char temp_str[5];
			strncpy(temp_str, &(statlist[0]->curr_comm)[4], strlen(statlist[0]->curr_comm) - 11);
			statlist[0]->data = atoi(temp_str);

			int temp_int = memory_write(statlist[0]->data);
			if (temp_int==0){
				statlist[0]->data=-1;
				nextline(statlist[0]);
			}else if (temp_int ==1){
				kerflag[7]=true;
				//protection fault flag
				kernel_mode = true;
			}else{
				kerflag[6]=true;
				//page fault flag
				kernel_mode = true;

			}
		}
	}
	print_cycle();
	if (kerflag[3]){ exit_virtual_proc(); }
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
	FILE * resultfile =  fopen("result", "w");
	fclose(resultfile);

	cycle_num = 0;
	kernel_mode = 1;
	min_pid = 1;
	frame_in_use = 0;

	printf("@@@@@@@@@@@@@@@111@@@@@@@@@launched, argc is %d\n", argc);
	printf("argv is %s, %s, %s\n", argv[0], argv[1], argv[2]);

	char* address_original = argv[0];
	char* address_input = argv[1];
	char* page_change_algo = argv[2];

	// char* address_input = "/testcase1";
	// char* page_change_algo = "fifo";

	if (strncmp(page_change_algo, "lru", 3)==0){frame_free_func = lru;}
	else if (strncmp(page_change_algo, "fifo", 4)==0){frame_free_func = fifo;}
	else if(strncmp(page_change_algo, "lfu", 3)==0){frame_free_func = lfu;}
	else if (strncmp(page_change_algo, "mfu", 3)==0){frame_free_func = mfu;}
	printf(", and done here\n");
	DIR* d = opendir(address_input);
	int path_len = strlen(address_input);

	if (!d) { 
		printf("directory %s not found, terminating \n", address_input);
		return 0; }
	for (int i = 0; i < 5; i++) { statlist[i] = NULL; }
	//initializing statlist, make sure data entry is null to indicate empty
	for(int i=0; i<16; i++){frame_table[i].using=false;}

	flist = (struct fimage*)malloc(sizeof(struct fimage));

	struct fimage* fimag_ptr = flist;
	bool fflag = false;
	struct dirent* usrprog_entry;
	while ((usrprog_entry = readdir(d)) != NULL) {
		//printf("file %s found \n", usrprog_entry->d_name);
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