#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <regex>
#include <pthread.h>
#include <cmath>
#define NO_OF_THREADS 2


int thread_counter = 0,acounter = 0;
int all_clocks[NO_OF_THREADS]; // Holds value of logical clocks of all the nodes
int all_sockdfd[NO_OF_THREADS];
int time_demon_time = 0;
int localClock = 0;

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
    std::cout << "Calculating new time" << std::endl;
    int new_clock = 0;
    for(int i=0; i < NO_OF_THREADS;i++){
        // std::cout << all_clocks[i] << std::endl;
        new_clock += all_clocks[i];
    }
    return floor((new_clock + localClock)/(NO_OF_THREADS+1));
}

void poll_time(){
    std::cout << "Polling" << std::endl;
    int tempsockfd = 0;
    char signal[1] = {'p'}; // Poll signal for nodes
    char clock_buffer[10]={'0'};
    int valread;
    for(int i=0; i < NO_OF_THREADS ;i++){
        tempsockfd = all_sockdfd[i];
        // std::cout << "tempsock " << tempsockfd << std::endl;
        send(tempsockfd, signal, strlen(signal), 0);
        int valread = read(tempsockfd,clock_buffer,10);
        // std::cout << "clock b: " << clock_buffer << std::endl;
        all_clocks[i] = std::stoi(clock_buffer);
    }
}
void send_time(){
    int tempsockfd = -1;
    char clock_buffer[10] = {'0'};
    std::string time = std::to_string(time_demon_time);
    // std::cout << "Converted to string is: " << time << std::endl;
    strcpy(clock_buffer, time.c_str());
    // std::cout << sizeof(clock_buffer) << std::endl;
    // std::cout << "TTT " << clock_buffer <<std::endl;
    for(int i=0; i < NO_OF_THREADS ;i++){
        tempsockfd = all_sockdfd[i];
        // std::cout << "sending time to: " << tempsockfd << std::endl;
        send(tempsockfd, clock_buffer, sizeof(clock_buffer),0);
    }
}

void* time_demon(void* args){
    // Run time demon after every 10 seconds
    time_demon_is_running = true;
    std::cout << "Time demon started" << std::endl;
    poll_time();
    time_demon_time = time_calculator();
    std::cout << "Average time: " << time_demon_time << std::endl;
    std::cout << "Time adjustment: " << (time_demon_time - localClock) << std::endl;
    localClock += (time_demon_time - localClock);
    std::cout << "Local clock value is now: " << localClock << std::endl;
    send_time();
    time_demon_is_running = false;
}

void* worker(void* args){
    int newsockfd = *((int *)args);
    // std::cout << newsockfd << std::endl;
    // Update sockfd on conncection
    pthread_mutex_lock(&mutex);
    while(!&lock){
            	 pthread_cond_wait(&lock,&mutex);
            }
    all_sockdfd[acounter] = newsockfd;
    acounter++;
    pthread_cond_signal(&lock);
    pthread_mutex_unlock(&mutex);
    if (thread_counter == NO_OF_THREADS){
        //Starting time demon
        pthread_t timed;
        pthread_create(&timed, NULL, time_demon,NULL);
    }
    while(time_demon_is_running){
    }
    
    close(newsockfd);
    thread_counter --;
    pthread_exit(NULL);
}

int main(int argc, char *argv[]){
    int processID;
    int port;
    int sockfd,clientfd,addrlen,rc;
    struct sockaddr_in cliaddr;
    pthread_t threads[NO_OF_THREADS];

    //represents the process Id and the port no
    struct process processesList;

    time_t t;
    srand((unsigned) time(&t));
    localClock = rand()%25; // Add randomness to the value of clock
    std::cout<< "Local Clock: " << localClock << std::endl;

    //Initialize socket
    sockfd = SocketInit(8447);
    addrlen = sizeof(cliaddr);
    
    while (time_demon_is_running){
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