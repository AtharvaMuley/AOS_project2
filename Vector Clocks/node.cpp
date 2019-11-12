#include <iostream>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <algorithm>

#define NO_OF_CLIENTS 3
#define BUFFER_LEN 1024

// Vector clock for all process. Limited to three process
int vector_clock[NO_OF_CLIENTS];
int processid = -1;
int port = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lock = PTHREAD_COND_INITIALIZER;



void update_vector_clock(int va[NO_OF_CLIENTS]){
    pthread_mutex_lock(&mutex);
    while(!&lock){
        pthread_cond_wait(&lock,&mutex);
    }

    for(int i=0; i< NO_OF_CLIENTS;i++){
        if (processid == i){
            vector_clock[i] = std::max(vector_clock[i], va[i]) + 1;
        }
        else{
            vector_clock[i] = std::max(vector_clock[i], va[i]);
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

//Used to initialize socket address
struct sockaddr_in set_sock_addr(int port){
    struct sockaddr_in servaddr;

    //fill servaddr with zeros 
    memset(&servaddr,0, sizeof(struct sockaddr_in));

    //Assign IP with port 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    return servaddr;
}

int bind_server(int sockfd, struct sockaddr_in servaddr){
    //bind ip with port 
    if(bind(sockfd,(struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ){
        std::cerr << "Binding Error: " << std::endl;
	    exit(0);
    }
    return 1;
}

int SocketInit(int port){
    int sockfd;
    int option = 1;
    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    // setsockopt(sockfd, SOL_SOCKET, SO_REUSESHAREUID, &option, sizeof(option));


    if (sockfd < 0){
        std::cout << "Error creating a socket" << std::endl;
        return -1;
    }
   
    return sockfd;
}

/* this thread is used for sending data to other processes */
void* send_thread(void * args){
    struct thread_data data = *((struct thread_data*)args);

    // int sockfd = data.sockfd;
    int sockfd = SocketInit(port);
    struct sockaddr_in servaddr = set_sock_addr(port);
    std::cout << "Sockfd from send is: " << sockfd << std::endl; 
    char buffer[BUFFER_LEN];

    std::ifstream readfile;
    readfile.open(data.filename);
    std::string output;
    if (readfile.is_open()){
        while (std::getline(readfile, output)){
            // std::cout << output << std::endl;
            sendto(sockfd, (const char *)output.c_str(), strlen(output.c_str()), 
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr));
            sleep(1);
            
        }
    }
    pthread_exit(NULL);
}

void* recv_thread(void * args){
    struct thread_data data = *((struct thread_data*)args);
    int sockfd = SocketInit(port);
    std::cout << "Sockfd from recv is: " << sockfd << std::endl; 
    struct sockaddr_in servaddr = set_sock_addr(port), cliaddr;

    if(bind_server(sockfd, servaddr)){
        char buffer[BUFFER_LEN];
        int n, len;
        while(1){
            n = recvfrom(sockfd,(char *)buffer, BUFFER_LEN, MSG_WAITALL,(struct sockaddr *) &cliaddr,(socklen_t*)&len);
            buffer[n] = '\0';
            std::cout << "message received " << buffer << std::endl;
        }
    }    
}

int main(int argc, char *argv[]){

    if (argc <3 ){
        std::cout<< "Please enter process number and port number" << std::endl;
        return -1;
    }
    struct thread_data td;
    processid = std::stoi(argv[1]);
    port = std::stoi(argv[2]);

    td.sockfd = SocketInit(8824);
    td.filename = std::to_string(processid) + ".txt";
    pthread_t send_t, recv_t;

    pthread_create(&send_t,NULL, send_thread, (void *)&td);
    pthread_create(&recv_t,NULL, recv_thread, (void *)&td);


    pthread_join(send_t, NULL);
    pthread_join(recv_t, NULL);

    return 0;
}