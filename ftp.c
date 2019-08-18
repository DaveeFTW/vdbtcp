#include "ftp.h"
#include "log.h"
#include "strlcat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <psp2/net/netctl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/threadmgr.h>

void log_null(const char *fmt, ...)
{
}

#define LOG_TRACE LOG
#define LOG_ERROR LOG
#define LOG_DEBUG LOG
#define LOG_WARN LOG
#define LOG_INFO LOG

enum {
	MSG_MAX_SIZE = 1024,
};

enum {
	CHANNEL_COMMANDS,
	CHANNEL_DATA
};

int ftp_send(int socket, void *data, size_t len) {
	int ret;
	if ((ret = sceNetSend(socket, data, len, 0)) != len) {
		LOG_ERROR("sceNetSend failed to send all data, ret=%08x\n", ret);
		return -1;
	}
	return 0;
}

void ftp_send_command(ftp_client *client, int status, const char *msg, ...) {
	char formatted_msg[MSG_MAX_SIZE];
	va_list opt;
	va_start(opt, msg);
	vsnprintf(formatted_msg, sizeof(formatted_msg), msg, opt);
	va_end(opt);

	char buf[MSG_MAX_SIZE];
	snprintf(buf, sizeof(buf), "%d %s\r\n", status, formatted_msg);
	ftp_send(client->fd, buf, strlen(buf));
	LOG_TRACE("<= %s", buf);
}

int ftp_send_data(ftp_client *client, void *data, size_t len) {
	return ftp_send(client->data_socket, data, len);
}

