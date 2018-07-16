/***************************************************************************
 * *    Inf2C-CS Coursework 2: TLB and Cache Simulation
 * *
 * *    Instructor: Boris Grot
 * *
 * *    TA: Priyank Faldu
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
/* Do not add any more header files */

/*
 * Various structures
 */
typedef enum {tlb_only, cache_only, tlb_cache} hierarchy_t;
typedef enum {instruction, data} access_t;
const char* get_hierarchy_type(uint32_t t) {
    switch(t) {
        case tlb_only: return "tlb_only";
        case cache_only: return "cache-only";
        case tlb_cache: return "tlb+cache";
        default: assert(0); return "";
    };
    return "";
}

typedef struct {
    uint32_t address;
    access_t accesstype;
} mem_access_t;

// These are statistics for the cache and TLB and should be maintained by you.
typedef struct {
    uint32_t tlb_data_hits;
    uint32_t tlb_data_misses;
    uint32_t tlb_instruction_hits;
    uint32_t tlb_instruction_misses;
    uint32_t cache_data_hits;
    uint32_t cache_data_misses;
    uint32_t cache_instruction_hits;
    uint32_t cache_instruction_misses;
} result_t;


/*
 * Parameters for TLB and cache that will be populated by the provided code skeleton.
 */
hierarchy_t hierarchy_type = tlb_cache;
uint32_t number_of_tlb_entries = 0;
uint32_t page_size = 0;
uint32_t number_of_cache_blocks = 0;
uint32_t cache_block_size = 0;
uint32_t num_page_table_accesses = 0;


/*
 * Each of the variables (subject to hierarchy_type) below must be populated by you.
 */
uint32_t g_total_num_virtual_pages = 0;
uint32_t g_num_tlb_tag_bits = 0;
uint32_t g_tlb_offset_bits = 0;
uint32_t g_num_cache_tag_bits = 0;
uint32_t g_cache_offset_bits= 0;
result_t g_result;


/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access)
 * 2) 32-bit virtual memory address
 */
  mem_access_t read_transaction(FILE *ptr_file) {
    char buf[1002];
    char* token = NULL;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf, 1000, ptr_file)!=NULL) {

        /* Get the access type */
        token = strsep(&string, " \n");
        if (strcmp(token,"I") == 0) {
            access.accesstype = instruction;
        } else if (strcmp(token,"D") == 0) {
            access.accesstype = data;
        } else {
            printf("Unkown access type\n");
            exit(-1);
        }

        /* Get the address */
        token = strsep(&string, " \n");
        access.address = (uint32_t)strtol(token, NULL, 16);

        return access;
    }

    /* If there are no more entries in the file return an address 0 */
    access.address = 0;
    return access;
}

/*
 * Call this function to get the physical page number for a given virtual number.
 * Note that this function takes virtual page number as an argument and not the whole virtual address.
 * Also note that this is just a dummy function for mimicing translation. Real systems maintains multi-level page tables.
 */
uint32_t dummy_translate_virtual_page_num(uint32_t virtual_page_num) {
    uint32_t physical_page_num = virtual_page_num ^ 0xFFFFFFFF;
    num_page_table_accesses++;
    if ( page_size == 256 ) {
        physical_page_num = physical_page_num & 0x00FFF0FF;
    } else {
        assert(page_size == 4096);
        physical_page_num = physical_page_num & 0x000FFF0F;
    }
    return physical_page_num;
}

void print_statistics(uint32_t num_virtual_pages, uint32_t num_tlb_tag_bits, uint32_t tlb_offset_bits, uint32_t num_cache_tag_bits, uint32_t cache_offset_bits, result_t* r) {
    /* Do Not Modify This Function */

    printf("NumPageTableAccesses:%u\n", num_page_table_accesses);
    printf("TotalVirtualPages:%u\n", num_virtual_pages);
    if ( hierarchy_type != cache_only ) {
        printf("TLBTagBits:%u\n", num_tlb_tag_bits);
        printf("TLBOffsetBits:%u\n", tlb_offset_bits);
        uint32_t tlb_total_hits = r->tlb_data_hits + r->tlb_instruction_hits;
        uint32_t tlb_total_misses = r->tlb_data_misses + r->tlb_instruction_misses;
        printf("TLB:Accesses:%u\n", tlb_total_hits + tlb_total_misses);
        printf("TLB:data-hits:%u, data-misses:%u, inst-hits:%u, inst-misses:%u\n", r->tlb_data_hits, r->tlb_data_misses, r->tlb_instruction_hits, r->tlb_instruction_misses);
        printf("TLB:total-hit-rate:%2.2f%%\n", tlb_total_hits / (float)(tlb_total_hits + tlb_total_misses) * 100.0);
    }

    if ( hierarchy_type != tlb_only ) {
        printf("CacheTagBits:%u\n", num_cache_tag_bits);
        printf("CacheOffsetBits:%u\n", cache_offset_bits);
        uint32_t cache_total_hits = r->cache_data_hits + r->cache_instruction_hits;
        uint32_t cache_total_misses = r->cache_data_misses + r->cache_instruction_misses;
        printf("Cache:data-hits:%u, data-misses:%u, inst-hits:%u, inst-misses:%u\n", r->cache_data_hits, r->cache_data_misses, r->cache_instruction_hits, r->cache_instruction_misses);
        printf("Cache:total-hit-rate:%2.2f%%\n", cache_total_hits / (float)(cache_total_hits + cache_total_misses) * 100.0);
    }
}

