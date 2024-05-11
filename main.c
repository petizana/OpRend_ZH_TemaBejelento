#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   //fork
#include <sys/wait.h> //waitpid
#include <errno.h>

#include <signal.h>
#include <sys/types.h>

#include <string.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <wait.h>

#include <sys/time.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <sys/types.h>
#include <sys/stat.h>

void handler(int signumber)
{
    printf("\nSignal érkezett %i számmal ==> ", signumber);
}

struct msg
{
    long mtype;
    char mmsg[300];
};

int send(int msgqueue)
{
    const struct msg msg = {5, "Milyen technológiával szeretné a feladatát megvalósítani?"};
    int status;

    status = msgsnd(msgqueue, &msg, strlen(msg.mmsg) + 1, 0);
    if (status < 0)
        perror("msgsnd");
    return 0;
}

int receive(int msgqueue)
{
    struct msg msg;
    int status;
    status = msgrcv(msgqueue, &msg, 1024, 5, 0);

    if (status < 0)
        perror("msgsnd");
    else
        printf("\nHallgató által fogadott üzenet üzenetsoron keresztül:%s\n", msg.mmsg);
    return 0;
}
int main(int argc, char **argv)
{
    int status;
    signal(SIGTERM, handler);
    int pipefd[2];
    int pipe2fd[2];
    char *dontes;
    int msgqueue;
    key_t kulcs;

    if (pipe(pipefd) == -1)
    {
        perror("Hiba a pipe nyitaskor!");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipe2fd) == -1)
    {
        perror("Hiba a pipe nyitaskor!");
        exit(EXIT_FAILURE);
    }

    kulcs = ftok(argv[0], 1);
    msgqueue = msgget(kulcs, 0600 | IPC_CREAT);
    if (msgqueue < 0)
    {
        perror("msgget");
        return 1;
    }

    pid_t hallgato = fork();
    if (hallgato < 0)
    {
        perror("The fork calling was not succesful\n");
        exit(1);
    }
    int oszt_mem_id;
    oszt_mem_id = shmget(kulcs, 500, IPC_CREAT | S_IRUSR | S_IWUSR);
    dontes = shmat(oszt_mem_id, NULL, 0); // osztott memória
    if (hallgato > 0)
    {
        pid_t temavezeto = fork();
        if (temavezeto < 0)
        {
            perror("The fork calling was not succesful\n");
            exit(1);
        }

        if (temavezeto > 0) // parent : NEPTUN
        {
            pause();
            printf("Hallgató sikeresen bejelentkezett\n");
            pause();
            printf("Témavezető sikeresen bejelentkezett\n");

            char string[300];
            close(pipefd[1]);
            read(pipefd[0], &string, 300);
            close(pipefd[0]);

            close(pipe2fd[0]);
            write(pipe2fd[1], &string, strlen(string) + 1);
            close(pipe2fd[1]);

            waitpid(hallgato, &status, 0);
            waitpid(temavezeto, &status, 0);
        }
        else // temavezeto process
        {
            printf("\nTémavezető vár 3mp-et majd küld egy %i signalt\n", SIGTERM);
            sleep(3);
            kill(getppid(), SIGTERM);

            char string[300];

            close(pipe2fd[1]);
            read(pipe2fd[0], &string, 300);
            close(pipe2fd[0]);

            printf("\nTémavezető megkapta csövön az üzenetet, ami a következő:%s\n", string);

            sleep(3);

            send(msgqueue);
            wait(NULL);

            srand(time(NULL));
            int r = rand() % 100 + 1;
            if (r <= 20)
                strcpy(dontes, "A témát elutasítom, nem vállalom...\n");
            else
                strcpy(dontes, "A témát vállalom, rendben van...\n");
            shmdt(dontes);
        }
    }
    else // hallgato process
    {
        printf("\nHallgató vár 3mp-et majd küld egy %i signalt\n", SIGTERM);
        sleep(3);
        kill(getppid(), SIGTERM);
        char bejelento[] = "Izgalmas webes program, Zana Péter, 2025, Nagy Emese";
        close(pipefd[0]);
        write(pipefd[1], &bejelento, strlen(bejelento) + 1);
        close(pipefd[1]);
        receive(msgqueue);

        msgctl( msgqueue, IPC_RMID, NULL ); 

        printf("Az osztott memóriából olvasott döntés: %s", dontes);
        shmdt(dontes);
        shmctl(oszt_mem_id,IPC_RMID,NULL);
    }
    return 0;
}