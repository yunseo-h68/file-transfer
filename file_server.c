#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 61440 // 60kb
#define SERVER_DIR "server/"

int err_handling(int err_chk, const int err_num, const char* message);

int main(int argc, char* argv[])
{
	struct stat file_stat;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;
	
	int fd = 0;
	int len = 0;
	int serv_sock = 0;
	int clnt_sock = 0;
	int err_chk = 0;
	mode_t file_mode = 0;
	off_t file_size = 0;

	char path[1024] = {0,};
	char buf[BUF_SIZE] = {0,};

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	err_handling(serv_sock, -1, "socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	err_chk = bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	err_handling(err_chk, -1, "bind() error");
	err_chk = listen(serv_sock, 5);
	err_handling(err_chk, -1, "listen() error");

	clnt_addr_size = sizeof(clnt_addr);
	clnt_sock = accept(serv_sock, (struct sockaddr*) &clnt_addr, &clnt_addr_size);
	err_handling(clnt_sock, -1, "accept() error");

	while(1) {
		len = read(clnt_sock, buf, BUF_SIZE);
		err_handling(len, -1, "read() error");
		if (!strlen(buf)) 
			continue;
		if(!strcmp(buf, "quit"))
			break;

		putchar('\n');
		fputs("Requested File Path : ", stdout);
		puts(buf);
		
		//
		strcpy(path, SERVER_DIR);
		strcat(path, buf);
		fd = open(path, O_RDONLY);
		if (fd == -1) {
			fputs("... NOT FOUND : ", stdout);
			puts(path);
			strcpy(buf, "NOT FOUND");
			write(clnt_sock, buf, BUF_SIZE);
			puts("File request failed");
			continue;
		}
		strcpy(buf, "FOUND");
		write(clnt_sock, buf, BUF_SIZE);

		err_chk = fstat(fd, &file_stat);
		err_handling(err_chk, -1, "fstat() error");
		file_size = file_stat.st_size;
		file_mode = file_stat.st_mode;

		puts("File Found");
		printf("- File Size : %ld\n", file_size);
		printf("- File Mode : %X\n", file_mode);
		write(clnt_sock, &file_size, sizeof(off_t));
		write(clnt_sock, &file_mode, sizeof(mode_t));

		while((len = read(clnt_sock, buf, BUF_SIZE))&&!strlen(buf));
		err_handling(len, -1, "read() error");
		if (strcmp(buf, "ok")) {
			close(fd);
			puts("File request failed");
			continue;
		}

		puts("Start File Transfer ... ");
		while ((len = read(fd, buf, BUF_SIZE)) > 0) {
			write(clnt_sock, buf, len);
		}
		puts("File Transfer Completed");
		close(fd);
	}
	close(clnt_sock);
	close(serv_sock);
}

int err_handling(int err_chk, const int err_num, const char* message)
{
	if (err_chk != err_num) {
		return err_chk;
	}
	
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
