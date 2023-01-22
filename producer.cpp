#include "sharedCode.cpp"

void mapCommodity(producer *prod){
    if(strcmp(prod->comm_name, "ALUMINIUM") == 0){
        prod->comm_id = 0; 
    }
    else if(strcmp(prod->comm_name, "COPPER") == 0){
        prod->comm_id = 1; 
    }
    else if(strcmp(prod->comm_name, "COTTON") == 0){
        prod->comm_id = 2; 
    }
    else if(strcmp(prod->comm_name, "CRUDEOIL") == 0){
        prod->comm_id = 3; 
    }
    else if(strcmp(prod->comm_name, "GOLD") == 0){
        prod->comm_id = 4; 
    }
    else if(strcmp(prod->comm_name, "LEAD") == 0){
        prod->comm_id = 5; 
    }
    else if(strcmp(prod->comm_name, "MENTHAOIL") == 0){
        prod->comm_id = 6; 
    }
    else if(strcmp(prod->comm_name, "NATURALGAS") == 0){
        prod->comm_id = 7; 
    }
    else if(strcmp(prod->comm_name, "NICKEL") == 0){
        prod->comm_id = 8; 
    }
    else if(strcmp(prod->comm_name, "SILVER") == 0){
        prod->comm_id = 9; 
    }
    else if(strcmp(prod->comm_name, "ZINC") == 0){
        prod->comm_id = 10; 
    }
    else {
        prod->comm_id = -1; 
    }

}
void producerLog(char *comm_name, double price);
int main(int argc, char *argv[]) {
    // fetch the arguments
    char *comm_name = argv[1];
    double comm_price_mean = atof(argv[2]);
    double comm_price_sd = atof(argv[3]);
    int sleep_interval = atoi(argv[4]);
    int BUFFER_SIZE = atoi(argv[5]);
    double price = 0;
    int err;
    char *sharedSpace;
    // allocate memory
    producer *prod = (producer *)malloc(sizeof(producer));
    // choose a price according to random distribution
    srand(price);
    std::default_random_engine generator(rand());
    std::normal_distribution<double> distribution(comm_price_mean, comm_price_sd);
    // generate a key
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

    // shmget returns an identifier in shmid
    int shmid = shmget(key,sizeof(producer),0666);
    if(shmid == -1){
        perror("shmget");
        exit(1);
    }
    // shmat to attach to shared memory
    sharedSpace = (char*) shmat(shmid, NULL, 0);
    // get buffer from shared space
    memcpy(buff, sharedSpace, sizeof(buffer));
    // leave shared memory
    shmdt(sharedSpace);
    while(true){
        price = distribution(generator);
        price = price < 0 ? price * -1 : price;
        producerLog(comm_name, price);
        fprintf(stderr, "generating a new value %lf\n", price);
        strcpy(prod->comm_name, comm_name);
        prod->price=price;
        mapCommodity(prod);
        // semaphore e wait for empty places
        semaphore[2].sem_op = -1;
        semaphore[2].sem_flg = SEM_UNDO;
        err = semop(s_id, &semaphore[2], 1);
        if (err == -1){
            perror("e-producer-buffer-wait");
            exit(1);
        }
        producerLog(comm_name, price);
        fprintf(stderr, "trying to get mutex on shared buffer\n");
        /* critical section add to buffer */
        semaphore[0].sem_op = -1;
        semaphore[0].sem_flg=SEM_UNDO;
        err = semop(s_id, &semaphore[0], 1);
        if (err == -1){
            perror("producer-buffer-wait");
            exit(1);
        }
        producerLog(comm_name, price);
        fprintf(stderr, "placing %lf on shared buffer\n", price);
        // go in shared space
        sharedSpace = (char*) shmat(shmid, NULL, 0);
        memcpy(buff, sharedSpace, sizeof(buffer));
        insertBuffer(buff, prod);
        // go out of shared space
        memcpy(sharedSpace, buff, sizeof(buffer));
        shmdt(sharedSpace);
        semaphore[0].sem_op = 1;
        err = semop(s_id, &semaphore[0], 1);
        if (err == -1){
            perror("producer-buffer-signal");
            exit(1);
        }
        /* critical section access finished */
        // semaphore n signal for buffer
        semaphore[1].sem_op = 1;
        err = semop(s_id, &semaphore[1], 1);
        if (err == -1){
            perror("n-producer-buffer-signal");
            exit(1);
        }
        producerLog(comm_name, price);
        fprintf(stderr, "sleeping for %d ms\n", sleep_interval);
        // usleep takes parameter in microseconds
        usleep(1000*sleep_interval);
    }
    shmdt(sharedSpace);
    return 0;
}

void producerLog(char *comm_name, double price){
    struct timespec curr_time;
    clock_gettime(CLOCK_REALTIME, &curr_time);
    struct tm *tm = localtime(&curr_time.tv_sec);
    float time_nsec = curr_time.tv_nsec/pow(10,9);
    string str = to_string(roundf(time_nsec*1000)/1000);
    str = str.substr(1, 4);
    char strr[10];
    strcpy(strr, str.c_str());
    // float time_sec = tm->tm_sec + curr_time.tv_nsec/pow(10,9);
    fprintf(stderr, "[%d/%d/%d ",tm->tm_mon+1,tm->tm_mday,tm->tm_year+1900);
    fprintf(stderr, "%d:%d:%d%s] ",tm->tm_hour,tm->tm_mday,tm->tm_sec,strr);
    fprintf(stderr, "%s: ", comm_name);
}
