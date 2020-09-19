#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 61440 // 60kb
#define CLIENT_DIR "client/"

int err_handling(int err_chk, const int err_num, const char* message);

int main(int argc, char* argv[])
{
	int fd = 0;
	int len = 0;
	int sock = 0;
	int err_chk = 0;
	mode_t file_mode = 0;
	off_t file_size = 0;
	unsigned long bytes_count = 0;

	clock_t start_clock;
	clock_t end_clock;
	double work_time = 0;

	struct sockaddr_in serv_addr;

	char path[1024] = {0,};
	char buf[BUF_SIZE] = {0,};
	
	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	err_handling(sock, -1, "socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	err_chk = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	err_handling(err_chk, -1, "connect() error");
	
	while(1) {
		putchar('>');
		scanf("%s", buf);
		if (!strcmp(buf, "quit"))
			break;

		write(sock, buf, BUF_SIZE);
		// 
		strcpy(path, CLIENT_DIR);
		strcat(path, buf);
		
		len = read(sock, buf, BUF_SIZE);
		err_handling(len, -1, "read() error");
		if (strcmp(buf, "FOUND")) {
			puts("File Not Found");
			continue;
		}
		puts("Fild Found ...");

		len = read(sock, &file_size, sizeof(off_t));
		err_handling(len, -1, "read() error");
		len = read(sock, &file_mode, sizeof(mode_t));
		err_handling(len, -1, "read() error");
		printf("... File Size : %ld\n", file_size);
		printf("... File Mode : %X\n", file_mode);
		fd = creat(path, file_mode);

		if (fd == -1) {
			fputs("Failed to create file : ", stdout);
			puts(path);
			strcpy(buf, "no");
			write(sock, buf, BUF_SIZE);
			continue;
		}
		fputs("File created : ", stdout);
		puts(path);
		strcpy(buf, "ok");
		write(sock, buf, BUF_SIZE);

		puts("Start download ...");
		start_clock = clock();
		while ((len = read(sock, buf, BUF_SIZE)) > 0) {
			write(fd, buf, len);
			bytes_count += len;
			if (bytes_count >= file_size) break;
		}
		end_clock = clock();

		work_time = (double)(end_clock - start_clock) / CLOCKS_PER_SEC;
		printf("... File Size : %ld\n", bytes_count);
		printf("... Time Spent : %.3lf\n", work_time);
		printf("... %.3lf b/s\n", (double)bytes_count/work_time);
		printf("... %.3lf kb/s\n", (double)(bytes_count/1024.0f)/work_time);
		printf("... %.3lf mb/s\n", (double)(bytes_count/1024.0f/1024.0f)/work_time);
		puts("Download complete");

		close(fd);
		len = 0;
		bytes_count = 0;
	}
	strcpy(buf, "quit");
	write(sock, buf, BUF_SIZE);
	close(sock);
	return 0;
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
