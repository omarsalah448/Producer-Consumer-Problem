#include "sharedCode.cpp"

#define CURRENCY_COLUMN 3
#define PRICE_COLUMN 19
#define AVG_PRICE_COLUMN 30
#define LAST_COLUMN 40

typedef struct commodity{
    char comm_name[20];
    double price;
    double reading1;
    double reading2;
    double reading3;
    double reading4;
} commodity;
void initialzieOneCommodity(commodity *item, char comm_name[]){
    strcpy(item->comm_name, comm_name);
    item->price=0;
    item->reading1=0;
    item->reading2=0;
    item->reading3=0;
    item->reading4=0;
}
void initialzieCommodities(commodity *items[]){
    for(int i=0;i<11;i++)
        items[i] = (commodity *) malloc(sizeof(commodity));
    initialzieOneCommodity(items[0], (char*)"ALUMINIUM");
    initialzieOneCommodity(items[1], (char*)"COPPER");
    initialzieOneCommodity(items[2], (char*)"COTTON");
    initialzieOneCommodity(items[3], (char*)"CRUDEOIL");
    initialzieOneCommodity(items[4], (char*)"GOLD");
    initialzieOneCommodity(items[5], (char*)"LEAD");
    initialzieOneCommodity(items[6], (char*)"MENTHAOIL");
    initialzieOneCommodity(items[7], (char*)"NATURALGAS");
    initialzieOneCommodity(items[8], (char*)"NICKEL");
    initialzieOneCommodity(items[9], (char*)"SILVER");
    initialzieOneCommodity(items[10], (char*)"ZINC");
}
double getAvergePrice(commodity *item);
void printTable(commodity *items[]);
void assignPrices(commodity *item, double price);

