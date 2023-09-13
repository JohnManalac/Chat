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
*   Notes/Questions:
*   
*   If the client fails to create any thread for receiving or sending messages, 
*   the entire process is aborted. We also exit the process if read or recv fail. 
*   
*   All perror_exit calls in the client will automatically close the socket conn_fd.
*   The server will appropriatetely close the socket listen_fd before certain perror_exit calls. 
*   
*   If the server fails to create a thread for any client, we delete that client and print an error message. 
*   Should we exit the server in this case instead? We thought it would be better for the server
*   to simply ignore the one client that failed. 
*
*
*   Sources:
*       
*       Manpages:
*           - https://man7.org/linux/man-pages/man3/strcpy.3.html
*           - https://man7.org/linux/man-pages/man3/fflush.3.html
*           - https://linux.die.net/man/3/strncpy
*           - https://man7.org/linux/man-pages/man2/kill.2.html
*           - https://man7.org/linux/man-pages/man2/socket.2.html
*           - https://man7.org/linux/man-pages/man2/time.2.html
*           - https://man7.org/linux/man-pages/man3/strcmp.3.html
*           - https://man7.org/linux/man-pages/man2/sigaction.2.html
*           - https://man7.org/linux/man-pages/man2/bind.2.html
*           - https://man7.org/linux/man-pages/man2/listen.2.html
*           - https://man7.org/linux/man-pages/man2/getaddrinfo.2.html
*           - https://man7.org/linux/man-pages/man2/recv.2.html
*           - https://linux.die.net/man/3/inet_ntoa
*           - https://man7.org/linux/man-pages/man3/pthread_create.3.html
*           - https://man7.org/linux/man-pages/man2/connect.2.html
*           - https://man7.org/linux/man-pages/man2/accept.2.html
*
*       Class:
*           - Pete
*           - http://www.cs.middlebury.edu/~pjohnson/courses/f22-0315/assignments/07/
*           - http://www.cs.middlebury.edu/~pjohnson/courses/f22-0315/lectures/19/notes
*           - http://www.cs.middlebury.edu/~pjohnson/courses/f22-0315/lectures/18/notes
*           - http://www.cs.middlebury.edu/~pjohnson/courses/f22-0315/lectures/15/notes
*           - http://www.cs.middlebury.edu/~pjohnson/courses/f22-0315/lectures/14/notes
*       
*       Other:
*           - https://stackoverflow.com/questions/21116315/difference-between-pthread-exitpthread-canceled-and-pthread-cancelpthread-sel
*           - https://www.educative.io/answers/how-to-copy-a-string-using-strcpy-function-in-c
*           - https://stackoverflow.com/questions/49991525/how-to-add-n-at-the-end-of-a-string-in-c
*           - https://www.ibm.com/docs/en/aix/7.2?topic=programming-terminating-threads
*           - https://www.gnu.org/software/libc/manual/html_node/Sigaction-Function-Example.html
*           - https://www.educative.io/answers/how-to-create-a-simple-thread-in-c
*           - https://stackoverflow.com/questions/60284105/how-can-i-use-sprintf-for-print-to-a-char
*           - https://www.freecodecamp.org/news/c-break-and-continue-statements-loop-control-statements-in-c-explained/#:~:text=In%20C%2C%20if%20you%20want%20to%20skip%20iterations%20in%20which,which%20the%20condition%20is%20true.
*           - https://man7.org/linux/man-pages/man1/continue.1p.html
*/
