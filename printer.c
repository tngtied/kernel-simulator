#include "structs.c"

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

	//print physical memory
	fprintf(stdout, "4. physical memory: \n");
	for(int i=0; i<4; i++){
		fprintf(stdout, "|");
		for (int j =0; j<4; j++){
			if (j!=0){fprintf(stdout, " ");}
			if (frame_table[i*4+j].using == false){fprintf(stdout, "-");}
			else{fprintf(stdout, "%d(%d)", frame_table[i*4+j].pg_ptr->pid, frame_table[i*4+j].pg_ptr->pgid);}
			//pid(pgid)
		}
	}
	fprintf(stdout, "|\n");
	
	//print virtual memory
	if (statlist[0]!=NULL){
		fprintf(stdout, "5. virtual memory:\n");
		for(int i=0; i<4; i++){
			fprintf(stdout, "|");
			for (int j =0; j<4; j++){
				if (j!=0){fprintf(stdout, " ");}
				if (frame_table[i*4+j].using == false){fprintf(stdout, "-");}
				else{fprintf(stdout, "%d", statlist[0]->page_table[i*4+j]->pgid);}
				//pgid
			}
		}
		fprintf(stdout, "|\n");


		fprintf(stdout, "5. page table:\n");
		for (int i=0; i<2; i++){
			for (int j=0; j<4; j++){
				fprintf(stdout, "|");
				for (int k =0; k<4; k++){
					if (k!=0){fprintf(stdout, " ");}
					struct page * temp_page = statlist[0]->page_table[j*4+k];
					if (i==0){
						if ((frame_table[temp_page->fid].pg_ptr == temp_page) && (frame_table[temp_page->fid].pg_ptr->pgid == temp_page->pgid)){
							//it can be parent's page
							fprintf(stdout, "%d", frame_table[i*4+j].pg_ptr->pid, frame_table[i*4+j].pg_ptr->pgid);
						}
						else{
							fprintf(stdout, "-");
						}
						//fid
					}
					else{
						if (!temp_page->using){fprintf(stdout, "-");}
						else if (temp_page->write){fprintf(stdout, "W");}
						else{fprintf(stdout, "R");}
					}
				}
			}
			fprintf(stdout, "|\n");
		}
	}

		fclose(stdout);
}
