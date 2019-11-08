#include<iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include<string>
#include <regex>
#include <pthread.h>

#define NO_OF_THREADS 50


int thread_counter = 0,no_of_nodes_connected = 0;
int all_clocks[NO_OF_THREADS]; // Holds value of logical clocks of all the nodes
int all_sockdfd[NO_OF_THREADS];
int time_demon_time;

bool time_demon_is_running = true;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lock = PTHREAD_COND_INITIALIZER;

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

//Close the socket
void socket_Reset(int sockfd){
    close(sockfd);
}

int time_calculator(){
    int new_clock = 0;
    for(int i=0; i < thread_counter;i++){
        new_clock += all_clocks[i];
    }
    return (int)new_clock/thread_counter;
}

void createProcessList(std::string processString){
    std::regex process_structure("(.[0-9]*)");
    std::smatch match;
    std::regex_search(processString, match, process_structure);
    std::cout<<"p"<< match.str(1)<<std::endl;
}

void poll_time(){
    int tempsockfd = -1;
    char signal[1] = {'p'}; // Poll signal for nodes
    char clock_buffer[10];
    int valread;
    for(int i=0; i < thread_counter;i++){
        tempsockfd = all_sockdfd[i];
        send(tempsockfd, signal, strlen(signal), 0);
        int valread = read(tempsockfd,clock_buffer,10);
        all_clocks[i] = std::stoi(clock_buffer);
    }
}
void send_time(){
    int tempsockfd = -1;
    char clock_buffer[10];
    int valread;
    strcpy(clock_buffer, (char*)time_demon_time);
    for(int i=0; i < thread_counter;i++){
        tempsockfd = all_sockdfd[i];
        send(tempsockfd, clock_buffer, sizeof(clock_buffer),0);
    }
}

void* worker(void* args){
    int newsockfd = *((int *)args);
    std::cout << newsockfd << std::endl;
    // Update sockfd on conncection
    pthread_mutex_lock(&mutex);
    while(!&lock){
            	 pthread_cond_wait(&lock,&mutex);
            }
    all_sockdfd[thread_counter] = newsockfd;
    thread_counter++;
    pthread_cond_signal(&lock);
    pthread_mutex_unlock(&mutex);
    
    while(time_demon_is_running){
    }

    close(newsockfd);
    pthread_exit(NULL);
}

void* time_demon(void* args){
    // Run time demon after every 10 seconds
    time_demon_is_running = true;
    poll_time();
    time_demon_time = time_calculator();

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
    int sockfd,clientfd,addrlen,rc;
    struct sockaddr_in cliaddr;
    pthread_t threads[NO_OF_THREADS];

    //represents the process Id and the port no
    struct process processesList;

    // if (argc < 2){ // Update
    //     std::cout<<"Process ID missing. Please specify process ID" << std::endl;
    //     return -1;
    // }
    // processID = std::stoi(argv[1]);
    // port = std::stoi(argv[2]);
    // localClock = (processID*7)%13; // Add randomness to the value of clock
    // std::cout << localClock << std::endl;
    // std::string procList = argv[3];
    // std::cout << procList << std::endl;
    // createProcessList(procList);

    //Initialize socket
    sockfd = SocketInit(8080);
    addrlen = sizeof(cliaddr);

    //Starting time demon
    pthread_t timed;
    pthread_create(&timed, NULL, time_demon,NULL);
    
    while (1){
        clientfd = accept(sockfd,(struct sockaddr * )&cliaddr,(socklen_t *)&addrlen);
    	if(clientfd < 0 ){
        	std::cout<< "Error creating connetion"<< std::endl;
        	continue;
    	}
    	else{
        	rc = pthread_create(&threads[thread_counter],NULL,worker,(void *)&clientfd);
        	thread_counter++;
        	while (thread_counter > NO_OF_THREADS){
        		/* While thread limit had reached wait till the current clients close the connection*/
        	}
        }
    }

    return 0;
}