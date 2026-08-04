#include <stdio.h>
#include "wsoundoutdoor89.h"
#define TEST_RATE 44100U
#define TEST_MEM 12000U
static wsound89_i16 memory_a[TEST_MEM];
static wsound89_i16 memory_b[TEST_MEM];
int main(void){
    wsoundoutdoor89_context drywet;
    wsoundoutdoor89_context wetonly;
    wsound89_u32 i;
    wsound89_i16 a,b;
    int found=0;
    if(wsoundoutdoor89_init(&drywet,memory_a,TEST_MEM,TEST_RATE,WSOUNDOUTDOOR89_URBAN_CANYON)!=WSOUND89_OK)return 1;
    if(wsoundoutdoor89_init(&wetonly,memory_b,TEST_MEM,TEST_RATE,WSOUNDOUTDOOR89_URBAN_CANYON)!=WSOUND89_OK)return 2;
    a=wsoundoutdoor89_process_sample(&drywet,12000);
    b=wsoundoutdoor89_process_wet_sample(&wetonly,12000);
    if(a!=12000||b!=0)return 3;
    for(i=1;i<TEST_MEM;i++){
        a=wsoundoutdoor89_process_sample(&drywet,0);
        b=wsoundoutdoor89_process_wet_sample(&wetonly,0);
        if(a!=b)return 4;
        if(b!=0)found=1;
    }
    if(!found)return 5;
    printf("wsoundoutdoor89 wet-send smoke: PASS\n");
    return 0;
}
