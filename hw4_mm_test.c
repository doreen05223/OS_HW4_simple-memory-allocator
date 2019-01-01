#include "lib/hw_malloc.h"
#include "hw4_mm_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(int argc, char *argv[])
{
    char input[30], ndInput[30];
    int N, num;
    void* ADDR;

    //if ctrl+D, break
    while(scanf("%s",input) != EOF) {
        if(strcmp(input,"alloc")==0) {
            scanf(" %d",&N);
            printf("0x%012llx\n",(long long int)hw_malloc(N));
            //printf("into alloc %d\n",N);
        } else if(strcmp(input,"free")==0) {
            scanf(" %p",&ADDR);
            //printf("into free %p\n",ADDR);
            if(hw_free(ADDR)==1) printf("success\n");
            else printf("fail\n");
        } else if(strcmp(input,"print")==0) {
            scanf(" %s",ndInput);
            if(strcmp(ndInput,"mmap_alloc_list")==0) {
                //printf("into mmap_alloc_list\n");
                print_mmap_alloc_list();
            } else {
                num=ndInput[4]-'0';
                print_bin(num);
                //printf("into bin %d\n",num);
            }
        }
    }

    return 0;
}
