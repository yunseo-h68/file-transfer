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
		// 클라이언트로부터 파일의 이름을 받음
		// 만약 파일의 이름이 'quit'이면 서버와 클라이언트 모두 종료
		len = read(clnt_sock, buf, BUF_SIZE);
		err_handling(len, -1, "read() error");
		if (!strlen(buf)) 
			continue;
		if(!strcmp(buf, "quit"))
			break;
		
		// 요청 받은 파일의 경로 출력
		putchar('\n');
		fputs("Requested File Path : ", stdout);
		puts(buf);
		
		// 파일을 찾을 경로를 path 변수에 저장
		// 기본적으로 SERVER_DIR 아래에서 파일을 찾으므로
		// path를 SERVER_DIR + 파일 경로로 설정함.
		// ex) 'server/' + 'test.txt' => 'server/test.txt'
		strcpy(path, SERVER_DIR);
		strcat(path, buf);

		// 요청 받은 파일을 여는 데 실패하면(파일이 없는 경우 등)
		// 'NOT FOUND'를 클라이언트에 전송하고 파일이름 수신 대기로 돌아감.
		// 파일을 여는 데 성공하면 'FOUND'를 클라이언트에 전송
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

		// 요청 받은 파일의 정보(file size, file mode)를 읽어들여
		// 클라이언트로 전송함
		err_chk = fstat(fd, &file_stat);
		err_handling(err_chk, -1, "fstat() error");
		file_size = file_stat.st_size;
		file_mode = file_stat.st_mode;
		puts("File Found");
		printf("- File Size : %ld\n", file_size);
		printf("- File Mode : %X\n", file_mode);
		write(clnt_sock, &file_size, sizeof(off_t));
		write(clnt_sock, &file_mode, sizeof(mode_t));
		
		// 클라이언트로부터 'ok' 신호가 오기를 대기함
		// 만약 'ok'가 아니라 다른 신호('no' 등)이 온다면
		// 파일 요청에 실패한 것으로 보고 파일이름 수신 대기로 돌아감.
		while((len = read(clnt_sock, buf, BUF_SIZE))&&!strlen(buf));
		err_handling(len, -1, "read() error");
		if (strcmp(buf, "ok")) {
			close(fd);
			puts("File request failed");
			continue;
		}
		
		// 만약 'ok' 신호가 왔다면
		// 클라이언트가 파일 수신 준비 상태인 것으로 보고
		// 파일 전송을 시작함.
		// 요청 받은 파일로부터 BUF_SIZE만큼 데이터를 읽어
		// 읽은 byte 수만큼 그대로 클라이언트에게 전송함
		puts("Start File Transfer ... ");
		while ((len = read(fd, buf, BUF_SIZE)) > 0) {
			write(clnt_sock, buf, len);
		}
		puts("File Transfer Completed");

		// 생성한 파일을 닫음
		close(fd);
	}
	// 서버와 클라이언트 모두 종료
	close(clnt_sock);
	close(serv_sock);
}

int err_handling(int err_chk, const int err_num, const char* message)
{
	// 만약 err_chk와 err_num이 같다면 에러를 출력하고 종료
	// 아니라면 err_chk 값을 그대로 반환함
	if (err_chk != err_num) {
		return err_chk;
	}
	
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
