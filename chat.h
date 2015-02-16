/*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT    
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _CHAT_H_
#define _CHAT_H_
 
/************************************************************************/
/*        Client/Server Application - Mutli-thread Chat Server          */
/*                                                                      */
/*        File: chat.h                                                  */
/************************************************************************/


#define _REENTRANT
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/sem.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define HOSTNAME_LENGTH 100         // maximum host name length
#define CLIENTNAME_LENGTH 100       // maximum client name length

/* This is exchange messge structure for message queue - used by both client and server */
struct exchg_msg {
    int instruction;    // action to be done
    int private_data;   // private data - used by different instructions */
                        // CMD_CLIENT_SEND/CMD_SERVER_BROADCAST - the length of the message
                        // CMD_SERVER_FAIL - the error code returns to the client
#define CONTENT_LENGTH	128
    char content[CONTENT_LENGTH];   // message content - expected to be terminated by a '\0' char
                                    // CMD_CLIENT_JOIN - carry the username
                                    // CMD_CLIENT_SEND/CMD_SERVER_BROADCAST - carry the chat message
};

/* Command instructions */
#define CMD_CLIENT_JOIN         100 // join the chat server
#define CMD_CLIENT_DEPART       101 // leave the chat server
#define CMD_CLIENT_SEND         102 // send a chat message to the chat room
#define CMD_SERVER_JOIN_OK      103 // a message carrying a reply message from chat server
#define CMD_SERVER_BROADCAST    104 // a chat message broadcasted by the chat server
#define CMD_SERVER_CLOSE        105 // the server closes
#define CMD_SERVER_FAIL         106 // the server incurs failure

/* ERROR code - these are the error codes returned with COMMAND_FAILURE by my server */
#define ERR_JOIN_DUP_NAME       200 // the new client has a duplicate name with another client
#define ERR_JOIN_ROOM_FULL      201 // server room is full
#define ERR_UNKNOWN_CMD         202 // unknown command
#define ERR_OTHERS              203 // other errors

#endif
