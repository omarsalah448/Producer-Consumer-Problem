#include <iostream>
#include<string.h>
#include<sstream>
#include <stdio.h>
#include<random>
#include <sys/ipc.h>
#include <sys/shm.h>
#include<sys/types.h>
#include<sys/sem.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>
using namespace std;

typedef struct producer{
    int comm_id;
    char comm_name[20];
    double price;
}producer;

typedef struct buffer{
    int MAX_SIZE;
    producer prod[100];
    int first;
    int last;
}buffer;

void initializeBuffer(buffer *buff, int max_size){
   buff->MAX_SIZE = max_size;
   buff->first = -1;
   buff->last = -1;
}

void insertBuffer(buffer *buf, producer *prod) {
   if((buf->first == 0 && buf->last == buf->MAX_SIZE-1) || (buf->first == buf->last+1))
      fprintf(stderr,"buffer is full\n");
   else {
    // inserting first element
   if (buf->first == - 1){
      buf->first = 0;
      buf->last = 0;
   }
   else{
      if(buf->last == buf->MAX_SIZE-1)
         buf->last = -1;
      buf->last++;
   }
   buf->prod[buf->last].comm_id = prod->comm_id;
   strcpy(buf->prod[buf->last].comm_name, prod->comm_name);
   buf->prod[buf->last].price = prod->price;
   }
}
int copyAndRemove(buffer *buf, producer *prod){
   if(buf->first == - 1) {
      fprintf(stderr,"buffer is empty\n");
      return 0;
   } 
   prod->comm_id = buf->prod[buf->first].comm_id;
   strcpy(prod->comm_name, buf->prod[buf->first].comm_name);
   prod->price = buf->prod[buf->first].price;
   if(buf->first == buf->last){
      buf->first = -1;
      buf->last = -1;
   }
   else{
      if(buf->first == buf->MAX_SIZE-1)
         buf->first = -1;
      buf->first++;
   }
   return 1;
}
