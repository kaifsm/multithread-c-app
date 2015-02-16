/*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT    
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "chat.h"
#include "chat_server.h"
#include <string.h>
#include <signal.h>
#include <assert.h>  
#include <stdbool.h>

static char banner[] =
"\n\n\
/*****************************************************************/\n\
/*                                         			 */\n\
/*    Client/Server Application - Mutli-thread Chat Server       */\n\
/*                                                               */\n\
/*    USAGE:  ./chat_server    [port]                            */\n\
/*            Press <Ctrl + C> to terminate the server           */\n\
/*****************************************************************/\n\
\n\n";

/* 
 * Debug option 
 * In case you do not need debug information, just comment out it.
 */
#define CSIS0234_COMP3234_DEBUG

/* 
 * Use DEBUG_PRINT to print debugging info
 */
#ifdef CSIS0234_COMP3234_DEBUG
#define DEBUG_PRINT(_f, _a...) \
    do { \
        printf("[debug]<%s> " _f "\n", __func__, ## _a); \
    } while (0)
#else
#define DEBUG_PRINT(_f, _a...) do {} while (0)
#endif

int port = DEFAULT_LISTEN_PORT;
struct chat_server  chatserver;

struct sockaddr_in address_2; 
struct sockaddr_in address_1; 
socklen_t ssize; 

int file_d, sock_fd, bytes;  

struct exchg_msg msgcl;  
struct chat_room croom;  

struct chatmsg_queue * cmsgq = & (croom.chatmsgQ);  
struct client_queue * cqueue = & (croom.clientQ);  


void *broadcast_thread_fn(void *);

void *client_thread_fn(void *);

void server_init(void);

void server_run(void);

void shutdown_handler(int);

bool dup_check (struct client_queue *, char *);

void delete_from_queue (struct client_queue *, struct chat_client * );

void add (struct client_queue *, struct chat_client *);

/*
 * The main server process
 */
int main(int argc, char **argv)
{
    printf("%s\n", banner);
    
    if (argc > 1) {
        port = atoi(argv[1]);
    } else {
        port = DEFAULT_LISTEN_PORT;
    }

    printf("Starting chat server ...\n");
    
    signal(SIGINT, shutdown_handler);
    signal(SIGTERM, shutdown_handler);

    server_init();
    
    server_run();

    return 0;
}

void server_init(void)
{

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {   perror("socket error");
        exit(1);
    }
  
    address_1.sin_family = AF_INET;         

    address_1.sin_port = htons(port);       

    address_1.sin_addr.s_addr = htonl(INADDR_ANY); 

    memset(&(address_1.sin_zero), '\0', 8); 

     if (bind(sock_fd, (struct sockaddr *)&address_1, sizeof(struct sockaddr)) == -1) 
    {   perror("bind error");
        exit(1);
    }

     if (listen(sock_fd, BACKLOG) == -1) 
    {   perror("listen error");
        exit(1);
    }

        ssize = sizeof(struct sockaddr_in);

        cqueue->head = NULL;
        cqueue->tail = NULL;
        cqueue->count = 0;
    
        cmsgq->head = 0;
        cmsgq->tail = 0;
    

    sem_init(& (cmsgq -> buffer_full), 0, MAX_QUEUE_MSG);
    sem_init(& (cmsgq -> buffer_empty), 0, 0);            
    sem_init(& (cmsgq -> mq_lock), 0, 1);
    
    pthread_create(& (croom.broadcast_thread), NULL, (void *) & broadcast_thread_fn, (void *) (cmsgq));
} 

    void server_run(void)
  {
         while (1) 
    {   
        if ((file_d = accept(sock_fd, (struct sockaddr *)&address_2, &ssize)) == -1) 
        {   perror("accept");
            exit(1);
        }

        if ((recv(file_d, &msgcl, sizeof(msgcl), 0)) == -1) 
        {   perror("recv");
            exit(1);
        } 

        if ( ntohl(msgcl.instruction) == CMD_CLIENT_JOIN )
        {   
            if (cqueue->count > MAX_ROOM_CLIENT) 
            {   msgcl.instruction = htonl(CMD_SERVER_FAIL);
                msgcl.private_data = htonl(ERR_JOIN_ROOM_FULL);
                strcpy(msgcl.content,"Nil");
                if (send(file_d, &msgcl, sizeof(msgcl), 0) == -1) 
                {   perror("Server socket sending error");
                    exit(1);
                }
            }

            else if (cqueue->head == NULL)
            {   struct chat_client *client_create = (struct chat_client *) malloc (sizeof (struct chat_client));
                
 		client_create->socketfd = file_d;
                client_create->address = address_2;
                
		strcpy(client_create->client_name, msgcl.content); 
                add(cqueue, client_create);                   
                
pthread_create(& (client_create->client_thread), NULL, (void *) & client_thread_fn, (void *) (client_create));
     
}
            else if (dup_check(cqueue, msgcl.content))  
            {   msgcl.instruction = htonl(CMD_SERVER_FAIL);
                msgcl.private_data = htonl(ERR_JOIN_DUP_NAME);
                strcpy(msgcl.content,"Nil");
                if (send(file_d, &msgcl, sizeof(msgcl), 0) == -1) 
                {   perror("Server socket sending error");
                    exit(1);
                }
            }

            else 
            {   struct chat_client *client_create = (struct chat_client *) malloc (sizeof (struct chat_client));
                client_create->socketfd = file_d;
                client_create->address = address_2;
                strcpy(client_create->client_name, msgcl.content);
                add(cqueue, client_create);                    
pthread_create(& (client_create->client_thread), NULL, (void *) & client_thread_fn, (void *) (client_create));            
            }
        }

        else
        {   msgcl.instruction = htonl(CMD_SERVER_FAIL);
            msgcl.private_data = htonl(ERR_UNKNOWN_CMD);
            strcpy(msgcl.content,"Nil");

            if ( send (file_d, &msgcl, sizeof(msgcl), 0) == -1 ) 
            {   perror ("Server socket sending error");
                exit(1);
            }   
        }       
    }
}
 

void *client_thread_fn(void *arg)
{
    int new_filed;
    
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    struct chat_client * client;

    char cname[CLIENTNAME_LENGTH];
    
    client = (struct chat_client *) arg;
    
    new_filed = client->socketfd;
    strcpy(cname,client->client_name);

    struct exchg_msg join_r;     
    join_r.instruction = htonl(CMD_SERVER_JOIN_OK);
    join_r.private_data = htonl(-1);
    strcpy(join_r.content, "Nil");

    if ( send (new_filed, &join_r, sizeof(join_r), 0) == -1 ) 
    {   perror ("Server socket sending error");
        exit(1);
    } 


    char welcome_message[CONTENT_LENGTH];  
    strcpy (welcome_message, cname);
    strcat (welcome_message, " just joins the chat room, welcome!");

    sem_wait (& (cmsgq -> buffer_full));           
    sem_wait (& (cmsgq -> mq_lock));                

    cmsgq -> slots [cmsgq->tail] = welcome_message; 
    cmsgq -> tail = (cmsgq -> tail + 1) % 8 ;

    sem_post(& (cmsgq -> mq_lock));      
    sem_post(& (cmsgq -> buffer_empty));           

    while (1) 
    {   
        struct exchg_msg msgs;    

        if ((recv(new_filed, &msgs, sizeof(msgs), 0)) == -1) 
        {   perror("recv");
            exit(1);
        }

        if ( ntohl(msgs.instruction) == CMD_CLIENT_SEND )
        {   
	    sem_wait(& (cmsgq -> buffer_full));           
            sem_wait(& (cmsgq -> mq_lock));                

            char cmsg[CONTENT_LENGTH];   
            
	    strcpy(cmsg, cname);
            strcat(cmsg, ": ");
            strcat(cmsg, msgs.content);
            
            cmsgq->slots[cmsgq->tail] = cmsg;
            cmsgq -> tail = (cmsgq -> tail + 1) % 8 ;

            sem_post(& (cmsgq -> mq_lock));      
            sem_post(& (cmsgq -> buffer_empty));
        }
        
        else if (ntohl(msgs.instruction) == CMD_CLIENT_DEPART)
        {
            sem_wait(& (cmsgq -> buffer_full));           
            sem_wait(& (cmsgq -> mq_lock)); 

            strcpy(msgs.content,cname);
            strcat(msgs.content," just leaves the chat room, goodbye!");
   
            cmsgq->slots[cmsgq->tail] = msgs.content;
            cmsgq -> tail = (cmsgq -> tail + 1) % 8 ;

            sem_post(& (cmsgq -> mq_lock));      
            sem_post(& (cmsgq -> buffer_empty));

            delete_from_queue (cqueue, client);

            close(new_filed);

            pthread_detach(pthread_self()); 
            pthread_exit(0);  

            break;
        }

        else
        {   msgs.instruction = htonl(CMD_SERVER_FAIL);
            msgs.private_data = htonl(ERR_UNKNOWN_CMD);
            strcpy(msgs.content,"Nil");

            if (send(new_filed, &msgs, sizeof(msgs), 0) == -1) {
                perror("Server socket sending error");
                exit(1);
            }

        }
        
    }
}

void delete_from_queue (struct client_queue * cqueue, struct chat_client * client)
{ 
    if (cqueue->count != 0)

  {
        struct chat_client* current = cqueue-> head;

        if (cqueue->count == 1)
        {  
            cqueue->head = NULL;
            cqueue->tail = NULL;        
        }
        else if (cqueue->tail->socketfd == client->socketfd)
        {  
            client->prev->next = NULL;
            cqueue->tail = client->prev;
        }
        else if (cqueue->head->socketfd == client->socketfd)
        {  
            cqueue->head = client->next;
            client->next->prev = NULL;
        }
        else 
        {  
            current = current->next;     
            while (current != NULL)
            {   if (current->socketfd == client->socketfd)
                {   client->prev->next = client -> next;
                    client->next->prev = client -> prev;
                    break;
                }
            current = current->next;
            }
        }
        free(client);
        cqueue->count --;
    }
}

void *broadcast_thread_fn(void *arg)
{

    int new_filed;  

    struct exchg_msg broadcast;   
    struct chat_client * client ;
    
 
    broadcast.instruction = htonl(CMD_SERVER_BROADCAST); 


    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (1) 
    {   
        sem_wait(& (cmsgq -> buffer_empty));           
        sem_wait(& (cmsgq -> mq_lock));   

        strcpy(broadcast.content, cmsgq->slots[cmsgq->head]);

        int mlen; 

        mlen = strlen(broadcast.content) + 1;
        mlen = (mlen < CONTENT_LENGTH) ? mlen : CONTENT_LENGTH;
        broadcast.content[mlen-1] = '\0';
        broadcast.private_data = htonl(mlen);

        client = cqueue -> head;

        while (client != NULL)
        {   new_filed = client -> socketfd; 

            if (send(new_filed, &broadcast, sizeof(broadcast), 0) == -1) 
            {   perror("Server socket sending error");
                exit(1);
            }
            client = client -> next;
        }

        cmsgq -> head = (cmsgq -> head + 1) % 8 ;

        sem_post(& (cmsgq -> mq_lock));               
        sem_post(& (cmsgq -> buffer_full));           
    }
    pthread_detach(pthread_self());                        
    pthread_exit(0);
}

bool dup_check (struct client_queue * cqueue, char * cname)
{   
    struct chat_client* current = cqueue->head;
    
    while (current != NULL)
    {
        if (strcmp(current->client_name, cname) == 0)
        {   return true;
        }
        
        current = current-> next;
    }
    return false;
}

void shutdown_handler(int signum)
{

    pthread_cancel (croom.broadcast_thread);
    pthread_join (croom.broadcast_thread, NULL);

    struct exchg_msg closem;		  
    struct chat_client * head, * next;    

    head = cqueue->head;
    closem.instruction = htonl(CMD_SERVER_CLOSE);
    closem.private_data = htonl(-1);    

    while(head != NULL) 
    { 
        if (send(head->socketfd, &closem, sizeof(closem), 0) == -1) 
        { perror("send"); } 
       
        next = head->next; 
        pthread_cancel (head->client_thread);
        pthread_join (head->client_thread, NULL);
        close(head->socketfd);
        free(head);
        head = next;
    } 

    sem_destroy(& (cmsgq -> buffer_full));
    sem_destroy(& (cmsgq -> buffer_empty));
    sem_destroy(& (cmsgq -> mq_lock));

    exit(0);
}

void add (struct client_queue * client, struct chat_client * chatcl)  
{
    if(client->tail != NULL)
    { 
        client->tail->next = chatcl;
        chatcl->prev = client->tail;
        chatcl->next = NULL;
        client->tail = chatcl;
    }
    else 
    { 
        client->head = chatcl;
        client->tail = chatcl;
        chatcl->next = NULL;
        chatcl->prev = NULL;
    }
    client->count++;
}
