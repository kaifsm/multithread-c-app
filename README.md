# 
#
#
This is a simple internet chat program configured to run on Linux platforms. The server program communicates with the client chat program by establishing a TCP connection. As the SICP involves more than two processes, which are
communicating via message passing, a standard chat protocol has been adopted. The chat server is currently designed to have only one chat room.

There are total 6 commands available for the user:
1) user: this command is used for setting the user identity
2) join [server-name] [port number]: this command is used to connect to a chat server
3) send [message]: this command is used to send a message to the chat server
4) depart: this command is used to exit the chat room
5) clear: this command clears the command window
6) exit: to exit the chat client
