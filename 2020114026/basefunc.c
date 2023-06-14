#include "structs.c"
#define min(a,b) (((a)<(b))?(a):(b))

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
} //kernel mode

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
}

void free_frame(int target){
	if (target+frame_in_use<=16){
		//frame_in_use = frame_in_use + target;
		return;
	}

	int j = target+frame_in_use-16;

	for (int i=0; i<j; i++){
		frame_in_use --;
		int freed_frame = frame_free_func();
		frame_table[freed_frame].using = false;
		frame_table[freed_frame].pg_ptr->fid = -1;
	}
	return;
}

int find_frame(){
	for (int i=0; i<16; i++){
		if (frame_table[i].using==false){
			frame_in_use++;
			return i;
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

void enque_proclist(struct process * target, struct page *parent_page){
	if (parent_page->child_num ==0){
		parent_page->child_procs = (struct proc_list*)malloc(sizeof(struct proc_list));
		parent_page->child_procs->p = target;
		parent_page->child_procs->next = NULL;
	}else{
		struct proc_list * cursor = parent_page->child_procs;
		for (int i=0; i<parent_page->child_num-1; i++){
			cursor= cursor->next;
		}
		cursor->next = (struct proc_list*)malloc(sizeof(struct proc_list));
		cursor->next->p = target;
		cursor->next->next = NULL;
	}
	return;
}

// void deque_proclist(struct process * target, struct page *start, int entries){
// 	//and free proc_list;
// 	struct proc_list * proc_tofree;
// 	if (start->child_procs->p == target){
// 		proc_tofree = start->child_procs;
// 		if (start->child_num!=1){ start->child_procs = start->child_procs->next; }
// 		else{
// 			start->child_procs = NULL;
// 			start->child_num=0;
// 		}
// 		proc_tofree->p = NULL;
// 		proc_tofree->next = NULL;
// 		free(proc_tofree);
// 		return;
// 	}

// 	struct proc_list * prev_entry = start->child_procs;
// 	struct proc_list * cursor_entry = start->child_procs->next;
// 	for (int i=0; i<entries; i++){
// 		if (cursor_entry->p == statlist[0]){
// 			proc_tofree = cursor_entry;
// 			prev_entry->next = cursor_entry->next;
// 			proc_tofree->p = NULL;
// 			proc_tofree->next = NULL;
// 			free(proc_tofree);
// 			start->child_num--;
// 			return;
// 		}
// 		cursor_entry = cursor_entry->next;
// 		prev_entry = prev_entry->next;
// 	}
// }

void child_handle_on_release(struct page * original_pg, int table_index){
	if (original_pg->child_num == 0){return;}
	else{
		struct proc_list * cursor_child = original_pg->child_procs;
		struct proc_list * next_child = cursor_child->next;

		struct page ** child_pgtable;
		for (int i = 0; i<original_pg->child_num; i++){
			//printf("  - loop %d\n", i);
			child_pgtable = cursor_child->p->page_table;
			child_pgtable[table_index] = (struct page*)malloc(sizeof(struct page));
			
			child_pgtable[table_index]->using = true;
			child_pgtable[table_index]->pid = cursor_child->p->id;
			child_pgtable[table_index]->pgid = original_pg->pgid;
			child_pgtable[table_index]->allocation_id = original_pg->pgid;
			child_pgtable[table_index]->fid = -1;
			child_pgtable[table_index]->child_procs = NULL;	
			child_pgtable[table_index]->child_num=0;		
			child_pgtable[table_index]->write = true;

			cursor_child->next = NULL;
			cursor_child->p = NULL;
			free(cursor_child);

			cursor_child =next_child;
			if (next_child!=NULL){ next_child=next_child->next;}
		}
		original_pg->child_procs = NULL;
		original_pg->child_num = 0;
		original_pg->write = true;
		return;
	}
}

