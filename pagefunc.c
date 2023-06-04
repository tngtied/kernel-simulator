#include "structs.c"

int lru(){
    int ru = cycle_num;
    int cand_dex = -1;
    for (int i=0; i<16; i++){
        if (frame_table[i].made==cycle_num){continue;}
        if (ru>frame_table[i].recent){
            cand_dex = i;
            ru = frame_table[i].recent;
        }
    }
    return cand_dex;
}
int fifo(){
    int oldest_cycle = cycle_num;
    int cand_dex = -1; 
    for (int i=0; i<16; i++){
        if (frame_table[i].made == cycle_num){continue;}
        if (oldest_cycle>frame_table[i].made){
            cand_dex = i;
            oldest_cycle = frame_table[i].made;
        }
    }
    return cand_dex;
}
int lfu(){
    int lf = cycle_num;
    int cand_dex = -1;
    for (int i=1; i<16; i++){
        if (frame_table[i].made == cycle_num){continue;}
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
    for (int i=1; i<16; i++){
        if (frame_table[i].made == cycle_num){continue;}
        if (mf<frame_table[i].frequency){
            cand_dex = i;
            mf = frame_table[i].frequency;
        }
    }
    return cand_dex;
}