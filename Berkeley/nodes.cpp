#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <regex>
#include <arpa/inet.h>

using namespace std;

int SocketInit(int port){
    int sockfd;
    struct sockaddr_in servaddr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        cout << "Error creating a socket" << endl;
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

//Close the socket
void socket_Reset(int sockfd){
    close(sockfd);
}

int main(int argc, char *argv[]){
    /*
    1: file name
    2: process ID
    3: port
    4: list of process 
    */
    int processID;
    int port;
    int localClock;
    int sockfd,addrlen,rc;
    struct sockaddr_in cliaddr;
    time_t t;
    srand((unsigned) time(&t));
    localClock = rand()%25; // Add randomness to the value of clock

    int clientfd,clientlen;
    struct sockaddr_in servaddr;

    clientfd = socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(8447);

    clientlen = sizeof(servaddr);
    connect(clientfd,(struct sockaddr *)&servaddr,clientlen);
    
    char signal[1];
    int valread;
    char clock[10]={'0'};
    char newClock[10]={'0'};
    valread = read(clientfd, signal, 1024);
    string lc = std::to_string(localClock);
    // std::cout << "Converted to string is: " << lc << std::endl;
    strcpy(clock, lc.c_str());
    std::cout << "Local Clock: "<< localClock<<std::endl;

    send(clientfd, clock, strlen(clock), 0);
    memset(&clock, '0', 10);
    // wait for logical value from the clock
    valread = read(clientfd, newClock, 1024);
    int temp = stoi(newClock);
    std::cout << "Average time: " << temp << std::endl;
    std::cout << "Time adjustment: " << (temp - localClock) << std::endl;
    localClock += (temp - localClock);
    std::cout << "Local clock value is now: " << localClock << std::endl;
    close(clientfd);
    return 0;
}