#include "common.h"

///////////////////////////////////////////////////////dataset/////////////////////////////////

vector<client_info> kep_client;
map<int, vector<int>> sockfd_reflects;

///////////////////////////////////////////////////////////////////////////////////////////////////

void show_clients() {
    int i = 0;
    cout << "\nclient info is:" << endl;
    for (i = 0; i < kep_client.size(); ++i) {
        cout << i << ":" << "addr:" << inet_ntoa (kep_client[i].addr.sin_addr) << ":" << ntohs (kep_client[i].addr.sin_port) << "  sockfd:" << kep_client[i].sockfd << endl;
    }
}

int add_client (int fd, struct sockaddr_in *addr) {
    client_info tep;
    tep.addr = *addr;
    tep.sockfd = fd;

    kep_client.push_back (tep);

    return kep_client.size();
}

int erase_client (int fd) {
    vec_client_iter i;
    for (i = kep_client.begin(); i != kep_client.end(); i++) {
        if (i->sockfd == fd) {
            kep_client.erase (i);
            return 1;
        }
    }
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////

int addtomap (int fd1, int fd2) {
    map_iv_iter tep1;

    tep1 = sockfd_reflects.find (fd1);


    if (tep1 != sockfd_reflects.end()) {
        if (find (tep1->second.begin(), tep1->second.end(), fd2) == tep1->second.end()) {
            tep1->second.push_back (fd2);
        }
    } else {
        vector<int > tep;
        tep.push_back (fd2);
        sockfd_reflects[fd1] = tep;
    }
    return sockfd_reflects.size();
}

int add_reflect (int fd1, int fd2) {
    addtomap (fd1, fd2);
    return addtomap (fd2, fd1);
}

//////////////////////////////////////////////////////////////////////////////////////

void signal_handler (int signo) { //處理殭屍進程
    if (signo == SIGCHLD) {
        pid_t pid;
        while ( (pid = waitpid (-1, NULL, WNOHANG)) > 0) {//WNOHANG 若pid指定的子进程没有结束，则waitpid()函数返回0，不予以等待。若结束，则返回该子进程的ID。
            cout << ("SIGCHLD pid ") << pid << endl;
        }
    }
}

int transfer (int src_sock) {
    /*
    one node tranfer to those who connect to this node
    */
    char buff[bufflen];
    int len = bufflen;
    int i;
    int getlen = 0, sendlen = 0,sendlen_all=0;

    if ( (getlen = recv (src_sock, buff, len, 0)) <= 0) {
        perror ("recv error");
        return -1;
    }

    map_iv_iter tep1;
    tep1 = sockfd_reflects.find (src_sock);
    if (tep1 != sockfd_reflects.end() && tep1->second.size()>0) {

        for (i = 0; i < tep1->second.size(); ++i) {

            if ( (sendlen = send (tep1->second[i], buff, getlen, 0)) <= 0) {
                perror ("send error");
                return -1;
            }
            sendlen_all+=sendlen;
        }
        cout << "get:" << getlen << "  send average:" << sendlen_all/i << endl;

        return sendlen_all/i;
    }
    return -1;
}

int build_conn (int sock1, int sock2) {
    int tep = fork();
    if (tep < 0) perror ("fork error");
    else if (!tep) { //child
        while (transfer (sock1, sock2) != -1) ;
        exit (0);
    } else {
        while (transfer (sock2, sock1) != -1) ;
        exit (0);
    }
    return 0;
}


int buildserver (int port) {
    int sockfd;

    struct sockaddr_in my_addr;

    if ( (sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
        perror ("couldn't build socket..");
        exit (-1);
    }

    cout << "get socket:" << sockfd << endl;

    my_addr.sin_family = AF_INET; /* host byte order */
    my_addr.sin_port = htons (port);   /* short, network byte order */
    my_addr.sin_addr.s_addr = htons (INADDR_ANY);

    bzero (& (my_addr.sin_zero), sizeof (my_addr.sin_zero));       /* zero the rest of the struct */


    if (bind (sockfd, (struct sockaddr*) &my_addr, sizeof (struct sockaddr)) == -1) {
        perror ("bind error:");
        exit (-1);
    }

    listen (sockfd, 20);
    cout << "listening in port:" << port << endl;
    return sockfd;
}

int mainloop() {
    int clientfd = 0, serverfd = 0;
    struct sockaddr_in client_addr;

    serverfd = buildserver (serverport); //build up server, listending

    size_t tep = sizeof (struct sockaddr);

    cout << "waiting to accept.." << endl;


    while (1) {
        if ( (clientfd = accept (serverfd, (struct sockaddr*) &client_addr, (socklen_t*) &tep)) == -1) {
            perror ("accept error..");
            exit (-1);
        }

        cout << "get client->ip:" << inet_ntoa (client_addr.sin_addr) << "port :" << ntohs (client_addr.sin_port);

        cout << "add client size:" << add_client (clientfd, &client_addr) << endl;
    }

    return -1;
}



int main() {
    signal (SIGCHLD, signal_handler);
    mainloop();

    /*
    int client2fd;
    struct sockaddr_in server_addr;

    if ( (client2fd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
        perror ("couldn't build socket..");
        exit (-1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons (22);
    server_addr.sin_addr.s_addr = inet_addr ("127.0.0.1");

    bzero (& (server_addr.sin_zero), sizeof (server_addr.sin_zero));

    cout << "connecting to " << inet_ntoa (server_addr.sin_addr) << ":" << ntohs (server_addr.sin_port) << endl;
    if (connect (client2fd, (struct sockaddr*) &server_addr, sizeof (struct sockaddr))  == -1) {
        perror ("server connect ssh server error..");
        exit (-1);
    }
    cout << "connected" << endl;


    /////////////////////////////////////////////////////up is client/////////
    build_conn (clientfd, client2fd);


    char p[] = "hello from server..";
    //cout<<sizeof(p)<<endl;
    int sd = 0;

    while (1) {

        if ( (sd = send (clientfd, (void*) p, sizeof (p), 0)) != sizeof (p)) {
            cout << "send error " << sd << "/" << sizeof (p) << endl;
        }

        cout << "send once" << sizeof (p) << endl;

        sleep (2);
    }
    */



    return 0;
}
