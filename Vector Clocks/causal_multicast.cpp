#include <iostream>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <algorithm>
#include <regex>
#include <list>


#define BUFFER_LEN 1024
#define MSG_QUEUE_LEN 100
#define NO_OF_CLIENTS 3

// Vector clock for all process. Limited to three process
int vector_clock[3] = {0,0,0};
int processid = -1;
int port = 0;
int total_no_processes = 0;

int all_sockets[NO_OF_CLIENTS-1];
pthread_mutex_t mutex_vlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t vlock = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mlock = PTHREAD_COND_INITIALIZER;

std::list <std::string> message_queue;
bool update_vector_clock(std::string);
struct socket_ds{
    int sockfd;
    struct sockaddr_in servaddr;
}sockets[NO_OF_CLIENTS];

std::string access_clock(){
    std::string clock = "[";
    for(int i=0; i < NO_OF_CLIENTS;i++){
        clock += std::to_string(vector_clock[i]) + ",";
    }
    clock += "]";
    return clock;
}

void* message_ordering(void *args){   
    while(1){
        if(!message_queue.empty()){
            for(auto itr = message_queue.begin();itr != message_queue.end(); itr++){
                std::string s = *itr;
                if(update_vector_clock(s)){
                    message_queue.remove(s);
                }
            }
        }
    }
}

int* convert_to_vector(std::string msg){
    int vector[NO_OF_CLIENTS];
    std::smatch match;
    std::regex cv("\\[(.[0-9]*),(.[0-9]*),(.[0-9]*)");
    std::regex_search(msg, match,cv);
    std::cout << msg << std::endl;
    for(int i=0; i <  NO_OF_CLIENTS;i++){
        vector[i] = std::stoi(match.str(i+1));
        // std::cout << "v: " << match.str(i+1) <<std::endl;
    }
    return vector;
}
std::string update_self(){
    pthread_mutex_lock(&mutex_vlock);
    while(!&vlock){
        pthread_cond_wait(&vlock,&mutex_vlock);
    }
    vector_clock[processid-1] += 1;
    return access_clock();
    pthread_cond_signal(&vlock);
    pthread_mutex_unlock(&mutex_vlock);
}
bool update_vector_clock(std::string msg){
    pthread_mutex_lock(&mutex_vlock);
    while(!&vlock){
        pthread_cond_wait(&vlock,&mutex_vlock);
    }
    int va[NO_OF_CLIENTS];
    int spid;
    std::smatch match,m2;
    std::regex pm("(.[0-9]*)");
    std::regex cv("\\[(.[0-9]*),(.[0-9]*),(.[0-9]*)");
    std::regex_search(msg, match,cv);
    // std::cout << msg << std::endl;
    bool flag = false;

    std::regex_search(msg,m2,pm);
    std::cout << m2.str(1) << std::endl;

    for(int i=0; i <  NO_OF_CLIENTS;i++){
        va[i] = std::stoi(match.str(i+1));
        // std::cout << "v: " << match.str(i+1) <<std::endl;
    }
    if(vector_clock[spid-1] + 1 != va[spid-1]){
        return false;
    }
    for(int i=0; i< NO_OF_CLIENTS;i++){
        if (spid-1 != i){
            if(vector_clock[i] > va[i]){
                return false;
            }
        }
    }
    vector_clock[spid] += 1;
    std::cout << "message received => " << msg << std::endl;
    vector_clock[spid-1] += 1;
    
    std::cout <<"Updated vector: " << access_clock() << std::endl;
    pthread_cond_signal(&vlock);
    pthread_mutex_unlock(&mutex_vlock);
    return true;
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
        // int option = 1;
        // Create a socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        // setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));

        struct sockaddr_in servaddr;

        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        servaddr.sin_port = htons(port+i);
        servaddr.sin_family = AF_INET;


        if (sockfd < 0){
            std::cout << "Error creating a socket" << std::endl;
            return;
        }
        sockets[i].sockfd = sockfd;
        sockets[i].servaddr = servaddr;
        std::cout << "Building sockets of port: " << port+i << " with sockfd " << sockets[i].sockfd << std::endl;

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
            strcpy(buffer, output.c_str());
            strcat(buffer," ");
            strcat(buffer, update_self().c_str());

            for(int i=0; i < NO_OF_CLIENTS; i++){
                // std::cout<< "sending to sockfd: " << sockets[i].sockfd << std::endl;
                sendto(sockets[i].sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *) &sockets[i].servaddr, sizeof(sockets[i].servaddr));
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

    // int option = 1;
    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));

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

    //Start thread for maintianing causal ordering
    pthread_t t;
    pthread_create(&t, NULL, message_ordering, NULL);
    char buffer[1024];
    int n, len;
    while(1){
        n = recvfrom(sockfd,(char *)buffer, BUFFER_LEN, 0,(struct sockaddr *) &cliaddr,(socklen_t*)&len);
        // buffer[n] = '\0';
        pthread_mutex_lock(&mutex_mlock);
        while(!&mlock){
            pthread_cond_wait(&mlock,&mutex_mlock);
        }
            std::string s (buffer, strlen(buffer));
            message_queue.push_back(s);
        pthread_cond_signal(&mlock);
        pthread_mutex_unlock(&mutex_mlock);
        // update_vector_clock(buffer);
       std::cout << "message received => " << s << std::endl;
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
    std::cout << access_clock() << std::endl;
    // td.sockfd = SocketInit(8824);
    init_all_sockets();
    td.filename = std::to_string(processid) + ".txt";
    pthread_t send_t, recv_t;
    std::cout << "Sleeping for 4 seconds to let every process initialize"<<std::endl;
    pthread_create(&recv_t,NULL, recv_thread, (void *)&td);
    sleep(4);
    pthread_create(&send_t,NULL, send_thread, (void *)&td);
    


    pthread_join(send_t, NULL);
    pthread_join(recv_t, NULL);

    return 0;
}