/*
** selectserver.c -- a cheezy multiperson chat server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "9034"   // port we're listening on
#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void tellEachOther(int i, int j){
    char buf[50];
    char buf2[50];
    sprintf(buf, "now you are talking to %d\n", i);
    sprintf(buf2, "now you are talking to %d\n", j);
    // tell to userId
    send(i, buf2, strlen(buf2), 0);
    send(j, buf, strlen(buf2), 0);
}

void listUsers(int i,int j, int fdmax, struct fd_set master, int listener){
    char msg[256]= "--now online--\n";
    int x=0;
    for(j = 0; j <= fdmax; j++) {
        if (FD_ISSET(j, &master)) {
            // except the listener and ourselves
            if (j != listener) {
                //convert int to string
                char str[15];
                sprintf(str, "%d", j);
                strcat(msg, "user");
                if(j == i)
                    strcat(msg, "(you)");
                strcat(msg, str);
                strcat(msg, "\n");
                x++;
            }
        }
    }
    //count the users
    strcat(msg, "user counts: ");
    char str2[15];
    sprintf(str2, "%d", x);
    strcat(msg, str2);
    
    if (FD_ISSET(i, &master)) {
        // except the listener and ourselves
        if (send(i, msg, strlen(msg), 0) == -1) {
            perror("send");
        }
    }
}

int main(void)
{
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer for client data
    uint32_t recvId;
    int nbytes;
    int talking[100]= {0};  //talk to who pairs

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    printf("server: waiting for connections...\n");

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                        //ask who to talk to
                        char *str="you're online now\n";
                        send(newfd, str, strlen(str), 0);
                        //show list
                        //send mesage to someone
                    }
                } else {
                    // handle data from a client
                    //recv
                    //switch cmd
                    //show -> list all
                    //talk -> get id, and pair with each other
                    if ((nbytes = recv(i, &buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        // split cmd
                        char *cmd;
                        cmd= strtok(buf, " ");
                        if (strncmp(cmd, "show",4) == 0) 
                        {
                           listUsers(i,j, fdmax, master, listener);
                        } 
                        else if (strncmp(cmd, "talk",4) == 0)
                        {
                            cmd= strtok(NULL, " ");
                            //get the userId
                            int userId= atoi(cmd);
                            //get the msg
                            char *msg= strtok(NULL, " ");

                            //send the msg
                            if (FD_ISSET(userId, &master)) {
                                // except the listener and ourselves
                                if (send(userId, msg, strlen(msg), 0) == -1) {
                                    perror("send");
                                }
                            }
                            
                        }
                        else
                        {
                        }
                    }
                    

                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}