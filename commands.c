#include "basefunc.c"

void schedule() {
	enqueue(0, 0, dequeue(1));
}//kernel mode, command 2

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
	struct process* temp_proc;

	while (fptr != NULL) {
		if (strcmp(file_name, fptr->name) == 0) { break; }
		fptr = fptr->next;
	}
	temp_proc = malloc(sizeof(struct process));
	temp_proc->name = (char*)malloc(sizeof(char)*(strlen(file_name)));

	strncpy (temp_proc->name, file_name, sizeof(file_name));

	temp_proc->name[flist->namelen]='\0';
	temp_proc->id = min_pid;
	temp_proc->status = 2;
	temp_proc->parent_proc = parent_pin;
	temp_proc->pFile = fopen(fptr->loc, "r");
	temp_proc->child = 0;

	temp_proc->min_pgid = parent_pin->min_pgid;
	temp_proc->min_allocid = parent_pin->min_allocid; 
	temp_proc->min_pgdex=parent_pin->min_pgdex;

	for (int i=0; i<32; i++){
		if (parent_pin->page_table[i]->using==true){
			temp_proc->page_table[i] = parent_pin->page_table[i];
			parent_pin->page_table[i]->write = false;

			struct proc_list* child_ptr = parent_pin->page_table[i]->child_procs;
			if (child_ptr == NULL){
				parent_pin->page_table[i]->child_procs = (struct proc_list*)malloc(sizeof(struct proc_list));
				parent_pin->page_table[i]->child_procs->p = temp_proc;
				parent_pin->page_table[i]->child_procs->next = NULL;
			}
			else{
				while (child_ptr->next!=NULL){child_ptr=child_ptr->next;}
				child_ptr->next = (struct proc_list*)malloc(sizeof(struct proc_list));
				child_ptr->next->p = temp_proc;
				child_ptr->next->next = NULL;
			}
		}else{
			temp_proc->page_table[i] = (struct page*)malloc(sizeof(struct page));
			temp_proc->page_table[i]->using=false;
		}
	}

	fseek(temp_proc->pFile, 0, SEEK_SET);
	temp_proc->data = -1;
	fgets(temp_proc->curr_comm, sizeof(temp_proc->curr_comm), temp_proc->pFile);
	temp_proc->curr_comm[strcspn(temp_proc->curr_comm, "\n")]=0;

	enqueue(2, 2, temp_proc);
	//enqueue to new
	min_pid++;
	return temp_proc;
	//parent process의 자식 기록 linked list에 넣기 위해서
}

void boot() {
	boot_instance.id = 0;
	fork_and_exec(&boot_instance, "init");
	//insert "init" in new process
	return;
}//kernel mode, command int 0

void exit_virtual_proc() {
	statlist[0]->status = 3;
	enqueue(3, 3, dequeue(0));
}

void memory_allocate(){
	char temp_str[3];

	strncpy(temp_str, &(statlist[0]->curr_comm)[16], strlen(statlist[0]->curr_comm) -15);
	int i = atoi(temp_str);

	int page_start_dex = find_pg_start_dex(i);
	struct page ** pgtable_ptr = statlist[0]->page_table;
	//i개의 available 한 frame 먼저 확보 
	free_frame(i);
	//후 page 할당

	for (int j=page_start_dex; j<page_start_dex+i;j++){

		//page 생성
		pgtable_ptr[j]->pid = statlist[0]->id;
		pgtable_ptr[j]->using = true;
		pgtable_ptr[j]->allocation_id = statlist[0]->min_allocid;
		pgtable_ptr[j]->pgid=statlist[0]->min_pgid;
		pgtable_ptr[j]->write = true;
		statlist[0]->min_pgid++;

		//frame 할당
		int found_fid=find_frame();
		if (frame_table[found_fid].using = true){
			struct page* original_page = frame_table[found_fid].pg_ptr;
			original_page->fid = -1;
		}
		pgtable_ptr[j]->fid = found_fid;

		frame_table[found_fid].pg_ptr = pgtable_ptr[j];
		frame_table[found_fid].made = cycle_num;
		frame_table[found_fid].frequency = 1;
		frame_table[found_fid].recent = cycle_num;
		statlist[0]->min_pgdex++;
		
	}
	statlist[0]->min_allocid++;
}
/*
			temp_proc->page_table[i]->using = true;
			temp_proc->page_table[i]->fid = parent_pin->page_table[i]->fid;
			temp_proc->page_table[i]->pgid = parent_pin->page_table[i]->pgid;
			temp_proc->page_table[i]->allocation_id = parent_pin->page_table[i]->allocation_id;
*/
void memory_release(int i){
	bool flag = false;
	struct page ** pgtable_ptr = statlist[0]->page_table;

	struct proc_list * child_start;
	struct proc_list * child_cursor;

	struct page ** child_pgtable_ptr;

	for (int j=0; j<32; j++){
		if (pgtable_ptr[j]->allocation_id==i){
			if (pgtable_ptr[j]->pid!=statlist[0]->id){
				pgtable_ptr[j] = (struct page*)malloc(sizeof(struct page*));
				pgtable_ptr[j]->using = false;
			}

			//child process page handle
			if (flag == false && pgtable_ptr[j]->child_procs !=NULL){ 
				child_start = pgtable_ptr[j]->child_procs;
			}
			if (child_start!=NULL){
				child_cursor = child_start;
				
				child_pgtable_ptr = child_cursor->p->page_table;
				child_pgtable_ptr[j] = (struct page*)malloc(sizeof(struct page));
				
				child_pgtable_ptr[j]->using = true;
				child_pgtable_ptr[j]->pid = child_cursor->p->id;
				child_pgtable_ptr[j]->pgid = pgtable_ptr[j]->pgid;
				child_pgtable_ptr[j]->allocation_id = pgtable_ptr[j]->pgid;
				child_pgtable_ptr[j]->child_procs = NULL;				
				child_pgtable_ptr[j]->write = true;
				
				while (child_cursor->next != NULL){
					child_cursor = child_cursor->next;

					child_pgtable_ptr = child_cursor->p->page_table;

					if (child_pgtable_ptr[j]->pid != child_cursor->p->id){
						child_pgtable_ptr[j] = (struct page*)malloc(sizeof(struct page));
						
						child_pgtable_ptr[j]->using = true;
						child_pgtable_ptr[j]->pid = child_cursor->p->id;
						child_pgtable_ptr[j]->pgid = pgtable_ptr[j]->pgid;
						child_pgtable_ptr[j]->allocation_id = pgtable_ptr[j]->pgid;
						child_pgtable_ptr[j]->child_procs = NULL;
						child_pgtable_ptr[j]->write = true;
					}
				}
			}

			//original page release handle
			//review again
			pgtable_ptr[j]->using = false;


			if (flag && pgtable_ptr[j]->child_procs!=NULL){
				struct proc_list * child_tofree = pgtable_ptr[j]->child_procs;
				struct proc_list * child_tofree_tracker = child_tofree;
				//if (child_tofree->next != NULL){child_tofree_tracker = pgtable_ptr[j]->child_procs;}

				while (child_tofree!=NULL){
					if (child_tofree_tracker->next != NULL){ child_tofree_tracker = child_tofree_tracker->next;}
					else{child_tofree_tracker = NULL;}

					child_tofree->p = NULL;
					child_tofree->next = NULL;
					free(child_tofree);

					if (child_tofree_tracker !=NULL){child_tofree = child_tofree_tracker;}
					else{child_tofree = NULL;}
				}
			}
			flag=true;
		}else if (flag==true){break;}
	}

	if (child_start!=NULL){

		while(child_start!=NULL){
			if (child_start->next!=NULL){child_cursor = child_start->next;}
			else {child_cursor = NULL;}

			child_start->p = NULL;
			child_start->next = NULL;
			free (child_start);
			
			if (child_cursor!=NULL){child_start = child_cursor;}
			else{child_start=NULL;}
		}

	}
	return;
}

