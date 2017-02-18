#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <stdlib.h>
#include <limits.h>
#include <ev.h>

#define PORT 8080
#define BUFLEN 512
#define DOCROOT "/var/www"
enum method {UNKNOWN, GET, PUT};

unsigned long g_num_clients = 0;

void stop_and_clean_up_socket_watcher(struct ev_loop *loop, struct ev_io *watcher);
void accept_callback(struct ev_loop *loop, struct ev_io *watcher, int events);
void read_callback(struct ev_loop *loop, struct ev_io *watcher, int events);
void parse_request(const char * buffer, int fd);
void send_page(char * urlpath, int fd);
void send_404(int fd);

int main(int argc, const char* argv[])
{
	int fd6;
	struct sockaddr_in6 addr6;
	struct ev_io accept_watcher;
	struct ev_loop *main_loop = ev_default_loop(0);

	/* Prep the address by zeroing it out and filling in the sockaddr_in struct */

	bzero(&addr6, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(PORT);
	addr6.sin6_addr = in6addr_any;
	
	/* Create socket and assign it to fd6 */

	if( (fd6 = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
		perror("Socket error");
		return -1;
	}

	/* Bind the socket to the address addr */

	if (bind(fd6, (const struct sockaddr *) &addr6, sizeof(addr6)) != 0) {
		perror("Bind error");
		return -1;
	}

	/* Listen on socket fd6 */	

	if (listen(fd6, 10) < 0) {
		perror("Listen error");
		return -1;
	}
    	
	/* Init and start the main event loop */

	ev_io_init(&accept_watcher, accept_callback, fd6, EV_READ);
	ev_io_start(main_loop, &accept_watcher);
	ev_run(main_loop, 0);

	return 0;
}

void stop_and_clean_up_socket_watcher(struct ev_loop *loop, struct ev_io *watcher)
{
	shutdown(watcher->fd, SHUT_RDWR);
	ev_io_stop(loop,watcher);
	free(watcher);
}

void accept_callback(struct ev_loop *loop, struct ev_io *watcher, int events)
{
	struct sockaddr_in6 client_addr6;
	socklen_t client_len;
	struct ev_io *client_watcher;
	int client_fd;

	/* Check early on if the event is invalid */

	if(EV_ERROR & events) {
		perror("Invalid event");
		return;
	}

	client_watcher = (struct ev_io*) malloc (sizeof(struct ev_io));
	if(client_watcher == NULL) {
		perror("Could not allocate memory for client watcher.");
		return;
	}

	client_len = sizeof(client_addr6);

	client_fd = accept(watcher->fd, (struct sockaddr *)&client_addr6, &client_len);

	if (client_fd < 0) {
		perror("Accept error");
		return;
	}

	g_num_clients++;
	printf("One client connected. Total now %ld clients.\n", g_num_clients);

	/* Start watcher for this socket */
	ev_io_init(client_watcher, read_callback, client_fd, EV_READ);
	ev_io_start(loop, client_watcher);
}

/* Read client message */
void read_callback(struct ev_loop *loop, struct ev_io *watcher, int events)
{
	char buffer[BUFLEN];
	ssize_t read_bytes;

	/* Check early on if the event is invalid */

	if(EV_ERROR & events) {
		perror("Invalid event");
		return;
	}

	read_bytes = recv(watcher->fd, buffer, BUFLEN, 0);

	if(read_bytes < 0) {
		perror("Read error");
		
		return;
	} else if(read_bytes == 0) { /* Socket shutdown from the other end */
		if(g_num_clients > 0) {
			g_num_clients--;
			printf("One client disconnected. Total now %ld clients.\n", g_num_clients);
		} else {
			fprintf(stderr, "Number of clients dropped below zero. This should not happen.\n");
		}

		stop_and_clean_up_socket_watcher(loop, watcher);

		return;
	} else {
		parse_request(buffer, watcher->fd);
	}
}

void parse_request(const char * buffer, int fd)
{
	const char delimiters[] = " ";
	char *token, *bufcopy;
	char urlpath[PATH_MAX];
	enum method parsed_method = UNKNOWN;

	bufcopy = strdupa(buffer); /* Make working copy.  */

	token = strtok (bufcopy, delimiters);

	printf("%s\n", token);
	if(strncmp(token, "GET", 3)==0) {
        	parsed_method = GET;
	} else {
		return;
	}

	token = strtok (NULL, delimiters);
	strncpy(urlpath, token, PATH_MAX);

	token = strtok (NULL, delimiters);
	printf("%s\n", token); /* The HTTP version, only 1.0 right now */

	send_page(urlpath, fd);

	return;
}

void send_page(char * urlpath, int fd)
{
	struct stat filestat;
	char filename_recv[PATH_MAX];
	char filename[PATH_MAX];
	off_t offset = 0;
	int urlfile;

	if(urlpath[(strlen(urlpath)-1)]=='/') {
		snprintf(filename_recv, PATH_MAX, "%s/index.html", DOCROOT);
	} else {
		snprintf(filename_recv, PATH_MAX, "%s%s", DOCROOT, urlpath);
	}

	if(realpath(filename_recv, filename)==NULL) {
		send_404(fd);
		return;
	}

	if(strncmp(filename, DOCROOT, strlen(DOCROOT))!=0) {
		send_404(fd);
		printf("Access violation: Attempted %s\n", filename);
		return;
	}

	printf("Retrieving %s...\n", filename);

	urlfile = open(filename, O_RDONLY);
	if (urlfile < 0) {
	    send_404(fd);
	    return;
	}

	fstat(urlfile, &filestat);

	if(sendfile(fd, urlfile, &offset, filestat.st_size) < 0) {
		perror("sendfile error");
		exit(1);
	}

	return;
}



void send_404(int fd)
{
}