void ftp_open_data(ftp_client *client) {
	int ret = 0;
	int socket = 0;
	struct SceNetSockaddrIn addr = { 0 };
	unsigned addrlen = sizeof(addr);
	int remote = 0;
	int port;

	socket = sceNetSocket("data", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
	if (socket < 0) {
		LOG_ERROR("PASV: sceNetSocket errno ret 0x%08X\n", ret);
		goto cleanup;
	}

	addr.sin_len = sizeof(addr);
	addr.sin_family = SCE_NET_AF_INET;
	addr.sin_port = sceNetHtons(0);

	ret = sceNetBind(socket, (SceNetSockaddr*)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERROR("PASV: sceNetBind ret 0x%08X\n", ret);
		goto cleanup;
	}
	ret = sceNetListen(socket, 5);
	if (ret < 0) {
		LOG_ERROR("PASV: sceNetListen ret 0x%08X\n", ret);
		goto cleanup;
	}
	sceNetGetsockname(socket, (SceNetSockaddr*)&addr, &addrlen);
	port = sceNetNtohs(addr.sin_port);
	SceNetCtlInfo info;
	sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
	for (char *c = info.ip_address; *c; ++c)
		if (*c == '.')
			*c = ',';
	ftp_send_command(client, 227, "Entering Passive Mode (%s,%d,%d)", info.ip_address, port >> 8, port & 0xFF);
	if ((remote = sceNetAccept(socket, (SceNetSockaddr*)&addr, &addrlen)) < 0) {
		LOG_ERROR("sceNetAccept() errno 0x%08X\n", ret);
		goto cleanup;
	}
	client->data_socket = remote;
	LOG_DEBUG("[%d] Got data socket: %d\n", client->fd, client->data_socket);
cleanup:
	if (socket)
		sceNetSocketClose(socket);
}

void ftp_close_data(ftp_client *client) {
	int ret = sceNetSocketClose(client->data_socket);
	if (ret < 0) {
		LOG_WARN("sceNetSocketClose ret=%08x\n", ret);
	}
	LOG_DEBUG("[%d] Closed data socket %d\n", client->fd, client->data_socket);
}

void ftp_USER(ftp_client *client, char *args) {
	ftp_send_command(client, 230, "Ok");
}

void ftp_SYST(ftp_client *client, char *args) {
	ftp_send_command(client, 215, "UNIX Type: L8");
}

void ftp_PASV(ftp_client *client, char *args) {
	ftp_open_data(client);
}

char *root_dirs[] = { "sd0:", "os0:", "vs0:", "vd0:", "tm0:", "ur0:", "host0:", "ud0:", "ux0:", 
	"gro0:", "grw0:", "sa0:", "mfa0:", "mfb0:", "lma0:", "lmb0:", "lmc0:", "lmd0:", "pd0:", 0};

const char* list_format = "%crwxr-xr-x   1 PSVita        %lld Feb  1  2009 %s\r\n";

void ftp_LIST(ftp_client *client, char *args) {
	int res, uid = 0, ok = 0;
	SceIoDirent dirent = { 0 };

	LOG_DEBUG("Listing directory %s\n", client->pwd);

	ftp_send_command(client, 150, "You got it");
	if (strcmp(client->pwd, "") == 0) {
		for (char **dir = &root_dirs[0]; *dir; ++dir) {
			char line[1024];
			snprintf(line, sizeof(line), list_format, 'd', (long long)4096, *dir);
			if (ftp_send_data(client, line, strlen(line)) < 0)
				goto cleanup;
		}
	} else {
		uid = sceIoDopen(client->pwd);
		if (uid < 0) {
			ftp_send_command(client, 451, "sceIoDopen returned %08x", uid);
			goto cleanup;
		}
		while ((res = sceIoDread(uid, &dirent))) {
			char line[1024];
			char dir = SCE_S_ISDIR(dirent.d_stat.st_mode) ? 'd' : '-';
			snprintf(line, sizeof(line), list_format, dir, dirent.d_stat.st_size, dirent.d_name);
			if (ftp_send_data(client, line, strlen(line)) < 0)
				goto cleanup;
		}
		if (res < 0) {
			ftp_send_command(client, 451, "sceIoDread returned %08x", res);
			goto cleanup;
		}
	}

	ok = 1;
cleanup:
	if (uid > 0)
		sceIoDclose(uid);
	ftp_close_data(client);

	if (ok)
		ftp_send_command(client, 226, "Completed");
}

void ftp_PWD(ftp_client *client, char *args) {
	ftp_send_command(client, 257, "\"/%s\"", client->pwd);
}

void ftp_TYPE(ftp_client *client, char *args) {
	ftp_send_command(client, 200, "Ok");
}

void ftp_CWD(ftp_client *client, char *args) {
	char tmp_path[PATH_MAX] = { 0 };
	char *path = args;
	if (strcmp(path, "..") == 0) {
		strlcat(tmp_path, client->pwd, sizeof(tmp_path));
		int len = strlen(tmp_path);
		if (len > 1) {
			char *cur;
			for (cur = tmp_path + len - 2; cur >= tmp_path && *cur != '/'; --cur)
				;
			*++cur = '\0';
		}
	} else if (path[0] == '/') {
		strlcat(tmp_path, path + 1, sizeof(tmp_path));
		if (path[strlen(path) - 1] != '/')
			strlcat(tmp_path, "/", sizeof(tmp_path));
	} else {
		strlcat(tmp_path, client->pwd, sizeof(tmp_path));
		strlcat(tmp_path, path, sizeof(tmp_path));
		strlcat(tmp_path, "/", sizeof(tmp_path));
	}
	int uid = sceIoDopen(tmp_path);
	if (strcmp(tmp_path, "") != 0 && uid < 0) {
		ftp_send_command(client, 550, "sceIoDopen returned %08x", uid);
	} else {
		if (uid > 0)
			sceIoDclose(uid);
		ftp_send_command(client, 250, "Ok");
		strcpy(client->pwd, tmp_path);
	}
}

void ftp_CDUP(ftp_client *client, char *args) {
	ftp_CWD(client, "..");
}

void ftp_RETR(ftp_client *client, char *args) {
	char buf[4 * 1024], path[PATH_MAX] = { 0 };
	int uid = 0, ok = 0, read = 0;

	ftp_send_command(client, 150, "Sending the file");
	strlcat(path, client->pwd, sizeof(path));
	strlcat(path, args, sizeof(path));
	uid = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (uid < 0)
		goto cleanup;
	while ((read = sceIoRead(uid, buf, sizeof(buf))) > 0) {
		if (ftp_send_data(client, buf, read) < 0)
			goto cleanup;
	}
	ok = 1;
cleanup:
	ftp_close_data(client);
	if (uid > 0)
		sceIoClose(uid);
	if (ok) {
		ftp_send_command(client, 226, "Ok");
	} else {
		ftp_send_command(client, 551, "Something bad happened: uid=%08x read=%08x", uid, read);
	}
}

void ftp_RNFR(ftp_client *client, char *args) {
	strncpy(client->rename_from, args, sizeof(client->rename_from));
	ftp_send_command(client, 350, "Ok");
}

static void concat_paths(char *output, size_t n, char *first, char *second) {
	if (second[0] == '/')
		strncpy(output, second + 1, n); // we receive paths like /ux0:/a/b/c but want ux0:/a/b/c
	else
		snprintf(output, n, "%s/%s", first, second);
}

void ftp_RNTO(ftp_client *client, char *args) {
	char old_path[PATH_MAX], new_path[PATH_MAX];
	int ret;
	concat_paths(old_path, sizeof(old_path), client->pwd, client->rename_from);
	concat_paths(new_path, sizeof(new_path), client->pwd, args);
	LOG_DEBUG("Rename %s => %s\n", old_path, new_path);
	if ((ret = sceIoRename(old_path, new_path)) < 0)
		ftp_send_command(client, 550, "Failed to rename, sceIoRename returned %08x", ret);
	else
		ftp_send_command(client, 250, "Success");
}

void ftp_MKD(ftp_client *client, char *args) {
	char path[PATH_MAX];
	int ret;
	concat_paths(path, sizeof(path), client->pwd, args);
	if ((ret = sceIoMkdir(path, 0777)) < 0)
		ftp_send_command(client, 550, "Failed to make the directory, sceIoMkdir returned %08x", ret);
	else
		ftp_send_command(client, 250, "%s", path);
}

void ftp_STOR(ftp_client *client, char *args) {
	char path[PATH_MAX], buf[4096];
	int uid = 0, ok = 0, ret = 0;
	concat_paths(path, sizeof(path), client->pwd, args);
	ftp_send_command(client, 150, "Accepted");
	uid = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (uid < 0) {
		LOG_WARN("Failed to open %s for writing, sceIoOpen returned %08x\n", path, uid);
		goto cleanup;
	}
	while ((ret = sceNetRecv(client->data_socket, buf, sizeof(buf), 0)) > 0) {
		size_t off = 0, to_write = ret;
		while (to_write > 0) {
			int written = sceIoWrite(uid, buf + off, to_write);
			if (written < 0) {
				LOG_ERROR("Error writing to file: sceIoWrite returned %08x\n", written);
				goto cleanup;
			}
			off += written;
			to_write -= written;
		}
	}
	if (ret < 0) {
		LOG_ERROR("Error while receiving file data: sceNetRecv ret=%08x\n", ret);
		goto cleanup;
	}
	ok = 1;
cleanup:
	ftp_close_data(client);
	if (uid > 0)
		sceIoClose(uid);
	if (ok)
		ftp_send_command(client, 226, "Ok");
	else
		ftp_send_command(client, 552, "Something went wrong");
}

void ftp_DELE(ftp_client *client, char *args) {
	char path[PATH_MAX];
	int ret;
	concat_paths(path, sizeof(path), client->pwd, args);
	if ((ret = sceIoRemove(path)) < 0)
		ftp_send_command(client, 550, "Failed to remove the file, sceIoRemove returned %08x", ret);
	else
		ftp_send_command(client, 250, "Ok");
}

void ftp_RMD(ftp_client *client, char *args) {
	char path[PATH_MAX];
	int ret;
	concat_paths(path, sizeof(path), client->pwd, args);
	if ((ret = sceIoRmdir(path)) < 0)
		ftp_send_command(client, 550, "Failed to remove the directory, sceIoRmdir returned %08x", ret);
	else
		ftp_send_command(client, 250, "Ok");
}

typedef struct command {
	const char* name;
	void (*func)(ftp_client*, char*);
} command;

command commands[] = {
	{ "USER", ftp_USER },
	{ "SYST", ftp_SYST },
	{ "PASV", ftp_PASV },
	{ "LIST", ftp_LIST },
	{ "PWD", ftp_PWD },
	{ "TYPE", ftp_TYPE },
	{ "CWD", ftp_CWD },
	{ "CDUP", ftp_CDUP },
	{ "RETR", ftp_RETR },
	{ "RNFR", ftp_RNFR },
	{ "RNTO", ftp_RNTO },
	{ "MKD", ftp_MKD },
	{ "STOR", ftp_STOR },
	{ "DELE", ftp_DELE },
	{ "RMD", ftp_RMD },
	{ 0, 0 }
};

int do_ftp_command(ftp_client *client, char *cmd, char *args) {
	command *c = &commands[0];
	while (c->name) {
		if (strcmp(c->name, cmd) == 0) {
			c->func(client, args);
			return 1;
		}
		++c;
	}
	return 0;
}

int do_ftp(int socket) {
	ftp_client client = { 0 };
	client.fd = socket;
	client.data_port = 0;
	strcpy(client.pwd, "");

	ftp_send_command(&client, 220, "Hello");
	int off = 0;
	for (;;) {
		char buf[MSG_MAX_SIZE] = { 0 };
		char *end;
		while (!(end = strchr(buf, '\n'))) {
			int res = sceNetRecv(client.fd, buf + off, sizeof(buf) - off - 1, 0);
			if (res == 0) {
				LOG_INFO("Closing connection\n");
				goto term;
			} else if (res < 0) {
				LOG_ERROR("sceNetRecv() errno 0x%08X -> 0x%08X\n", client.fd, res);
				goto term;
			} else {
				off += res;
				if (off >= sizeof(buf) - 1) {
					LOG_ERROR("Can't receive a command\n");
					goto term;
				}
			}
		}
		if (end == buf) {
			LOG_WARN("Received a malformed command\n");
			goto term;
		}
		*--end = '\0';
		char *args = strchr(buf, ' ');
		if (args) {
			*args = '\0';
			++args;
		} else {
			args = "[no arguments provided]";
		}
		char *cmd = buf;
		LOG_TRACE("=> %s %s\n", cmd, args);
		if (!do_ftp_command(&client, cmd, args)) {
			ftp_send_command(&client, 500, "Unknown command");
		}
		int next_cmd_off = end - buf + 2;
		memmove(buf, buf + next_cmd_off, sizeof(buf) - next_cmd_off);
		off -= next_cmd_off;
	}
term:
	LOG_INFO("Closed command socket %d\n", client.fd);
	return 0;
}
