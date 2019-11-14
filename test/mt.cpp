#include <iostream>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

#define no_of_clients 3
int thread_counter;

void* worker_thread(void *args){
    int newsockfd = *((int*)args);
    std::cout << "Sock for from thread is: " << newsockfd << std::endl;
    

    thread_counter--;
    pthread_exit(NULL);
}

int main (int argc, char *argv[]){
    pthread_t t[no_of_clients];
    
    int sockfd, clientfd, addrlen;
    struct sockaddr_in servaddr,cliaddr;

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
    servaddr.sin_port = htons(/*DEFINE a port */);

    //bind ip with port 
    if(bind(sockfd,(struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ){
        std::cerr << "Binding Error: " << std::endl;
	exit(0);
    }

    if(listen(sockfd,5) < 0){
        std::cerr << "Listening Error: " << std::endl;
	exit(0);
    }
    addrlen = sizeof(cliaddr);
    thread_counter = 0;
    while(1){
        clientfd = accept(sockfd,(struct sockaddr * )&cliaddr,(socklen_t *)&addrlen);
        pthread_create(&t[thread_counter],NULL, worker_thread, (void*)&clientfd);
        thread_counter++;
        while(thread_counter > no_of_clients){}
    }

}