#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h> 
#include <semaphore.h>
#include <fcntl.h>

int create_tmp_file() {
    char filename[32] = {"./tmp_file-XXXXXX"};
    int fd = mkstemp(filename);
    unlink(filename);
    if (fd < 1) {
        perror("Creating tmp file failed\n");
        exit(1);
    }
    write(fd, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 20);
    return fd;
}

int main() {
    int fd = create_tmp_file();
    unsigned char* shmem = (unsigned char*)mmap(NULL, 100, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if (shmem == MAP_FAILED) {
        perror("Mapping failed\n");
        exit(1);
    }

    sem_t* sem1 = sem_open("/sem1", O_CREAT, 777, 0);
    sem_t* sem2 = sem_open("/sem2", O_CREAT, 777, 0);
    if (sem1 == SEM_FAILED || sem2 == SEM_FAILED) {
        perror("Semaphore opening falied\n");
        exit(1);
    }
    sem_unlink("/sem1");
    sem_unlink("/sem2");
    
    pid_t proc = fork();
    if (proc < 0) {
        printf("Error fork\n");
        exit(1);
    }

    if (proc == 0) {
        int x, y;
        int res;
        sem_wait(sem1);
        memcpy(&x, shmem, sizeof(x));
        memcpy(&y, shmem + sizeof(x), sizeof(y));
        
        if (x > 0 && y > 0)
            res = 1;        
        if (x < 0 && y > 0)
            res = 2;       
        if (x < 0 && y < 0)
            res = 3;        
        if (x > 0 && y < 0)
            res = 4;

        memcpy(shmem + sizeof(x) + sizeof(y), &res, sizeof(res));
        sem_post(sem2);
        sem_close(sem1);
        sem_close(sem2);
        munmap(shmem, 20);
        close(fd);
        exit(EXIT_SUCCESS);
    }
    else if (proc > 0) {
        int x, y;
        int res;
        scanf("%d %d", &x, &y);

        memcpy(shmem, &x, sizeof(x));
        memcpy(shmem + sizeof(y), &y, sizeof(y));
        
        sem_post(sem1);
        sem_wait(sem2);

        memcpy(&res, shmem + sizeof(x) + sizeof(y), sizeof(res));
        printf("%d\n", res);
    }
    
    sem_close(sem1);
    sem_close(sem2);
    munmap(shmem, 20);
    close(fd);
    exit(EXIT_SUCCESS);
}
