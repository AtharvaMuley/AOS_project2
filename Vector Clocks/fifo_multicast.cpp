#include <iostream>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <algorithm>

#define BUFFER_LEN 1024
#define NO_OF_CLIENTS 3

// Vector clock for all process. Limited to three process
int vector_clock[NO_OF_CLIENTS];
int processid = -1;
int port = 0;
int total_no_processes = 0;
int all_sockets[NO_OF_CLIENTS-1];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lock = PTHREAD_COND_INITIALIZER;

struct socket_ds{
    int sockfd;
    struct sockaddr_in servaddr;
}sockets[NO_OF_CLIENTS];


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

void init_all_sockets(){
    for(int i=0; i < NO_OF_CLIENTS; i++){
        // all_sockets[i] = SocketInit(port+i-1);
        int sockfd;
        int option = 1;
        // Create a socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));

        struct sockaddr_in servaddr;

        servaddr.sin_addr.s_addr = inet_addr("127.0.01");

        servaddr.sin_port = htons(port+i);
        servaddr.sin_family = AF_INET;


        if (sockfd < 0){
            std::cout << "Error creating a socket" << std::endl;
            return;
        }
        sockets[i].sockfd = sockfd;
        sockets[i].servaddr = servaddr;
        // std::cout << "Building sockets of port: " << port+i << "with sockfd" << sockets[i].sockfd << std::endl;

    }
}


int SocketInit(int port){
    int sockfd;
    int option = 1;
    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));
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

    char buffer[BUFFER_LEN];

    std::ifstream readfile;
    std::string filename = std::to_string(processid) + ".txt";
    readfile.open(filename);
    // std::cout << "Filename: " << filename << std::endl;
    std::string output;
    int destsockfd;
    if (readfile.is_open()){
        while (std::getline(readfile, output)){
            // std::cout << output << std::endl;
            for(int i=0; i < NO_OF_CLIENTS; i++){
                // std::cout<< "sending to sockfd: " << sockets[i].sockfd << std::endl;
                sendto(sockets[i].sockfd, (const char *)output.c_str(), strlen(output.c_str()), 0, (const struct sockaddr *) &sockets[i].servaddr, sizeof(sockets[i].servaddr));
            }
            
            sleep(3);
            
        }
    }
    pthread_exit(NULL);
}

void* recv_thread(void * args){
    struct thread_data data = *((struct thread_data*)args);
    std::cout << "Self port is: " << port+processid-1<< std::endl;
    int sockfd;

    int option = 1;
    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));

    std::cout << "Sockfd from recv is: " << sockfd << std::endl; 
    
    struct sockaddr_in servaddr, cliaddr;
    //fill servaddr with zeros 
    memset(&servaddr,0, sizeof(struct sockaddr_in));

    //Assign IP with port 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port+processid-1);

    if(bind(sockfd,(struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ){
        std::cerr << "Binding Error: " << std::endl;
	    exit(0);
    }

    char buffer[BUFFER_LEN];
    int n, len;
    while(1){
        n = recvfrom(sockfd,(char *)buffer, BUFFER_LEN, MSG_WAITALL,(struct sockaddr *) &cliaddr,(socklen_t*)&len);
        buffer[n] = '\0';
        std::cout << "message received =>" << buffer << std::endl;
    }
}


int main(int argc, char *argv[]){

    if (argc < 4 ){
        std::cout<< "Please enter total no of process, process number and port number" << std::endl;
        return -1;
    }
    struct thread_data td;
    total_no_processes = std::stoi(argv[1]);
    processid = std::stoi(argv[2]);
    port = std::stoi(argv[3]);

    
    init_all_sockets();
    td.filename = std::to_string(processid) + ".txt";
    pthread_t send_t, recv_t;
    std::cout << "Sleeping for 2 seconds to let every process initialize"<<std::endl;
    pthread_create(&recv_t,NULL, recv_thread, (void *)&td);
    sleep(4);
    pthread_create(&send_t,NULL, send_thread, (void *)&td);


    pthread_join(send_t, NULL);
    pthread_join(recv_t, NULL);

    return 0;
}