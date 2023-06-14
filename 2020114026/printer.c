#include "basefunc.c"

void print_cycle() {
	FILE * resultfile =  fopen("fifo_result_2", "a");
	struct process* print_ptr;
	if (result_status.cycle != 0) { 
		//fflush(resultfile);
		fprintf(resultfile, "\n\n");
	}

	//print cycle
	fflush(resultfile);
	fprintf(resultfile, "[cycle #%d]\n", result_status.cycle);

	//print mode
	fflush(resultfile);
	fprintf(resultfile, "1. mode: ");
	fflush(resultfile);
	if (result_status.ker_mode) { fprintf(resultfile, "kernel\n"); }
	else { fprintf(resultfile, "user\n"); }

	//print command
	fflush(resultfile);
	fprintf(resultfile, "2. command: %s\n", result_status.command);

	//print running
	fflush(resultfile);
	fprintf(resultfile, "3. running: ");
	fflush(resultfile);
	if (statlist[0] == NULL) { fprintf(resultfile, "none\n"); }
	else { fprintf(resultfile, "%d(%s, %d)\n", statlist[0]->id, statlist[0]->name, statlist[0]->parent_proc->id); }

	//print physical memory
	fprintf(resultfile, "4. physical memory:\n");
	for(int i=0; i<4; i++){
		fprintf(resultfile, "|");
		for (int j =0; j<4; j++){
			if (j!=0){fprintf(resultfile, " ");}
			if (frame_table[i*4+j].using == false){fprintf(resultfile, "-");}
			else{fprintf(resultfile, "%d(%d)", frame_table[i*4+j].pg_ptr->pid, frame_table[i*4+j].pg_ptr->pgid);}
			//pid(pgid)
		}
	}
	fprintf(resultfile, "|");
	
	//print virtual memory
	if (statlist[0]!=NULL){
		fprintf(resultfile, "\n");

		fprintf(resultfile, "5. virtual memory:\n");
		for(int i=0; i<8; i++){
			fprintf(resultfile, "|");
			for (int j =0; j<4; j++){
				if (j!=0){fprintf(resultfile, " ");}
				if (statlist[0]->page_table[i*4+j]->using == false){fprintf(resultfile, "-");}
				else{fprintf(resultfile, "%d", statlist[0]->page_table[i*4+j]->pgid);}
				//pgid
			}
		}
		fprintf(resultfile, "|\n");


		fprintf(resultfile, "6. page table:\n");
		for (int i=0; i<2; i++){
			for (int j=0; j<8; j++){
				fprintf(resultfile, "|");
				for (int k =0; k<4; k++){
					if (k!=0){fprintf(resultfile, " ");}
					struct page * temp_page = statlist[0]->page_table[j*4+k];
					if (i==0){
						if (((frame_table[temp_page->fid].using==true)&&(frame_table[temp_page->fid].pg_ptr == temp_page)) && (frame_table[temp_page->fid].pg_ptr->pgid == temp_page->pgid)){
							//it can be parent's page
							fprintf(resultfile, "%d", temp_page->fid);
						}
						else{
							fprintf(resultfile, "-");
						}
						//fid
					}
					else{
						if (!temp_page->using){fprintf(resultfile, "-");}
						else if (temp_page->write){fprintf(resultfile, "W");}
						else{fprintf(resultfile, "R");}
					}
				}
			}
			fprintf(resultfile, "|");
			if (i==0){fprintf(resultfile, "\n");}
		}
	}

	fclose(resultfile);
}
