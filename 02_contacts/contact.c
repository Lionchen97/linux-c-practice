#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#define NAME_LENGTH 16
#define PHONE_LENGTH 32
#define BUFFER_LENGTH 128
#define MIN_TOKEN_LENGTH 5
#define INFO printf
// 头插
#define LIST_INSERT(item, list) do{   \
    item->prev = NULL;                \
    item->next = list;                \
    if(list) (list)->prev = item; \
    list = item;                      \
} while (0)
#define LIST_REMOVE(item,list) do{                       \
    if(item->prev !=NULL) item->prev->next = item->next; \
    if(item->next !=NULL) item->next->prev = item->prev; \
    if(list == item) list = item -> next;                \
    item->prev = item->next = NULL;                      \
}while(0)

struct person
{
    char name[NAME_LENGTH];
    char phone[PHONE_LENGTH];
    /* data */
    struct person *next;
    struct person *prev;
};

struct contacts {
    struct person *people;
    int count;
};
enum{
    OPER_INSERT = 1,
    OPER_PRINT,
    OPER_DELETE,
    OPER_SEARCH,
    OPER_SAVE,
    OPER_LOAD
};


// define interface
int person_insert(struct person **ppeople,struct person *ps){
    if(ps==NULL) return -1;
    LIST_INSERT(ps,*ppeople);
    return 0;
}
int person_delete(struct person **ppeople,struct person *ps){
    if(ps==NULL) return -1;
    LIST_REMOVE(ps,*ppeople);
    return 0;
}
struct person* person_search(struct person *people,const char *name){
    struct person *item = NULL;
    for(item = people;item!=NULL;item = item->next){
        if(!strcmp(name,item->name)) break;
    }
    return item;
}
int person_traversal(struct person *people){
    struct person *item = NULL;
    for(item=people;item !=NULL;item=item->next){
        INFO("name: %s,phone: %s\n",item->name,item->phone);
    }
    return 0;
}
// end
int insert_entry(struct contacts *cts){
    if(cts==NULL) return -1;
    struct person *p =(struct person*)malloc(sizeof(struct person));
    if(p==NULL) return -2;
    // name
    INFO("Pleasr Input Name: \n");
    scanf("%s",p->name);
    // phone
    INFO("Please Input Phone: \n");
    scanf("%s",p->phone);
    // add people
    if(0!=person_insert(&cts->people,p)){
        free(p);
        return -3;
    }
    cts->count ++;
    INFO("Insert Success\n");
    return 0;
}
int print_entry(struct contacts *cts){
    if(cts ==NULL)return -1;
    // cts->people
    person_traversal(cts->people);
    return 0;
}
int delete_entry(struct contacts *cts){
     if(cts==NULL) return -1;
     INFO("Please Input Name:\n");
     char name[NAME_LENGTH] = {0};
     scanf("%s",name);
     struct person *ps = person_search(cts->people,name);
     if(ps ==NULL){
        INFO("Person don't Exit\n");
        return -2;
     }
     //delete
     person_delete(&cts->people,ps);
     INFO("name:%s,phone:%s\n",ps->name,ps->phone);
     free(ps);
     cts->count--;
     INFO("Delete Success\n");
     return 0;
}
int search_entry(struct contacts* cts){
     if(cts==NULL) return -1;
     INFO("Please Input Name:\n");
     char name[NAME_LENGTH] = {0};
     scanf("%s",name);
     struct person *ps = person_search(cts->people,name);
     if(ps ==NULL){
        INFO("Person don't Exit\n");
        return -2;
     }
     INFO("name:%s,phone:%s\n",ps->name,ps->phone);

}
int save_file(struct person *people,const char *filename){
    FILE *fp = fopen(filename,"w");
    if(fp==NULL) return -1;
    struct person *item = NULL;
    for(item = people;item!=NULL;item = item->next){
        // 写到文件缓冲区
        fprintf(fp,"name: %s,phone: %s\n",item->name,item->phone);
        // 从缓冲区写道文件中
        fflush(fp);
    }
    fclose(fp);
    return 0;
}
int parser_token(char *buffer,int buffer_length,char *name,char *phone){
    if(buffer==NULL) return -1;
    if(buffer_length<MIN_TOKEN_LENGTH) return -2;
    int status = 0,i=0,j=0;
    for(i=0;buffer[i]!=',';i++){
        if(buffer[i]==' '){
            status=1;
        }
        else if(status==1){
            name[j++] = buffer[i];
        }
    }
    status=0,j=0;
    for(;i<buffer_length;i++){
        if(buffer[i]==' '){
            status=1;
        }
        else if (status==1)
        {
            phone[j++] = buffer[i];
        }
    }
    INFO("file token : %s --> %s",name,phone);//自动换行
    return 0;
}
int load_file(struct person **ppeople, int *count, const char *filename) {

  FILE *fp = fopen(filename, "r");
  if (fp == NULL) return -1;

  while (!feof(fp)) {

    char buffer[BUFFER_LENGTH] = {0};
    fgets(buffer, BUFFER_LENGTH, fp);

    char name[NAME_LENGTH] = {0};
    char phone[PHONE_LENGTH] = {0};

    if (parser_token(buffer, strlen(buffer), name, phone) != 0) {
      continue; 
    }

    struct person *ps = malloc(sizeof(struct person));
    if (ps == NULL) return -2;

    memcpy(ps->name, name, NAME_LENGTH);
    memcpy(ps->phone, phone, PHONE_LENGTH);
    INFO("%s",ps->name);
    person_insert(ppeople, ps);
    (*count)++;
  }

  fclose(fp);
  return 0;
}

int save_entry(struct contacts *cts){
    if(cts ==NULL) return -1;
    INFO("Please Input Save Filename:\n");
    char filename[NAME_LENGTH] = {0};
    scanf("%s",filename);
    save_file(cts->people,filename);
    INFO("%s Save Success\n",filename);
    return 0;
}

int load_entry(struct contacts *cts){
    if(cts==NULL) return -1;
    INFO("Please Input Load Filename:\n");
    char filename[NAME_LENGTH]={0};
    scanf("%s",filename);
    load_file(&cts->people,&cts->count,filename);
    return 0;
}

void menu_info(void){
    INFO("\n********************************************************\n");
    INFO("***** 1. Add Person\t\t2. Print People ********\n");
    INFO("***** 3. Del Person\t\t4. Search Person *******\n");
    INFO("***** 5. Save People\t\t6. Load People *********\n");
    INFO("***** Other Key for Exiting Program ********************\n");
    INFO("********************************************************\n\n");
}
int main(){
    struct contacts *cts = (struct contacts*)malloc(sizeof(struct contacts));
    if(cts==NULL) return -1;
    memset(cts,0,sizeof(struct contacts));
    while(1){
        menu_info();
        int select = 0;
        scanf("%d",&select);
        switch (select)
        {
        case OPER_INSERT:
            insert_entry(cts);
            break;
        case OPER_PRINT:
            print_entry(cts);
            break;
        case OPER_DELETE:
            delete_entry(cts);
            break;
        case OPER_SEARCH:
            search_entry(cts);
            break;
        case OPER_SAVE:
            save_entry(cts);
            break;
        case OPER_LOAD:
            load_entry(cts);
            break;
        default:
            goto exit;
            break;
        }
    }
exit:
    free(cts);
    return 0;
}