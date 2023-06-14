#include "pagefunc.c"

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
	temp_proc->name = (char*)malloc(sizeof(char)*(strlen(file_name))+1);

	strncpy (temp_proc->name, file_name, (sizeof(file_name)));

	//temp_proc->name[flist->namelen]='\0';
	temp_proc->id = min_pid;
	temp_proc->status = 2;
	temp_proc->parent_proc = parent_pin;
	printf("parent of %s is set to %s(%d)\n", temp_proc->name, parent_pin->name, parent_pin->id);
	printf("opening file located %s\n", fptr->loc);
	temp_proc->pFile = fopen(fptr->loc, "r");
	temp_proc->child = 0;

	temp_proc->min_pgid = parent_pin->min_pgid;
	temp_proc->min_allocid = parent_pin->min_allocid; 
	temp_proc->min_pgdex = parent_pin->min_pgdex;
	printf("forking %s, pid is %d\n", temp_proc->name, temp_proc->id);

	//CoW
	for (int i=0; i<32; i++){
		if (temp_proc->id!=1 && parent_pin->page_table[i]->using==true){
			temp_proc->page_table[i] = parent_pin->page_table[i];
			parent_pin->page_table[i]->write = false;

			enque_proclist(temp_proc, parent_pin->page_table[i]);
			parent_pin->page_table[i]->child_num++;
			// struct proc_list* child_ptr = parent_pin->page_table[i]->child_procs;
			// if (child_ptr == NULL){
			// 	parent_pin->page_table[i]->child_procs = (struct proc_list*)malloc(sizeof(struct proc_list));
			// 	parent_pin->page_table[i]->child_procs->p = temp_proc;
			// 	parent_pin->page_table[i]->child_procs->next = NULL;
			// }
			// else{
			// 	while (child_ptr->next!=NULL){child_ptr=child_ptr->next;}
			// 	child_ptr->next = (struct proc_list*)malloc(sizeof(struct proc_list));
			// 	child_ptr->next->p = temp_proc;
			// 	child_ptr->next->next = NULL;
			// }
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
	//page handle
	for (int i =0; i<32; i++){
		if (statlist[0]->page_table[i]->using==true){
			child_handle_on_release(statlist[0]->page_table[i], i);
			statlist[0]->page_table[i]->child_procs = NULL;
			if (statlist[0]->page_table[i]->fid != -1){
				//frame handle
				frame_table[statlist[0]->page_table[i]->fid].using = false;
				frame_in_use--;
			}
		}
	}

	statlist[0]->status = 3;
	enqueue(3, 3, dequeue(0));
}

void memory_allocate(){
	char temp_str[5];
	//fflush(stdout);
	
	strncpy(temp_str, &(statlist[0]->curr_comm)[16], strlen(statlist[0]->curr_comm) -15);
	int i = atoi(temp_str);

	int page_start_dex = find_pg_start_dex(i);
	struct page ** pgtable_ptr = statlist[0]->page_table;
	//i개의 available 한 frame 먼저 확보 
	free_frame(i);
	//후 page 할당
	int found_fid;

	for (int j=page_start_dex; j<page_start_dex+i; j++){

		//page 생성
		pgtable_ptr[j]->pid = statlist[0]->id;
		pgtable_ptr[j]->using = true;
		pgtable_ptr[j]->allocation_id = statlist[0]->min_allocid;//not updated properly
		pgtable_ptr[j]->pgid=statlist[0]->min_pgid;
		pgtable_ptr[j]->write = true;
		pgtable_ptr[j]->child_procs=NULL;
		pgtable_ptr[j]->child_num=0;
		statlist[0]->min_pgid++;

		printf("~~ allocated new page ~~ \n ^ pid %d\n", pgtable_ptr[j]->pid);
		printf(" ^ using %d, allocation id %d\n", pgtable_ptr[j]->using, pgtable_ptr[j]->allocation_id);
		printf(" ^ pgid %d, write %d, child num %d\n", pgtable_ptr[j]->pgid, pgtable_ptr[j]->write, pgtable_ptr[j]->child_num);

		//frame 할당
		found_fid=find_frame();
		//printf("found fid = %d using is %d\n", found_fid, frame_table[found_fid].using);

		pgtable_ptr[j]->fid = found_fid;
		
		frame_table[found_fid].using = true;
		frame_table[found_fid].pg_ptr = pgtable_ptr[j];
		frame_table[found_fid].made = cycle_num;
		frame_table[found_fid].frequency = 1;
		frame_table[found_fid].recent = cycle_num;

		printf("allocated frame table at %d\n", found_fid);

		statlist[0]->min_pgdex=j+1;
		
	}
	printf("allocate id %d\n", statlist[0]->min_allocid);
	statlist[0]->min_allocid++;
}
/*
			temp_proc->page_table[i]->using = true;
			temp_proc->page_table[i]->fid = parent_pin->page_table[i]->fid;
			temp_proc->page_table[i]->pgid = parent_pin->page_table[i]->pgid;
			temp_proc->page_table[i]->allocation_id = parent_pin->page_table[i]->allocation_id;
*/
void memory_release(int i){
	printf("<in release> target is %d\n", i);
	bool flag = false; //true if target page found
	struct page ** pgtable_ptr = statlist[0]->page_table;
	

	//page table handle
	for (int j=0; j<32; j++){ // find matching allocation id
		if (pgtable_ptr[j]->using && pgtable_ptr[j]->allocation_id==i){
			flag = true;
			
			child_handle_on_release(pgtable_ptr[j], j);
			printf("release target found at index %d\n", j);
			if (pgtable_ptr[j]->pid==statlist[0]->id){
				printf("  - it was owned by itself\n");
				//if it was owned by the calling process

				
				//frame handle
				if (pgtable_ptr[j]->fid!=-1){
					printf("fid wasn't -1, it was %d\n", pgtable_ptr[j]->fid);
					frame_table[pgtable_ptr[j]->fid].using = false;
					pgtable_ptr[j]->fid=-1;
					frame_in_use--;
				}
			}

			//original page table handle
			pgtable_ptr[j]->using = false;			
		}
		else if (flag){ break; }
	}
	return;
}

int memory_read(int i){
//return 0 if success
//return 1 if pagefault
	
	struct page * page_ptr = find_pg_by_pgid(i, statlist[0]->page_table);
	//find target page object

	printf("input i is %d, page pgid is %d\n", i, page_ptr->pgid);
	printf("read bool check %d, %d, %d\n", page_ptr->fid!=-1 ,frame_table[page_ptr->fid].using, frame_table[page_ptr->fid].pg_ptr == page_ptr);

	if ((page_ptr->fid!=-1 && frame_table[page_ptr->fid].using)&& 
		(frame_table[page_ptr->fid].pg_ptr == page_ptr || check_parent_page(frame_table[page_ptr->fid].pg_ptr))){
	//what if it is another page also inherited by the same parent process?
	//it isn't. page id is made sure above page ptr declaration
		frame_table[page_ptr->fid].frequency++;
		frame_table[page_ptr->fid].recent = cycle_num;
		return 0;
	}
	else{return 1;}
}

void page_fault_handle(){
	free_frame(1);
	int frame_dex = find_frame();
	if (frame_table[frame_dex].using == true){ frame_table[frame_dex].pg_ptr->fid = -1; }

	struct page * tar_pg_ptr=find_pg_by_pgid(statlist[0]->data, statlist[0]->page_table);
	//find target page object 

	frame_table[frame_dex].using = true;
	frame_table[frame_dex].made = cycle_num;
	frame_table[frame_dex].frequency = 1;
	frame_table[frame_dex].pg_ptr = tar_pg_ptr;
	tar_pg_ptr->write = true;
	tar_pg_ptr->fid = frame_dex;
}

int memory_write(int pgid){
	struct page * tar_pg_ptr = find_pg_by_pgid(statlist[0]->data, statlist[0]->page_table);

	if (tar_pg_ptr->fid == -1 || !frame_table[tar_pg_ptr->fid].using){
		//frame doesn't exist 
		//page fault
		return 2;
	}
	else if(frame_table[tar_pg_ptr->fid].pg_ptr->pid != statlist[0]->id){
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
		frame_table[tar_pg_ptr->fid].frequency++;
		frame_table[tar_pg_ptr->fid].recent=cycle_num;
		return 0;
	}
}

void protection_fault_handle_parent(){
	printf("cycle[%d] protection fault invoked by parent\n", cycle_num);
	struct page * original_pg = find_pg_by_pgid(statlist[0]->data, statlist[0]->page_table);
	original_pg->write = true;
	return;
}

void protection_fault_handle_child(){
	printf("cycle[%d] protection fault invoked by child\n", cycle_num);
	struct page * original_pg = find_pg_by_pgid(statlist[0]->data, statlist[0]->page_table);

	original_pg->write=true;
	//parent page write true

	int pgtable_dex;
	for (int i=0; i<32; i++){
		if (statlist[0]->page_table[i]==original_pg){
			pgtable_dex=i;
			break;
		}
	}

	//dequeue and free child process list element in parent page
	child_handle_on_release(original_pg, pgtable_dex);

	free_frame(1);
	int frame_dex = find_frame();
	if (frame_table[frame_dex].using == true){ frame_table[frame_dex].pg_ptr->fid = -1; }

	//frame allocation
	statlist[0]->page_table[pgtable_dex]->fid= frame_dex;
	frame_table[frame_dex].using = true;
	frame_table[frame_dex].made = cycle_num;
	frame_table[frame_dex].frequency = 1;
	frame_table[frame_dex].recent = cycle_num;
	frame_table[frame_dex].pg_ptr = statlist[0]->page_table[pgtable_dex];

	return;
}