int memory_read(int i){
//return 0 if success
//return 1 if pagefault
	
	struct page * page_ptr = find_pg_by_pgid(i, statlist[0]->page_table);
	//find target page object 

	if (frame_table[page_ptr->fid].using && 
		(frame_table[page_ptr->fid].pg_ptr == page_ptr || check_parent_page(frame_table[page_ptr->fid].pg_ptr))){
	//what if it is another page also inherited by the same parent process?
	
		frame_table[page_ptr->fid].accessed=true;
		frame_table[page_ptr->fid].frequency++;
		frame_table[page_ptr->fid].recent=cycle_num;
		return 0;
	}
	else{return 1;}
}

void page_fault_handle(){
	free_frame(1);
	int frame_dex = find_frame();

	struct page * tar_pg_ptr=find_pg_by_pgid(statlist[0]->data, statlist[0]->page_table);
	//find target page object 

	frame_table[frame_dex].using = true;
	frame_table[frame_dex].made = cycle_num;
	frame_table[frame_dex].frequency = 1;
	frame_table[frame_dex].accessed = true;
	frame_table[frame_dex].pg_ptr = tar_pg_ptr;
}

int memory_write(int pgid){
	struct page * tar_pg_ptr = find_pg_by_pgid(statlist[0]->data, statlist[0]->page_table);

	if (!frame_table[tar_pg_ptr->fid].using){
		//frame doesn't exist 
		//page fault
		return 2;
	}
	else if(frame_table[tar_pg_ptr->fid].pg_ptr != tar_pg_ptr){
		//frame exists, but not owned by itself
		//protection fault, child
		return 1;
	}
	else if (!tar_pg_ptr->write){
		//it exists, owned by itself, but write not permitted
		//protection fault, parent
		return 3;

	}
	else{
		//frame exists, owned by itself, write permitted
		frame_table[tar_pg_ptr->fid].accessed=true;
		frame_table[tar_pg_ptr->fid].frequency++;
		frame_table[tar_pg_ptr->fid].recent=cycle_num;
		return 0;
	}
}

void protection_fault_handle_parent(){
	struct page * original_pg = find_pg_by_pgid(statlist[0]->data, statlist[0]->page_table);
	original_pg->write = true;
}

void protection_fault_handle_child(){
	struct page * new_pg = (struct page*)malloc(sizeof(struct page*));
	struct page * original_pg = find_pg_by_pgid(statlist[0]->data, statlist[0]->page_table);

	new_pg->using = true;
	new_pg->pid = statlist[0]->id;
	new_pg->pgid = original_pg->pgid;
	new_pg->allocation_id = original_pg->allocation_id;
	new_pg->child_procs = NULL;

	free_frame(1);
	int frame_dex = find_frame();
	//frame allocation
	new_pg->fid= frame_dex;
	frame_table[frame_dex].using = true;
	frame_table[frame_dex].made = cycle_num;
	frame_table[frame_dex].frequency = 1;
	frame_table[frame_dex].recent = cycle_num;
	frame_table[frame_dex].pg_ptr = new_pg;

	for (int i=0; i<32; i++){
		if (statlist[0]->page_table[i]==original_pg){
			statlist[0]->page_table[i]=new_pg;
			return;
		}
	}
}