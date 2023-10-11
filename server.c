#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "defn.h"

union semun 
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void sem_P(int sem_id, int sem_num)
{
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_op = -1;
    sem_op.sem_flg = SEM_UNDO;
    semop(sem_id, &sem_op, 1);
}

void sem_V(int sem_id, int sem_num)
{
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_op = 1;
    sem_op.sem_flg = SEM_UNDO;
    semop(sem_id, &sem_op, 1);
}

struct product data[MAX_SIZE];
int globalId;
int semid;
FILE* file;

void* handler(void *arg) 
{
    int cd = *(int*)arg;
    int choice;
    printf("Client joined\n");

    while(recv(cd, &choice, sizeof(choice), 0))
    {
        printf("Choice %i selected\n", choice);
        if(choice == 1)
        {
            char name[256];
            float price;
            int quantity;
            recv(cd, name, sizeof(name), 0);
            recv(cd, &price, sizeof(price), 0);
            recv(cd, &quantity, sizeof(quantity), 0);

            int i;
            for(i = 0; i < MAX_SIZE; i++)
            {
                if(data[i].id == -1)
                {
                    sem_P(semid, i);
                    data[i].id = globalId;
                    globalId++;
                    strcpy(data[i].name, name);
                    data[i].price = price;
                    data[i].quantity = quantity;
                    sem_V(semid, i);
                    break;
                }
            }

            char msg[256];
            if(i < MAX_SIZE)
            {
                sem_P(semid, MAX_SIZE);
                strcpy(msg, "Successfully added\n");
                fprintf(file, "ADD\t%s\t%.2f\t%i\n", name, price, quantity);
                fflush(file);
                sem_V(semid, MAX_SIZE);
            }
            else
                strcpy(msg, "No space available\n");
            send(cd, msg, sizeof(msg), 0);



        }
        else if(choice == 2)
        {
            int id;
            recv(cd, &id, sizeof(id), 0);

            int i;
            for(i = 0; i < MAX_SIZE; i++)
            {
                if(data[i].id == id)
                {
                    sem_P(semid, i);
                    data[i].id = -1;
                    sem_V(semid, i);
                    break;
                }
            }

            char msg[256];
            if(i < MAX_SIZE)
            {
                sem_P(semid, MAX_SIZE);
                strcpy(msg, "Successfully deleted\n");
                fprintf(file, "DELETE\t%i\n", id);
                fflush(file);
                sem_V(semid, MAX_SIZE);
            }
            else
                strcpy(msg, "Id not found\n");
            send(cd, msg, sizeof(msg), 0);
        }
        else if(choice == 3)
        {
            int id, quantity;
            float price;
            recv(cd, &id, sizeof(id), 0);
            recv(cd, &price, sizeof(price), 0);
            recv(cd, &quantity, sizeof(quantity), 0);

            int i;
            for(i = 0; i < MAX_SIZE; i++)
            {
                if(data[i].id == id)
                {
                    sem_P(semid, i);
                    data[i].price = price;
                    data[i].quantity = quantity;
                    sem_V(semid, i);
                    break;
                }
            }

            char msg[256];
            if(i < MAX_SIZE)
            {
                sem_P(semid, MAX_SIZE);
                strcpy(msg, "Successfully updated\n");
                fprintf(file, "UPDATE\t%i\t%.2f\t%i\n", id, price, quantity);
                fflush(file);
                sem_V(semid, MAX_SIZE);
            }
            else
                strcpy(msg, "Id not found\n");
            send(cd, msg, sizeof(msg), 0);
        }
        else if(choice == 4)
        {
            char msg[4096] = "";
            char line[256];
            for(int i = 0; i < MAX_SIZE; i++)
            {
                if(data[i].id > -1)
                {
                    sprintf(line, "%i\t%s\t%.2f\t%i\n", data[i].id, data[i].name, data[i].price, data[i].quantity);
                    strcat(msg, line);
                }
            }
            send(cd, msg, sizeof(msg), 0);
        }
        else if(choice == 6)
        {
            int id;
            recv(cd, &id, sizeof(id), 0);
            struct product p;
            p.id = -1;
            for(int i = 0; i < MAX_SIZE; i++)
            {
                if(data[i].id == id)
                {
                    p = data[i];
                    break;
                }
            }
            send(cd, &p, sizeof(p), 0);
        }
        else if(choice == 7)
        {
            int cartCount;
            recv(cd, &cartCount, sizeof(cartCount), 0);
            struct product finalCart[cartCount];
            int ind[cartCount];
            int sem[cartCount];
            recv(cd, finalCart, sizeof(finalCart), 0);

            for(int i = 0; i < cartCount; i++)
            {
                ind[i] = 0;
                sem[i] = 0;
                for(int j = 0; j < MAX_SIZE; j++)
                {
                    if(finalCart[i].id == data[j].id)
                    {
                        sem_P(semid, j);
                        sem[i] = j+1;
                        if(data[j].quantity >= finalCart[i].quantity && data[j].id != -1)
                        {
                            finalCart[i].price = data[j].price;
                            ind[i] = j+1;
                        }
                        break;
                    }
                }
            }

            send(cd, ind, sizeof(ind), 0);
            send(cd, finalCart, sizeof(finalCart), 0);

            char c[256];
            recv(cd, c, sizeof(c), 0);
            for(int i = 0; i < cartCount; i++)
            {
                if(ind[i] && c[0] == 'y')
                    data[ind[i]-1].quantity -= finalCart[i].quantity;
                if(sem[i])
                    sem_V(semid, sem[i]-1);
            }
            break;
        }
        
    }

    printf("Client disconnected\n");
    close(cd);
    return NULL;
}

int main()
{
    for(int i = 0; i < MAX_SIZE; i++)
        data[i].id = -1;
    globalId = 0;
    file = fopen("log.txt", "w");

    semid = semget(SEM_KEY, MAX_SIZE+1, IPC_CREAT | 0666);
    union semun arg;
    arg.val = 1;
    for(int i = 0; i <= MAX_SIZE; i++)
        semctl(semid, i, SETVAL, arg);
    
    
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd < 0)
    {
        perror("Socket error");
        return 1;
    }
    
    int opt = 1;
    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Option error");
        return 1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(8080);
    if(bind(sd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("Bind error");
        return 1;
    }

    if(listen(sd, 3) < 0)
    {
        perror("Listen error");
        return 1;
    }

    printf("Waiting for connections\n");

    while(1)
    {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int cd = accept(sd, (struct sockaddr*)&client, &len);
        if(cd < 0)
        {
            perror("Accept error");
            return 1;
        }

        pthread_t tid;
        if(pthread_create(&tid, NULL, handler, (void*)&cd) < 0)
        {
            perror("Thread error");
            return 1;
        }
        
        pthread_detach(tid);
    }

    return 0;
}


