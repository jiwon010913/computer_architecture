#include <stdio.h>
#include <stdlib.h>

// 제약조건
// 1. input 으로 주어지는 파일의 한줄은 100자를 넘지 않음.
// 2. input 으로 주어지는 파일의 길이는 100000줄을 넘지 않음. 즉, address trace 길이는 100000줄을 넘지 않음.
#define MAX_ROW     100000
#define MAX_COL     2
#define MAX_INPUT   100

// 전역변수
// 문제를 풀기 위한 힌트로써 제공된 것이며, 마음대로 변환 가능합니다.
enum COLS {
    MODE,
    ADDR
};

// LRU 알고리즘을 위한 자료구조
typedef struct {
    int valid;      // 유효 비트 (1: 유효, 0: 무효)
    int tag;        // 태그 비트
    int timestamp;  // 타임스탬프
    int dirty;      // (1: 일치하지 않음, 0: 일치함)
} CacheBlock;

/* 전역 변수 */
int i_total, i_miss;    /* 인스트럭션 캐시 접근 횟수 및 miss 횟수 */
int d_total, d_miss, d_write;   /* 데이터 캐시 접근 횟수 및 miss 횟수, 메모리 쓰기 횟수 */
int time_count; /* LRU를 구현하기 위한 시간 */
int total_set;
int trace[MAX_ROW][MAX_COL] = {{0,0},};
int trace_length = 0;

CacheBlock** instruction_cache;  // instruction cache 배열
CacheBlock** data_cache;         // data cache 배열

void solution(int cache_size, int block_size, int assoc);
void read_op(int addr, int cache_size, int block_size, int assoc);
void write_op(int addr, int cache_size, int block_size, int assoc);
void fetch_inst(int addr, int cache_size, int block_size, int assoc);
int find_lru_block(CacheBlock* cache, int assoc);
void update_timestamp(CacheBlock* cache, int assoc, int used_block);

int main(){
	// DO NOT MODIFY -- START --  //
    // cache size
    int cache[5] = {1024, 2048, 4096, 8192, 16384}; 
    // block size
    int block[2] = {16, 64};
    // associatvity e.g., 1-way, 2-way, ... , 8-way
    int associative[4] = {1, 2, 4, 8};
    int i=0,j=0,k=0; 

    /* 입력 받아오기 */
    char input[MAX_INPUT];
    while (fgets(input, sizeof(input), stdin)) {  
        if(sscanf(input, "%d %x\n", &trace[trace_length][MODE], &trace[trace_length][ADDR]) != 2) {
            fprintf(stderr, "error!\n");
        }
        trace_length++;
    }


    /* 캐시 시뮬레이션 */
    printf("cache size || block size || associative || d-miss rate || i-miss rate || mem write\n");
    for(i=0; i<5; i++){
      	for(j=0; j<2; j++){
            for(k=0; k<4; k++){
                solution(cache[i], block[j], associative[k]);
            }
        }
    }
	// DO NOT MODIFY -- END --  //
    return 0;
}

void solution(int cache_size, int block_size, int assoc) {
    /* 전역변수 값 초기화 */
    i_total=i_miss=0;
    d_total=d_miss=d_write=0;

    // Calculate the number of total_set
    total_set = cache_size / (block_size * assoc);

    // Create cache arrays for instruction cache and data cache dynamically
    instruction_cache = calloc(total_set, sizeof(CacheBlock*));;
    data_cache = calloc(total_set, sizeof(CacheBlock*));;

    for (int i = 0; i < total_set; i++) {
        instruction_cache[i] = calloc(assoc, sizeof(CacheBlock));
        data_cache[i] = calloc(assoc, sizeof(CacheBlock));
    }

	// DO NOT MODIFY -- START --  //
    int mode, addr;
    double i_miss_rate, d_miss_rate;    /* miss rate을 저장하는 변수 */

    int index = 0;
    while(index != trace_length) {
        mode = trace[index][MODE];
        addr = trace[index][ADDR];

        switch(mode) {
            case 0 :
                read_op(addr, cache_size, block_size, assoc);
                d_total++;	
                break;
            case 1 :
                write_op(addr, cache_size, block_size, assoc);
                d_total++;	
                break;
            case 2 :
                fetch_inst(addr, cache_size, block_size, assoc);
                i_total++;	
                break;
        }
        index++;
    }
   	// DO NOT MODIFY -- END --  //

    // Calculate miss rates
    i_miss_rate = (double)i_miss / (double)i_total;
    d_miss_rate = (double)d_miss / (double)d_total;
	
	// DO NOT MODIFY -- START --  //
    printf("%8d\t%8d\t%8d\t%.4lf\t%.4lf\t%8d\n", cache_size, block_size, assoc, d_miss_rate, i_miss_rate, d_write);
	// DO NOT MODIFY -- END --  //

    // Remember to free the allocated memory when you're done using it
    for (int i = 0; i < total_set; i++) {
        free(instruction_cache[i]);
        free(data_cache[i]);
    }
    free(instruction_cache);
    free(data_cache);
}

