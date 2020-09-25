#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
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

	struct timeval start_time;
	struct timeval end_time;
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
		// File 이름을 입력받아 서버로 전송
		// 만약 입력 받은 값이 'quit'이면 서버와 클라이언트 모두 종료
		putchar('>');
		scanf("%s", buf);
		if (!strcmp(buf, "quit"))
			break;
		write(sock, buf, BUF_SIZE);
		
		// 서버로부터 받은 파일을 저장할 경로를 path 변수에 저장.
		// CLIENT_DIR에 설정된 경로와 위에서 입력받은 파일명을 더함.
		// ex) 'client/' + 'test.txt' => 'client/test.txt'
		strcpy(path, CLIENT_DIR);
		strcat(path, buf);
		
		// 입력한 파일이 서버에 존재하는지 확인.
		// 만약 서버로부터 'FOUND'가 전송되었으면 서버에 파일이 존재.
		// 만약 'NOT FOUND' 혹은 이외의 값이 전송되었으면 파일이 존재하지 않음.
		// 파일이 없으면 다시 파일명을 입력받는 부분으로 돌아감.
		len = read(sock, buf, BUF_SIZE);
		err_handling(len, -1, "read() error");
		if (strcmp(buf, "FOUND")) {
			puts("File Not Found");
			continue;
		}
		puts("Fild Found ...");

		// 파일의 정보를 서버로부터 받음.
		// file size와 file mode를 받으며
		// 위에서 설정한 파일 경로(path)와 file mode로 파일을 생성함.
		len = read(sock, &file_size, sizeof(off_t));
		err_handling(len, -1, "read() error");
		len = read(sock, &file_mode, sizeof(mode_t));
		err_handling(len, -1, "read() error");
		printf("... File Size : %ld\n", file_size);
		printf("... File Mode : %X\n", file_mode);
		fd = creat(path, file_mode);

		// 만약 파일 생성에 실패할 경우('client' 디렉토리가 없을 경우 등)
		// 서버에 'no'를 보내고 다시 파일 이름을 입력받는 부분으로 돌아감.
		// 서버는 'no'를 수신하면 파일을 전송하지 않고 다시 대기 상태.
		if (fd == -1) {
			fputs("Failed to create file : ", stdout);
			puts(path);
			strcpy(buf, "no");
			write(sock, buf, BUF_SIZE);
			continue;
		}
		// 파일이 정상적으로 생성되었으면 'ok'를 서버에 보내고
		// 파일 다운로드를 시작함.
		fputs("File created : ", stdout);
		puts(path);
		strcpy(buf, "ok");
		write(sock, buf, BUF_SIZE);
		
		// 서버로부터 BUF_SIZE만큼 데이터를 읽어 읽은 byte 수(len)만큼
		// 위에서 생성한 파일에 작성함.
		puts("Start download ...");
		gettimeofday(&start_time, NULL); // 시작하기 전 시간을 start_time에 저장
		while ((len = read(sock, buf, BUF_SIZE)) > 0) {
			write(fd, buf, len);
			// bytes_count에 현재까지 읽은 총 byte 수를 저장하고
			// 만약 bytes_count가 file_size보다 크거나 같다면 데이터 수신을 종료함.
			bytes_count += len;
			if (bytes_count >= file_size) break;
		}
		gettimeofday(&end_time, NULL); // 끝난 시간을 end_time에 저장

		// 끝난 시간에서 시작 시간을 빼서 수행 시간(work_time)을 계산함.
		work_time = (double)(end_time.tv_sec)
			+ (double)(end_time.tv_usec)/1000000.0
			- (double)(start_time.tv_sec)
			- (double)(start_time.tv_usec)/1000000.0;
		// 전송 받은 byte 수, 수행 시간, 초당 수신 bytes 수 등의 정보를 출력함.
		printf("... File Size : %ld\n", bytes_count);
		printf("... Time Spent : %.3lf\n", work_time);
		printf("... %.3lf b/s\n", (double)bytes_count/work_time);
		printf("... %.3lf kb/s\n", (double)(bytes_count/1024.0f)/work_time);
		printf("... %.3lf mb/s\n", (double)(bytes_count/1024.0f/1024.0f)/work_time);
		puts("Download complete");

		// 생성한 파일을 닫고 변수를 초기화함.
		close(fd);
		len = 0;
		bytes_count = 0;
	}
	// 서버에 'quit'를 전송하여 서버와 클라이언트 모두 종료되도록 함.
	strcpy(buf, "quit");
	write(sock, buf, BUF_SIZE);
	close(sock);
	return 0;
}

int err_handling(int err_chk, const int err_num, const char* message)
{
	// 만약 err_chk와 err_num이 같다면 에러를 출력하고 종료.
	// 아니라면 err_chk 값을 그대로 반환함.
	if (err_chk != err_num) {
		return err_chk;
	}

	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
