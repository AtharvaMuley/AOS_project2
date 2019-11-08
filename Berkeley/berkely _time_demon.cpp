#include<iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include<string>
#include <regex>
#include <pthread.h>

#define NO_OF_THREADS 50

using namespace std;

int thread_counter = 0;

// process attributes
struct process{
    int processID;
    int port;
};

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

void createProcessList(string processString){
    std::regex process_structure("(.[0-9]*)");
    std::smatch match;
    std::regex_search(processString, match, process_structure);
    cout<<"p"<< match.str(1)<<endl;
}

void* time_demon(void* args){
    int newsockfd = *((int *)args);
    cout << newsockfd << endl;
    close(newsockfd);
    pthread_exit(NULL);
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
    float localClock;
    int sockfd,clientfd,addrlen,rc;
    struct sockaddr_in cliaddr;
    pthread_t threads[NO_OF_THREADS];

    //represents the process Id and the port no
    struct process processesList;

    if (argc < 2){ // Update
        cout<<"Process ID missing. Please specify process ID" << endl;
        return -1;
    }
    processID = stoi(argv[1]);
    port = stoi(argv[2]);
    localClock = (processID*7)%13; // Add randomness to the value of clock
    cout << localClock << endl;
    string procList = argv[3];
    cout << procList << endl;
    // createProcessList(procList);

    //Initialize socket
    sockfd = SocketInit(8080);
    addrlen = sizeof(cliaddr);
    
    while (1){
        clientfd = accept(sockfd,(struct sockaddr * )&cliaddr,(socklen_t *)&addrlen);
    	if(clientfd < 0 ){
        	std::cout<< "Error creating connetion"<< std::endl;
        	continue;
    	}
    	else{
        	rc = pthread_create(&threads[thread_counter],NULL,time_demon,(void *)&clientfd);
        	thread_counter++;
        	while (thread_counter > NO_OF_THREADS){
        		/* While thread limit had reached wait till the current clients close the connection*/
        	}
        }
    }

    return 0;
}