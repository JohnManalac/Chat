/*  Client and Server Chat System C-Implementation
*   V 0.3
*
*   Collaborators: John Manalac and Johana Ramirez 
*   Last changed: Dec 18, 2022.
*
*   Implementation Notes: 
*
*   BUF_SIZE is the max size for any message to be sent or received, including 
*   the new trailing line, null-terminator, and any extra diagnostic information,
*   such as the time or username. 
*
*   Server:
*
*       - Single command-line argument, which represents the port number to listen for incoming connections. 
*       - Every client is handled by a separate thread, with an arbritary number of clients supported.  
*       - When a client connects, the server prints a message identifying the new client by IP address and port number.
*       - When a client changes its nickname, the server prints a message indicating both old and new names. 
*         If this is the first time the client's nickname has been changed, the old name will be "unknown".
*       - When a client disconnects, the server prints a message a message indicating so.
*
*   Client:
*
*       - Two command-line arguments: the hostname and port number of the server.
*       - Upon receiving a message from the server, the client will print it out preceded by a timestamp.
*       - If the user enters end-of-file (ie, the user types Ctrl-D) or types exit, the client terminates with a descriptive message.
*       - If the connection is terminated (eg, the server process terminates), the client also terminates with a descriptive message.
*
*   System: 
*       - When a client sends a message, every client connected to the server will receive and print it out, with proper attribution.
*       - When a client disconnects from the server, the server notifies all remaining clients through printing an informative message.
*
*/