/*
 *
 * Add any global variables and/or functions here as you wish.
 *
 */

 //additonal helper functions
 uint32_t log2(uint32_t inp) {
   uint32_t out = 0;
   out = log(inp)/log(2);
   return out;
 }

 //tlb
 typedef struct{
   uint32_t virtual_page_number;
   uint32_t physical_page_number;
   int      ranking;
 } tlbData;         //type for storing tlb

 int number_of_tlb_entries_int;
 int number_of_tlb_entries_int = (int) number_of_tlb_entries;

 int inTLB(uint32_t a, tlbData *b){ //to check if something is in lru
   uint32_t i;
   for (i = 0; i<number_of_tlb_entries; i++){
     if ((b[i]).virtual_page_number == a){
       return 1;
     }
   }
 return 0;
 }

 int LRUrank(uint32_t a, tlbData *b){ //which lru rank something is in
   uint32_t i;
   for (i = 0; i<number_of_tlb_entries; i++){
     if((b[i]).virtual_page_number==a){
       return (b[i]).ranking;
     }
   }
   return 0;
 }

//cache

typedef struct{
  uint32_t physical_address;
  uint32_t validBit;
} cacheData        //type for storing cache

int number_of_cache_blocks_int;
int number_of_cache_blocks_int = (int) number_of_cache_blocks;

int inCache(uint32_t physical_addr, cacheData *c){
  int i;
  for (int i = 0; i<number_of_cache_blocks_int; i++){
    if ((c[i]).physical_address == physical_addr & (&c[i]).validBit == 1){
      return 1;
    }
  }
  return 0;
}



