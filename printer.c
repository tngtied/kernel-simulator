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