void signal_handler(int sig){
	signal(SIGINT, signal_handler);
    fflush(stdout);
    return;
}
void getOut(char *sharedSpace,int s_id,int shmid){
    shmdt(sharedSpace); 
    semctl(s_id, 0, IPC_RMID);
    shmctl(shmid,IPC_RMID,NULL);
    cout <<"goodbye...\n";
    exit(1);
}
int main(int argc, char *argv[]){
    int BUFFER_SIZE = atoi(argv[1]);
    char *sharedSpace;
    producer *cons = (producer *)malloc(sizeof(producer));
    commodity *item = (commodity *)malloc(sizeof(commodity));
    commodity *items[11];
    initialzieCommodities(items);
    int shmid, err;
    key_t key = ftok("keyFile",65);
    if (key == -1) {
            perror("ftok");
            exit(1);
        }
    //create a buffer
    buffer *buff = (buffer *)malloc(sizeof(buffer));
    initializeBuffer(buff, BUFFER_SIZE);
    // create the semaphores
    struct sembuf semaphore[3];
    int s_id = semget(key, 3, 0660|IPC_CREAT);
    if (s_id == -1){
        perror ("create semget"); 
        exit(1);
    }
    if(s_id == -1){
        perror("semget");
        exit(1);
    }
    // create a union for semctl function
    union sem_attr{
        int val;
        struct semid_ds *buf;
        ushort array[3];
    } sem_attr;

    // s semaphore to enforce mutual exclusion
    semaphore[0].sem_flg=0;
    semaphore[0].sem_num=0;
    semaphore[0].sem_op=0;
    sem_attr.val=1;
    
    err = semctl(s_id, 0, SETVAL, sem_attr);
    if (err == -1){
        perror ("s-semctl:SETVAL");
        exit(1);
    }
    /* n semaphore to make sure consumer doesn't
    consume from empty buffer */
    semaphore[1].sem_flg=0;
    semaphore[1].sem_num=1;
    semaphore[1].sem_op=0;
    sem_attr.val=0;
    
    err = semctl(s_id, 1, SETVAL, sem_attr);
    if (err == -1){
        perror ("n-semctl:SETVAL");
        exit(1);
    }
    /* e semaphore to make sure producer doesn't
    produce to full buffer */
    semaphore[2].sem_flg=0;
    semaphore[2].sem_num=2;
    semaphore[2].sem_op=0;
    sem_attr.val=BUFFER_SIZE;
  
    err = semctl(s_id, 2, SETVAL, sem_attr);
    if (err == -1){
        perror ("e-semctl:SETVAL");
        exit(1);
    }
    shmid = shmget(key,sizeof(producer),0666|IPC_CREAT);
    if(shmid == -1){
        perror("shmget");
        exit(1);
    }
    sharedSpace = (char*) shmat(shmid, NULL, 0);
    // make buffer the shared variable
    memcpy(sharedSpace, buff, sizeof(buffer));
    // leave shared memory
    shmdt(sharedSpace);
    while(true){
        // signal to handle when to terminate
        signal(SIGINT, signal_handler);
        // semaphore n wait for buffer
        semaphore[1].sem_op = -1;
        semaphore[1].sem_flg=SEM_UNDO;
        err = semop(s_id, &semaphore[1], 1);
        if (err == -1){
            perror("n-consumer-buffer-wait");
            getOut(sharedSpace, s_id, shmid);
        }
        /* critical section take from buffer */
        semaphore[0].sem_op = -1;
        semaphore[0].sem_flg=SEM_UNDO;
        err = semop(s_id, &semaphore[0], 1);
        if (err == -1){
            perror("s-consumer-buffer-wait");
            getOut(sharedSpace, s_id, shmid);
        }
        // go in shared space
        sharedSpace = (char*) shmat(shmid, NULL, 0);
        memcpy(buff, sharedSpace, sizeof(buffer));
        copyAndRemove(buff, cons);
        // go out of shared space
        memcpy(sharedSpace, buff, sizeof(buffer));
        shmdt(sharedSpace);
        // sleep(5);
        semaphore[0].sem_op = 1;
        err = semop(s_id, &semaphore[0], 1);
        if (err == -1){
            perror("s-consumer-buffer-signal");
            getOut(sharedSpace, s_id, shmid);
        }
        /* critical section access finished */
        // semaphore signal e for empty places
        // sleep(10);
        semaphore[2].sem_op = 1;
        err = semop(s_id, &semaphore[2], 1);
        if (err == -1){
            perror("e-consumer-buffer-signal");
            getOut(sharedSpace, s_id, shmid);
        }
        int idx = cons->comm_id;
        strcpy(items[idx]->comm_name, cons->comm_name);
        assignPrices(items[idx], cons->price);

        printTable(items);
    }
    shmdt(sharedSpace);
    semctl(s_id, 0, IPC_RMID);
    shmctl(shmid,IPC_RMID,NULL);

    return 0;
}
int itemsNotZero(commodity* item){
    int count=0;
    if(item->price > 0)
        count++;
    if(item->reading1 > 0)
        count++;
    if(item->reading2 > 0)
        count++;
    if(item->reading3 > 0)
        count++;
    if(item->reading4 > 0)
        count++;
    return count;
}
double getAvergePrice(commodity *item){
    int count = itemsNotZero(item);
    double avgPrice;
    if(count > 0)
        avgPrice = (item->price+item->reading1+
        item->reading2+item->reading3+item->reading4)/count;
    else
        avgPrice = 0;
    return avgPrice;
}
void assignPrices(commodity *item, double price){
    item->reading4 = item->reading3;
    item->reading3 = item->reading2;
    item->reading2 = item->reading1;
    item->reading1 = item->price;
    item->price    = price;
}
void moveToBeginning(int clear){
    printf("\e[1;1H");
    if(clear == 1)
        printf("\e[2J");
}
void moveScreen(int row, int column){
    printf("\033[%d;%dH", row, column);
}
void printRow(){
    cout << '+';
    for(int i=0;i<37;i++)
        cout << '-';
    cout << '+' << '\n';
}
void printHeader(){
    cout << "| Currency";
    for(int i=0;i<6;i++)
        cout << ' ';
    cout << "|  Price   | "; 
    cout << "AvgPrice |" << '\n'; 
}
void printItem(commodity *item){
    int len1 = 15;
    int len2 = 10;
    int len3 = 10;
    cout << "| " << item->comm_name;
    len1 = len1 - strlen(item->comm_name)-1;
    for(int i=0;i<len1;i++)
        cout << ' ';
    cout << "|";
    for(int i=0;i<len2;i++)
        cout << ' ';
    cout << "|"; 
    for(int i=0;i<len2;i++)
        cout << ' ';
    cout << "|" << "\n";
}
void printEmptyRow(){
    int len1 = 15;
    int len2 = 10;
    cout << "|";
    for(int i=0;i<len1;i++)
        cout << ' ';
    cout << "|";
    for(int i=0;i<len2;i++)
        cout << ' ';
    cout << "|"; 
    for(int i=0;i<len2;i++)
        cout << ' ';
    cout << "|" << "\n";
}
void editItem(int row, commodity *item){ 
    moveScreen(row, CURRENCY_COLUMN);
    cout << item->comm_name;
    moveScreen(row, PRICE_COLUMN);
    // change color
    if (item->price < item->reading1){
        cout << "\033[1;31m";
        printf("%7.2lf↓", item->price);
        moveScreen(row, AVG_PRICE_COLUMN);
        printf("%7.2lf↓", getAvergePrice(item));
    }
    else if (item->price > item->reading1){
        cout << "\033[1;32m";
        printf("%7.2lf↑", item->price);
        moveScreen(row, AVG_PRICE_COLUMN);
        printf("%7.2lf↑", getAvergePrice(item));
    }
    else{
        cout << "\033[1;34m";
        printf("%7.2lf", item->price);
        moveScreen(row, AVG_PRICE_COLUMN);
        printf("%7.2lf", getAvergePrice(item));
    }
    // reset color
    cout << "\033[0m";
    // cout<<" ↑ ↓ ";
    moveScreen(row, LAST_COLUMN);
    printf("\n");
}
void printTable(commodity *items[]){
    moveToBeginning(1);
    printRow();
    printHeader();
    printRow();
    for (int i=0;i<11;i++){
        printEmptyRow();
        editItem(i+4, items[i]);
    }
    printRow();
}