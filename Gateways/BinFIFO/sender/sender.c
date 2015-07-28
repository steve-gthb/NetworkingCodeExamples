#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>



static int create_socket(struct in_addr *remote_ip, int port) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr = *remote_ip;

    connect(sock, (const struct sockaddr*) &sockaddr, sizeof(sockaddr));

	return sock;
}


// The hardware will accept a variable sized frame, but my test is simple...

typedef struct __attribute__((packed)) {
	uint64_t size;
	uint64_t a;
	uint64_t b;
	uint64_t c;
} message_t;

static void send_frames(int sock) {
	message_t *m = malloc(sizeof(message_t));
	
	m->size = sizeof(message_t);
	m->a = '0';
	m->b = '1';
	m->c = '2';

	printf("Sending %zd bytes\n", m->size);
	send(sock, m, m->size, MSG_WAITALL);
}


int main(int argc, char *argv[]) {
	char *ip = "172.16.50.1";
	if (argc >= 2) ip = argv[1];

	struct in_addr remote_ip;
	inet_aton(ip, &remote_ip);
	int port = 1000;

	printf("Remote host: %s:%d\n", ip, port);
	
	for (int p=0; p < 1024; p++) {
		int mySocket = create_socket(&remote_ip, port + p);
		send_frames(mySocket);
		close(mySocket);
	}

	printf("Sender finished\n");
	getchar();
}
