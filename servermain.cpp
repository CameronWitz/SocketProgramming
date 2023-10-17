/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>

#define PORT "23659"  // the port users will be connecting to

#define BACKLOG 10  // how many pending connections queue will hold
#define MAXDATASIZE 1024 // max request size (Department name), unlikely to be larger than this

void readList(std::unordered_map<std::string, std::string> &dept_to_server, std::unordered_map<std::string, std::vector<std::string>> &server_to_dept ){
    std::ifstream infile;
    infile.open("list.txt");
    std::string backend_server;
    std::string departments;
    size_t beginning;
    std::vector<std::string> servers;
    
    while(infile >> backend_server){
        servers.push_back(backend_server);
        infile >> departments;
        beginning = 0;
        std::vector<std::string> depts_vec;
        for(size_t i = 0; i < departments.length(); i++){
            char cur = departments[i];
            if(cur == ';'){
                dept_to_server[departments.substr(beginning, i-beginning)] = backend_server;
                // std::cout << "DEBUG: Department read:" << departments.substr(beginning, i-beginning) << std::endl;
                beginning = i + 1;
                depts_vec.push_back(departments.substr(beginning, i-beginning));
            }
        }
        dept_to_server[departments.substr(beginning, departments.length()-beginning)] = backend_server;
        depts_vec.push_back(departments.substr(beginning, departments.length()-beginning));
        server_to_dept[backend_server] = depts_vec;
    }
    std::cout << "Main server has read the department list from list.txt." << std::endl;

    std::cout << "Total num of Backend Servers: " << servers.size() << std::endl;
    for(std::vector<std::string>::iterator iter = servers.begin(); iter < servers.end(); iter ++){
        std::string server_num = (*iter);
        std::vector<std::string> these_depts = server_to_dept[server_num];
        std::cout << "Backend Servers " << server_num << " contains " << these_depts.size();
        std::cout << " distinct departments" << std::endl;
    }

}

// invoked due to sigaction
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;//, my_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int clients = 0;
    int yes=1;
    // char s[INET6_ADDRSTRLEN];
    int rv;
    std::unordered_map<std::string, std::string> dept_to_server;
    std::unordered_map<std::string, std::vector<std::string>> server_to_dept;

    std::cout << "Main server is up and running." << std::endl;
    // read in the List.txt file into the unordered_map
    readList(dept_to_server, server_to_dept);

    // Specify the type of connection we want to host
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; //IPV4 and IPV6 both fine
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // use my IP

    // store linked list of potential hosting ports in servinfo
    if ((rv = getaddrinfo("localhost", PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        clients ++;

        // Don't think i need this part
        // inet_ntop(their_addr.ss_family,
        //     get_in_addr((struct sockaddr *)&their_addr),
        //     s, sizeof s);
        // printf("server: got connection from %s\n", s);

        // socklen_t addr_len = sizeof my_addr;
        // int ret, port;
        // if((ret = getsockname(new_fd, (struct sockaddr *)&my_addr, &addr_len)) == -1){
        //     perror("getsockname");
        //     exit(1);
        // }
     
        // All this is just to confirm that the port number is the same as what was specified.
        // inet_ntop(my_addr.ss_family, 
        //     get_in_addr((struct sockaddr *)&my_addr),
        //     s, sizeof s);

        // if (my_addr.ss_family == AF_INET) {
        //     std::cout << "IPV4" << std::endl;
        //     port = ntohs(((struct sockaddr_in *)&my_addr)->sin_port);
        // } else if (my_addr.ss_family == AF_INET6) {
        //     std::cout << "IPV6" << std::endl;
        //     port = ntohs(((struct sockaddr_in6 *)&my_addr)->sin6_port);
        // }
        // else {
        //     perror("not ipv4 or ipv6");
        //     exit(1);
        // }
        
        // std::cout << "Receiving client over port number " << PORT << std::endl;
       
         
        if (!fork()) { // this is the child process
            int cur_client = clients;
            close(sockfd); // child doesn't need the listener

            while(1){
                // Respond to any queries from the client
                int numbytes;
                char buf[MAXDATASIZE];

                if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                    perror("recv");
                    exit(1);
                }

                if(numbytes == 0){
                    std::cout << "Client closed connection, this process will exit" << std::endl;
                    close(new_fd);
                    exit(0);
                }

                buf[numbytes] = '\0';
                std::string request(buf);

                std::cout << "Main server has received the request on Department " << request;
                std::cout << "from client " << cur_client << " using TCP over port " << PORT << std::endl;

                std::string reply;

                if(dept_to_server.find(request) == dept_to_server.end()){
                    std::cout << "HERE" << std::endl;
                    reply = "Not Found"; 
                    std::cout << "Department " << request << " does not show up in backend server ";
                    int firsttime = 1;
                    for(auto &entry : server_to_dept){
                        if(!firsttime){
                            std::cout << ", ";
                        }
                        firsttime = 0;
                        std::cout << entry.first;
                    }
                    std::cout << std::endl;
                }
                else{
                    reply = dept_to_server[request]; 
                    std::cout << request << " shows up in backend server " << reply << std::endl;
                }

                //TODO: will need to make sure all is sent
                if (send(new_fd, reply.c_str(), reply.length(), 0) == -1){
                    perror("send");
                }
                if(reply == "Not Found"){
                    std::cout << "The Main Server has sent “Department Name: Not found” to client " ;
                    std::cout << cur_client << " using TCP over port " << PORT << std::endl;
                }
                else{
                    std::cout << "Main Server has sent searching result to client " << cur_client;
                    std::cout << "using TCP over port " << PORT << std::endl;
                }

            }
        }

        close(new_fd);  // parent doesn't need this
    }

    return 0;
}