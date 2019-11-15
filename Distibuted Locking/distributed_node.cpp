#include <iostream>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <string.h>
#include <list>

#define NO_OF_CLIENTS 4
#define REQ_MSG "REQUEST"
#define OK_MSG "OK"
#define REL_MSG "RELEASE"
#define no_of_clients 3

int processid = -1;
int port = 0000;
int thread_counter;
int top = -1;
std::list <int> resource_queue;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lock = PTHREAD_COND_INITIALIZER;

void resource_handler(){
    sleep(4);
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
    char signal[] = OK_MSG;
    char buffer[1024];
    int valread;
    
    valread = read(newsockfd,buffer,10);
    // std::cout << buffer << std::endl;
    std::cout << "Message from " << newsockfd << " socket: " << buffer << std::endl;
    if(strcmp(buffer, REQ_MSG) == 0){
        std::cout << "Requested Resource" << std::endl;
        if (top == -1){
        top = newsockfd;
        }
        else{
            pthread_mutex_lock(&mutex);
            while(!&lock){
                pthread_cond_wait(&lock,&mutex);
            }
            resource_queue.push_back(newsockfd);
            pthread_cond_signal(&lock);
            pthread_mutex_unlock(&mutex);
        }
    }
    
    //Wait till turn doesnt comes up
    while(top != newsockfd){}
    //Send ok message
    send(newsockfd, signal, sizeof(signal), 0);

    //wait for release msg
    valread = read(newsockfd,buffer,10);
    if(strcmp(buffer, REL_MSG) == 0){
        std::cout << "Message from " << newsockfd << " socket: " << buffer << std::endl;
        pthread_mutex_lock(&mutex);
        while(!&lock){
            pthread_cond_wait(&lock,&mutex);
        }
        if(resource_queue.empty()){
            top = -1;
        }
        else{
            top = resource_queue.front();
            resource_queue.pop_front();
        }
        pthread_cond_signal(&lock);
        pthread_mutex_unlock(&mutex);
        std::cout << "Resource released by sockfd: "<<newsockfd<<std::endl;
    }

    

    thread_counter--;
    pthread_exit(NULL);
}

bool fileInit(){
    std::fstream readfile;
    readfile.open("./shared_file.txt", std::ios::out|std::ios::in);
    if(readfile.is_open()){
        int counter = 0;
        readfile << counter;
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
    port = std::stoi(argv[2]);
    std::cout << "Process ID: " << processid << " Port: "<< port << std::endl;

    
    // Master process server
    if (processid == 1){

        if(fork() == 0){
            // Child process
            if(!fileInit()){
                std::cout << "Error creating the shared file" << std::endl;
                return -1;
            }

            int sockfd, clientfd, addrlen;
            struct sockaddr_in servaddr,cliaddr;
            pthread_t t[no_of_clients];

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
            addrlen = sizeof(cliaddr);
            thread_counter = 0;
            std::cout << "Shared File is now ready to be accessed." << std::endl;
            while(1){
                clientfd = accept(sockfd,(struct sockaddr * )&cliaddr,(socklen_t *)&addrlen);
                pthread_create(&t[thread_counter],NULL, worker, (void*)&clientfd);
                pthread_join(t[thread_counter],NULL);
                thread_counter++;
                while(thread_counter > no_of_clients){}
            }
        }
        else{
            //Parent
            // std::cout << "This works " << std::endl;
        
            while (1){}   
        }
    }
    else{
            int clientfd,clientlen;
            struct sockaddr_in servaddr;

            clientfd = socket(AF_INET,SOCK_STREAM,0);
            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            servaddr.sin_port = htons(port);

            clientlen = sizeof(servaddr);
            if(connect(clientfd,(struct sockaddr *)&servaddr,clientlen) == -1){
                std::cout << "Error connecting to server" << std::endl;
                return -1;
            }
            char signal[] = REQ_MSG;
            char buffer[1024];

            std::cout << "Requesting access to the Shared File" << std::endl;
            send(clientfd, signal, sizeof(signal), 0);
            read(clientfd, buffer, 1024);
            std::cout << "Message from server: "<< buffer << std::endl;
            
            //Request resource only when ok msg is receved
            if(strcmp(buffer, OK_MSG) == 0){
                resource_handler();
            }  

            //Send release message to server
            strcpy(signal,REL_MSG);
            send(clientfd, signal, sizeof(signal), 0);
            sleep(2);
            close(clientfd);
        }
    
    return 0;
}