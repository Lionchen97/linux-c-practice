#include<stdlib.h>
#include<stdio.h>
#define OUT 0
#define IN 1
#define INIT OUT
int splite(char c){
    if((' '==c)||('\n'==c)||('\t'==c)||('\"'==c)||('\''==c)||('+'==c)||('.'==c)||
    ('-'==c)||(','==c)){
        return 1;
    }
    return 0;
}
int count_word(char *filename){
    int status = INIT;
    int word=0;
    FILE *fp = fopen(filename,"r");
    if(fp == NULL)return -1;
    char c;
    while ((c=fgetc(fp))!=EOF)
    {
        if(splite(c)){
            status = OUT;
        }else if (OUT == status){
            status = IN;
            word++;
        }
    }
    return word;
}
int main(int argc,char *argv[]){
    if(argc<2)return -1;
    printf("word:%d\n",count_word(argv[1]));
    return 0;
}