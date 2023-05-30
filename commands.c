#include "structs.c"

void schedule() {
	enqueue(0, 0, dequeue(1));
}//kernel mode, command 2
struct process* temp_proc;

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