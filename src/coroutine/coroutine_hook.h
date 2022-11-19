#ifndef CORPC_COROUTINE_COUROUTINE_HOOK_H
#define CORPC_COROUTINE_COUROUTINE_HOOK_H

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef ssize_t (*read_fun_ptr_t)(int fd, void *buf, size_t count);
typedef ssize_t (*write_fun_ptr_t)(int fd, const void *buf, size_t count);
typedef int (*connect_fun_ptr_t)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
typedef int (*accept_fun_ptr_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
typedef int (*socket_fun_ptr_t)(int domain, int type, int protocol);
typedef int (*sleep_fun_ptr_t)(unsigned int seconds);


namespace corpc {

int accept_hook(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t read_hook(int fd, void *buf, size_t count);
ssize_t write_hook(int fd, const void *buf, size_t count);
int connect_hook(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
unsigned int sleep_hook(unsigned int seconds);
void setHook(bool);

}

extern "C" {

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
unsigned int sleep(unsigned int seconds);

}

#endif