int main(int argc, char** argv) {

    /*
     *
     * Read command-line parameters and initialize configuration variables.
     *
     */
    int improper_args = 0;
    char file[10000];
    if ( argc < 2 ) {
        improper_args = 1;
        printf("Usage: ./mem_sim [hierarchy_type: tlb-only cache-only tlb+cache] [number_of_tlb_entries: 8/16] [page_size: 256/4096] [number_of_cache_blocks: 256/2048] [cache_block_size: 32/64] mem_trace.txt\n");
    } else  {
        /* argv[0] is program name, parameters start with argv[1] */
        if ( strcmp(argv[1], "tlb-only") == 0 ) {
            if ( argc != 5 ) {
                improper_args = 1;
                printf("Usage: ./mem_sim tlb-only [number_of_tlb_entries: 8/16] [page_size: 256/4096] mem_trace.txt\n");
            } else {
                hierarchy_type = tlb_only;
                number_of_tlb_entries = atoi(argv[2]);
                page_size = atoi(argv[3]);
                strcpy(file, argv[4]);
            }
        } else if ( strcmp(argv[1], "cache-only") == 0 ) {
            if ( argc != 6 ) {
                improper_args = 1;
                printf("Usage: ./mem_sim cache-only [page_size: 256/4096] [number_of_cache_blocks: 256/2048] [cache_block_size: 32/64] mem_trace.txt\n");
            } else {
                hierarchy_type = cache_only;
                page_size = atoi(argv[2]);
                number_of_cache_blocks = atoi(argv[3]);
                cache_block_size = atoi(argv[4]);
                strcpy(file, argv[5]);
            }
        } else if ( strcmp(argv[1], "tlb+cache") == 0 ) {
            if ( argc != 7 ) {
                improper_args = 1;
                printf("Usage: ./mem_sim tlb+cache [number_of_tlb_entries: 8/16] [page_size: 256/4096] [number_of_cache_blocks: 256/2048] [cache_block_size: 32/64] mem_trace.txt\n");
            } else {
                hierarchy_type = tlb_cache;
                number_of_tlb_entries = atoi(argv[2]);
                page_size = atoi(argv[3]);
                number_of_cache_blocks = atoi(argv[4]);
                cache_block_size = atoi(argv[5]);
                strcpy(file, argv[6]);
            }
        } else {
            printf("Unsupported hierarchy type: %s\n", argv[1]);
            improper_args = 1;
        }
    }
    if ( improper_args ) {
        exit(-1);
    }
    assert(page_size == 256 || page_size == 4096);
    if ( hierarchy_type != cache_only) {
        assert(number_of_tlb_entries == 8 || number_of_tlb_entries == 16);
    }
    if ( hierarchy_type != tlb_only) {
        assert(number_of_cache_blocks == 256 || number_of_cache_blocks == 2048);
        assert(cache_block_size == 32 || cache_block_size == 64);
    }

    printf("input:trace_file: %s\n", file);
    printf("input:hierarchy_type: %s\n", get_hierarchy_type(hierarchy_type));
    printf("input:number_of_tlb_entries: %u\n", number_of_tlb_entries);
    printf("input:page_size: %u\n", page_size);
    printf("input:number_of_cache_blocks: %u\n", number_of_cache_blocks);
    printf("input:cache_block_size: %u\n", cache_block_size);
    printf("\n");

    /* Open the file mem_trace.txt to read memory accesses */
    FILE *ptr_file;
    ptr_file =fopen(file,"r");
    if (!ptr_file) {
        printf("Unable to open the trace file: %s\n", file);
        exit(-1);
    }

    /* result structure is initialized for you. */
    memset(&g_result, 0, sizeof(result_t));

    /* Do not delete any of the lines below.
     * Use the following snippet and add your code to finish the task. */

    /* You may want to setup your TLB and/or Cache structure here. */

    //declare and malloc for tlb array
    if(strcmp(argv[1], "tlb-only") == 0 ){
      tlbData *tlb;
      tlb = (tlbData *)malloc(number_of_tlb_entries*sizeof(tlbData);
    }

    //create array for cache
    //malloc for cache
    if(strcmp(argv[1], "cache-only") == 0){
      cacheData *cache;
      cache = (cacheData *)malloc(number_of_cache_blocks*sizeof(cacheData));
    }

    mem_access_t access;
    /* Loop until the whole trace file has been read. */
    while(1) {
        access = read_transaction(ptr_file);
        // If no transactions left, break out of loop.
        if (access.address == 0)
            break;
        /* Add your code here */

        g_total_num_virtual_pages = pow(2,(32-page_size));

        //tlb only
        if(hierachy_type==tlb_only){

          g_num_tlb_tag_bits = log2(number_of_tlb_entries);
          g_tlb_offset_bits = log2(page_size); //?

          //get virtual page number
          uint32_t virtual_page_number;
          virtual_page_number = access.address >> g_tlb_offset_bits;



          int k; //used in for loops

          //A hit
          if (inTLB(virtual_page_number, tlb)==1){
            if(access.accesstype==data){
               g_result.tlb_data_hits++;
            }
            else{
              g_result.tlb_instruction_hits++
            }
            int a = LRUrank(virtual_page_number, tlb) //rank associated to the current hit
            //for loop to -1 from the rankings that are higher than the current hit
            for (k = 0; k<number_of_tlb_entries_int; k++){
              if ((tlb[k]).ranking>a){
                (tlb[k]).ranking--;
              }
            }
            //set the rank of the one just hit to max
            for (k=0; k<number_of_tlb_entries_int; k++){
              if ((tlb[k]).virtual_page_number == virtual_page_number){
                (tlb[k]).ranking = number_of_tlb_entries - 1;
              }
            }
          }

          //A miss
          if (inTLB(virtual_page_number, tlb)==0){
            if(access.accesstype==data){
              g_result.tlb_data_misses++;
            }
            else{
              g_result.tlb_instruction_misses++;
            }
           //get physical page_size
           physical_page_number=dummy_translate_virtual_page_num(virtual_page_number);
           //for loop to -1 from the rest's rankings
           for (k = 0; k<number_of_tlb_entries_int; k++){
           (tlb[k]).ranking--;
           }
           //update tlb
           // when tlb full, replace lowest ranked(-1) tlb enrty with new tlb data
           for (k = 0; k<number_of_tlb_entries_int; k++){
             if ((tlb[k]).ranking==-1){
               (tlb[k]).physical_page_number = physical_page_number;
               (tlb[k]).virtual_page_number = virtual_page_number;
               (tlb[k]).ranking = number_of_tlb_entries_int-1;
             }
           }
          //tlb not full, add to next empty space in tlb data
           if(inTLB(virtual_page_number, lru)==0){
             for (k = 0; k<number_of_tlb_entries_int; k++){
               if ((tlb[k]).virtual_page_number = 0){
                 (tlb[k]).physical_page_number = physical_page_number;
                 (tlb[k]).virtual_page_number = virtual_page_number
                 (tlb[k]).ranking = number_of_tlb_entries_int-1;
                 break;  //end for loop
               }
               else {
                 continue;
               }
             }
           }
        //free tlb space
        //free(tlb);
        }



        //cache only
        if(hierarchy_type==cache_only){
            g_num_cache_tag_bits = 32 - (log2(number_of_cache_blocks) + log2(cache_block_size));  //32-(n + m + 2)
            g_cache_offset_bits = log2(cache_block_size);
            //get physical address
            uint32_t virtual_page_number;
            virtual_page_number = access.address >> g_cache_offset_bits; //virtual page is the virtual address shifted by the offset
            uint32_t physical_page_number;
            physical_page_number = dummy_translate_virtual_page_num(virtual_page_number);
            uint32_t physical_address;
            physical_address = (physical_page_number << g_cache_offset bits) + g_cache_offset_bits;



            int i; //used in for loops

           if (inCache(physical_address, cache) == 1){  //hit
             if(access.accesstype==data){
              g_result.cache_data_hits++;
           }
             else{
               g_result.cache_instruction_hits++;
             }
           }

           if (inCache(physical_address, cache) == 0){  //miss
             if(access.accesstype==data){
               g_result.cache_data_misses++;
             }
             else{
               g_result.cache_instruction_misses++;
             }

             /*uint32_t index;
             //index = log2(number_of_cache_blocks);

             //uint32_t pos;

              //pos = physical_address % number_of_cache_blocks

             for(i=0; i<number_of_cache_blocks; i++){
               if((&cache[i]).physical_address == 0){
                 (&cache[i]).validBit = 1;
                 (&cache[i]).physical_address = physical_address;
               }
             }
           }
           //free cache
           free(cache);*/
       }





    /*   //tlb and cache
       if(hierachy_type=tlb_cache){

         //get virtual page number
         uint32_t virtual_page_number;
         virtual_page_number = access.address >> g_tlb_offset_bits;
         //uint32_t physical_page_number;
         //physical_page_number =dummy_translate_virtual_page_num(virtual_page_number);
         uint32_t physical_address;
         //aphysical_address = (physical_page_number << g_cache_offset bits) | g_cache_offset_bits;

         //declare and malloc for tlb array
         tlbData *tlb;
         tlb = (tlbData *)malloc(number_of_tlb_entries*sizeof(tlbData);

         int k; //used in for loops

         //A hit on tlb
         if (inTLB(virtual_page_number, tlb)==1){
           if(access.accesstype==data){
              g_result.tlb_data_hits++;
           }
           else{
             g_result.tlb_instruction_hits++
           }
           int a = LRUrank(virtual_page_number, tlb) //rank associated to the current hit
           //for loop to -1 from the rankings that are higher than the current hit
           for (k = 0; k<number_of_tlb_entries_int; k++){
             if ((&tlb[k]).ranking>a){
               (&tlb[k]).ranking--;
             }
           }
           //set the rank of the one just hit to max
           for (k=0; k<number_of_tlb_entries_int; k++){
             if ((&tlb[k]).virtual_page_number == virtual_page_number){
               (&tlb[k]).ranking = number_of_tlb_entries - 1;
             }
           }
         }

         else {
           //A hit on cache
           if{
           }
           //A miss
           else{
           }
         }
       }*/


        /* Feed the address to your TLB and/or Cache simulator and collect statistics. */







    }

    if(strcmp(argv[1], "tlb-only") == 0 ){
      free(tlb);
    }

    if(strcmp(argv[1], "tlb-only") == 0 ){
      free(cache);
    }

    /* Do not modify code below. */
    /* Make sure that all the parameters are appropriately populated. */
    print_statistics(g_total_num_virtual_pages, g_num_tlb_tag_bits, g_tlb_offset_bits, g_num_cache_tag_bits, g_cache_offset_bits, &g_result);

    /* Close the trace file. */
    fclose(ptr_file);
    return 0;
}
