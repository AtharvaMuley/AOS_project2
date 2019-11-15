#include <iostream>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <string.h>

#define NO_OF_CLIENTS 4
#define REQ_MSG "REQUEST"
#define OK_MSG "REQUEST"
#define REL_MSG "REQUEST"


int processid = -1;
int port = 0000;

int SocketInit(int port){
    int sockfd;
    struct sockaddr_in servaddr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

    if(listen(sockfd,5) < 0){
        std::cerr << "Listening Error: " << std::endl;
	exit(0);
    }

    return sockfd;
}

void resource_handler(){
    std::fstream readfile;
    readfile.open("shared_file.txt");
    if(readfile.is_open()){
        std::string output;
        std::getline(readfile, output);
        int counter = stoi(output);
        counter++;
        readfile.seekg(0, std::ios::beg);
        readfile << std::to_string(counter);
        std::cout << "Counter value is: " << counter << std::endl;
    }
}

// This serves the request
void* worker(void *args){
    int newsockfd = *((int*)args);
    

    pthread_exit(NULL);
}

bool fileInit(){
    std::ifstream readfile;
    readfile.open("shared_file.txt");
    if(readfile.is_open()){
        readfile.close();
        return true;
    }
    else
        return false;
}

int main(int argc, char *argv[]){

    /*
        First argument is the file name
        Second argument is the process id
        Third argument is the port number
    */
   if(argc < 3){
       std::cout << "Please enter Process Id and port no" << std::endl;
       return -1;
   }
    processid = std::stoi(argv[1]);
    std::cout << "Process ID: " << processid << std::endl;
    port = std::stoi(argv[2]);

    
    // Master process server
    if (processid == 1){

        if(fork() == 0){
            // Child process
            int clientfd, addrlen, thread_counter = 0;
            pthread_t threads[NO_OF_CLIENTS];
            struct sockaddr_in cliaddr;
            //Create a server for requesting nodes
            int sockfd = SocketInit(port);

            if(!fileInit()){
                return -1;
            }

            addrlen = sizeof(cliaddr);
            while(1){
                clientfd = accept(sockfd,(struct sockaddr * )&cliaddr,(socklen_t *)&addrlen);
                pthread_create(&threads[thread_counter], NULL, worker, (void*)&clientfd);
                thread_counter++;
                while (thread_counter>NO_OF_CLIENTS){}
            }
        }
        else{
            //Parent
            while (1)
            {
                
            }
            
        }
    }
    int serverlen,csockfd;
    struct sockaddr_in servaddr;
    //Other process
    serverlen = sizeof(servaddr);
    // Create a socket
    csockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (csockfd < 0){
        std::cout << "Error creating a socket" << std::endl;
        return -1;
    }
    //fill servaddr with zeros 
    memset(&servaddr,0, sizeof(struct sockaddr_in));

    //Assign IP with port 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(port);
    if (connect(csockfd,(struct sockaddr *)&servaddr,serverlen) == -1){
        std::cout << "Error connecting, Exiting" << std::endl;
        return -1;
    }
    
    char signal[] = REQ_MSG;
    char buffer[1024];
    std::cout << "Trying to create a socket" << std::endl;
    while(1){
        send(csockfd, signal, sizeof(signal), 0);
        read(csockfd, buffer, 1024);

        resource_handler();
        strcpy(signal,REL_MSG);
        send(csockfd, signal, sizeof(signal), 0);
        sleep(2);
    }
    return 0;
}