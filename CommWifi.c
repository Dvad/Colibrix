#include <C:\CSD\workspace\helloworld-oabi\src\Params.h>
#include <C:\CSD\workspace\helloworld-oabi\src\Pilote.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <netdb.h>

int Wifi initialize()
{
	//on crée les Socket
	static struct sockaddr_in local_addr;
	static struct sockaddr_in remote_addr;
	
	//Déclaration de variables nécéssaire
	unsigned int sin_size;
	static int sock;
	char buffer[255];
	
	// Build the server address & port
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(5643); // Listen on port 5555
	local_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all available IP
	memset(&(local_addr.sin_zero), '\0', 8);
	// Instanciate a socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	// Set the server address & port to the socket
	bind(sock, (struct sockaddr *)(&local_addr), sizeof(struct sockaddr));
	
	// Set the socket in listening mode
	if (listen(sock, 1) == -1)// 1 in the number of connections at the same time (connection queue size)
	{
		return -1;
	}
	return 0;
}
	while (1)
	{// Server loop
		int csock;

		// Wait for a client connection (accept block us until there is client in the queue)
		sin_size = sizeof(remote_addr);
		csock = accept(sock, (struct sockaddr_in*)(&remote_addr), &sin_size);
		// Here we have a new socket for the new client
		if (csock != -1 )
		{
			printf("trouvé");
			while (1){
				recv(csock, buffer, sizeof(buffer), 0);
				printf("%s", buffer);
				sleep(0.01);
			}
		}
	}
	
return 0;

}

