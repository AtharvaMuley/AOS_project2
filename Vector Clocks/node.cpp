#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <fstream>
#include <algorithm>

#define NO_OF_CLIENTS 3

// Vector clock for all process. Limited to three process
int vector_clock[NO_OF_CLIENTS];
int processid = -1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lock = PTHREAD_COND_INITIALIZER;

void update_vector_clock(int va[NO_OF_CLIENTS]){
    pthread_mutex_lock(&mutex);
    while(!&lock){
        pthread_cond_wait(&lock,&mutex);
    }

    for(int i=0; i< NO_OF_CLIENTS;i++){
        if (processid == i){
            vector_clock[i] == std::max(vector_clock[i], va[i]) + 1;
        }
        else{
            vector_clock[i] == std::max(vector_clock[i], va[i]);
        }
    }
    pthread_cond_signal(&lock);
    pthread_mutex_unlock(&mutex);
}

struct thread_data{
    // int processid;
    int sockfd;
    std::string filename;
};

int SocketInit(int port){
    int sockfd;
    struct sockaddr_in servaddr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){
        std::cout << "Error creating a socket" << std::endl;
        return -1;
    }
    //fill servaddr with zeros 
    memset(&servaddr,0, sizeof(struct sockaddr_in));

    //Assign IP with port 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    //bind ip with port 
    if(bind(sockfd,(struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ){
        std::cerr << "Binding Error: " << std::endl;
	exit(0);
    }

    // if(listen(sockfd,5) < 0){
    //     std::cerr << "Listening Error: " << std::endl;
	// exit(0);
    // }

    return sockfd;
}

/* this thread is used for sending data to other processes */
void* send_thread(void * args){
    struct thread_data data = *((struct thread_data*)args);

    std::ifstream readfile;
    readfile.open(data.filename);
    std::string output;
    std::cout << data.sockfd;
    if (readfile.is_open()){
        while (std::getline(readfile, output)){
            std::cout << output << std::endl;
        }
    }
}

void* recv_thread(void * args){
    
}

int main(int argc, char *argv[]){

    if (argc <2 ){
        std::cout<< "Please enter process number" << std::endl;
        return -1;
    }
    struct thread_data td;
    processid = std::stoi(argv[1]);
    td.sockfd = SocketInit(8824);
    td.filename = std::to_string(processid) + ".txt";
    pthread_t send_t, recv_t;

    pthread_create(&send_t,NULL, send_thread, (void *)&td);
    pthread_create(&recv_t,NULL, recv_thread, (void *)td);


    pthread_join(send_t, NULL);
    pthread_join(recv_t, NULL);

    return 0;
}