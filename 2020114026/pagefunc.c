#include "printer.c"

int lru(){
    int ru = cycle_num;
    int cand_dex = -1;
    for (int i=0; i<16; i++){
        if (!frame_table[i].using || frame_table[i].made==cycle_num){continue;}
        if (ru>frame_table[i].recent){
            cand_dex = i;
            ru = frame_table[i].recent;
        }
    }
    return cand_dex;
}
int fifo(){
    printf("fifo called\n");
    int oldest_cycle = cycle_num;
    int cand_dex = -1; 
    for (int i=0; i<16; i++){
        //printf(" - - -  oldest cycle %d, %dth frame cycle %d\n", oldest_cycle, i, frame_table[i].made);
        if (!frame_table[i].using || frame_table[i].made == cycle_num){continue;}
        if (oldest_cycle>frame_table[i].made){
            cand_dex = i;
            oldest_cycle = frame_table[i].made;
        }
    }
    printf(" - - - fifo returns %d\n", cand_dex);
    return cand_dex;
}
int lfu(){
    int lf = cycle_num;
    int cand_dex = -1;
    for (int i=0; i<16; i++){
        if (!frame_table[i].using || frame_table[i].made == cycle_num){continue;}
        if (lf>frame_table[i].frequency){
            cand_dex = i;
            lf = frame_table[i].frequency;
        }
    }
    return cand_dex;
}
int mfu(){
    int mf = 0;
    int cand_dex = -1;
    for (int i=0; i<16; i++){
        printf(":: i %d frequency is %d\n", i, frame_table[i].frequency);
        if (!frame_table[i].using || frame_table[i].made == cycle_num){continue;}
        if (mf<frame_table[i].frequency){
            cand_dex = i;
            mf = frame_table[i].frequency;
        }
    }
    return cand_dex;
}