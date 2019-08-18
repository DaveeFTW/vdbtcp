#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int do_ftp(int socket);

enum {
	PORT = 1337,
	PATH_MAX = 4096
};

typedef struct ftp_client {
	int fd;
	int data_socket;
	int data_port;
	char pwd[PATH_MAX];
	char rename_from[PATH_MAX];
} ftp_client;

#ifdef __cplusplus
}
#endif