void read_op(int addr, int cache_size, int block_size, int assoc){
    int index = (addr / block_size) % total_set;
    int tag = (addr / block_size) * total_set;

    int i;
    for (i = 0; i < assoc; i++) {
        if (data_cache[index][i].valid && data_cache[index][i].tag == tag) {
            // Cache hit
            update_timestamp(data_cache[index], assoc, i);
            return;
        }
    }
    
    // Cache miss
    d_miss++;

    int lru_block = find_lru_block(data_cache[index], assoc);
    if (data_cache[index][lru_block].dirty) //이걸 왜 확인해? write back의 경우... dirty bit 확인해
        d_write++;  // Write back to memory if the block is dirty

    // Update cache with new block    
    data_cache[index][lru_block].valid = 1;
    data_cache[index][lru_block].tag = tag;
    data_cache[index][lru_block].dirty = 0;

    // Update timestamp for other blocks in the set
    update_timestamp(data_cache[index], assoc, lru_block);
	return;
}


void write_op(int addr, int cache_size, int block_size, int assoc){
    int index = (addr / block_size) % total_set;
    int tag = (addr / block_size) * total_set;

    int i;
    for (i = 0; i < assoc; i++) {
        if (data_cache[index][i].valid && data_cache[index][i].tag == tag) {
            // Cache hit
            update_timestamp(data_cache[index], assoc, i);
            data_cache[index][i].dirty  = 1;
            return;
        }
    }

    // Cache miss
    d_miss++;

    int lru_block = find_lru_block(data_cache[index], assoc);
    if (data_cache[index][lru_block].dirty) 
        d_write++;  // Write back to memory if the block is dirty

    // Update cache with new block
    data_cache[index][lru_block].valid = 1;
    data_cache[index][lru_block].tag = tag;
    data_cache[index][lru_block].dirty = 1;

    // Update timestamp for other blocks in the set
    update_timestamp(data_cache[index], assoc, lru_block);
	return;
}

void fetch_inst(int addr, int cache_size, int block_size, int assoc){
    int index = (addr / block_size) % total_set;
    int tag = (addr / block_size) * total_set;

    int i;
    for (i = 0; i < assoc; i++) {
        if (instruction_cache[index][i].valid && instruction_cache[index][i].tag == tag) {
            // Cache hit
            update_timestamp(instruction_cache[index], assoc, i);
            return;
        }
    }

    // Cache miss
    i_miss++;
        
    int lru_block = find_lru_block(instruction_cache[index], assoc);
    // Update cache with new block
    instruction_cache[index][lru_block].valid = 1;
    instruction_cache[index][lru_block].tag = tag;
    instruction_cache[index][lru_block].dirty = 0;

    // Update timestamp for other blocks in the set
    update_timestamp(instruction_cache[index], assoc, lru_block);
	return;
}

int find_lru_block(CacheBlock* cache, int assoc) {
    int lru_block = 0;
    int max_timestamp = cache[0].timestamp;
    int i;
    for (i = 1; i < assoc; i++) {
        if (cache[i].timestamp > max_timestamp) {
            lru_block = i;
            max_timestamp = cache[i].timestamp;
        }
    }
    return lru_block;
}

void update_timestamp(CacheBlock* cache, int assoc, int used_block) {
    int i;
    for (i = 0; i < assoc; i++) {
        if (i != used_block) {
            cache[i].timestamp++;
        }
    }
    cache[used_block].timestamp = 0;  
    // Set the timestamp of the used block to the most recently used
}
