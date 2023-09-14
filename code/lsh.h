#include <sys/types.h>

typedef struct pid_list
{
    pid_t pid;
    struct pid_list *next;
} pid_list;

typedef struct pipe_node
{
    int fd[2];
    struct pipe_node *next;
} pipe_node;