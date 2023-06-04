#include "structs.c"

void enqueue(int liststat, int procstat, struct process* proc_in) {
	//destination status, process
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

bool check_exit() {
	for (int i = 0; i < 5; i++) { 
		if (statlist[i]!=NULL || kerflag[i]==true) { return false; }
	}
	return true;
}

bool check_ready(struct process* proc_in) {
	if (statlist[4] == NULL) { return false; }
	//if wait process doesn't exist

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


int KMP_pgtable(int i){
	//finds continuous unoccupied i pages (virtual memory)
	//in the page table of the current process
	bool available[32]= {false};
	int cand_start_dex = -1;
	bool cand_found = false;
	struct page ** pgtable_ptr = statlist[0]->page_table;
	for (int j=0; j<32-i+1; j++){
		if (pgtable_ptr[j]->using==false){
			if (cand_found==false){
				cand_start_dex = j;
				cand_found = true;
			}
			if (j - cand_start_dex == 1){return cand_start_dex;}
		}else {
			cand_found = false;
		}
	}
	//KMP (Knuth-Morris-Pratt) 참고
	//동일 process 내에서 할당 - deallocate 가 반복될 경우, 앞쪽의 빈 페이지에 새로 쓰는
	//경우가 발생할 수 있다. 그러므로 allocate 당시 malloc함수를 사용하고, 
	//dealloc시 free하기를 반복하는 것보다는
	//process fork 시 32개 page를 한번에 malloc, 이후 내부 정보 (pid, fid, pgid)
	//만 갱신해주는 방식으로
	//그렇다면 해당 struct page가 사용중인지 아닌지 나타내는 bool 멤버 변수 사용하기
}


int find_pg_start_dex(int i){
	if (statlist[0]->min_pgdex+i>32){ return (KMP_pgtable(i)); }
	else{ return statlist[0]->min_pgdex; }
}

int find_frame(){
	if (frame_in_use==16){
		return(frame_free_func());
	}else{
		frame_in_use++;
		//maybe i should change it to allocating from
		//the recent allocation.
		for (int i=0; i<16; i++){
			if (frame_table[i].using==false){return i;}
		}
	}
}

bool check_parent_page (struct page * parent){
	if (parent->child_procs==NULL){return false;}
	struct proc_list* cursor = parent->child_procs;
	while (true){
		if (cursor->p == statlist[0]){return true;}
		if (cursor->next!=NULL){cursor = cursor->next;}
		else{return false;}
	}
}

struct page* find_pg_by_pgid(int pgid_in, struct page**table){
	for (int i=0; i<32; i++){
		if (table[i]->pgid == pgid_in){return table[i];}
	}
	return NULL;
}