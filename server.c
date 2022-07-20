/* 
    server.c
    Ghazi Shazan Ahmad
    IMT2019033
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <unistd.h>   // sleep() is declared here
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <semaphore.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

#define MSG_VAL_LEN  16
// For the client queue message
#define CLIENT_Q_NAME_LEN 16

// For the server queue message
#define MSG_TYPE_LEN 16

typedef struct{
    char client_q[CLIENT_Q_NAME_LEN];
    char msg_val[MSG_VAL_LEN];
} client_msg_t;

typedef struct{
    char msg_type[MSG_TYPE_LEN];
    char msg_val[MSG_VAL_LEN];
} server_msg_t;


static client_msg_t client_msg;

#define SERVER_QUEUE_NAME   "/server_msgq"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE sizeof(client_msg_t)
#define MSG_BUFFER_SIZE (MAX_MSG_SIZE * MAX_MESSAGES) 

#define MIN_COURSES 10
#define MAX_COURSES 15
#define MIN_TEACHERS 5
#define MAX_TEACHERS 10
#define MAX_LEN 10

#define COURSE_NOT_FOUND -1
#define TEACHER_NOT_FOUND -2

char courses[MAX_COURSES][MAX_LEN];
char teachers[MAX_TEACHERS][MAX_LEN];

int nc, nt;

char* substr(const char *src, int m, int n);
void add_course(char *ret);
void add_teacher(char *ret);
int del_course(char *ret);
int del_teacher(char *ret);

void *summary(void *pThreadName);

sem_t bin_sem;

int main (int argc, char **argv)
{
    mqd_t qd_srv, qd_client;   // Server and Client Msg queue descriptors
    int num = 1;

    printf ("Welcome to Edu-Server!!!\n");

    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    if ((qd_srv = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS,
                           &attr)) == -1) {
        perror ("Server MsgQ: mq_open (qd_srv)");
        exit (1);
    }
    int res_sem, res;
    pthread_t thread;
    res_sem = sem_init(&bin_sem, 0, 1);
    if(res_sem != 0) {
        printf("Semaphore1 creation failure: %d\n", res_sem);
        exit(1);
    }

    if( (res = pthread_create( &thread, NULL, &summary, "Summary")) )  {
        printf("Thread creation failed: %d\n", res);
        exit(1);
    }

    client_msg_t in_msg;
	int val_client;
    while (1) {
        // ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio);
        if (mq_receive(qd_srv,(char *) &in_msg, MAX_MSG_SIZE, NULL) == -1) {
            perror ("Server msgq: mq_receive");
            exit (1);
        }
        // printf("%s\n", str);

        char str[MSG_VAL_LEN];
        strcpy(str, in_msg.client_q);
        char *ret;
		ret = strstr(str, "ADD_COURSE");
        if(ret) {
            strcpy(ret, in_msg.msg_val);
            add_course(ret);
        }
        ret = strstr(str, "ADD_TEACHER");
        if(ret) {
            strcpy(ret, in_msg.msg_val);
            add_teacher(ret);
        }
        ret = strstr(str, "DEL_COURSE");
        if(ret) {
            strcpy(ret, in_msg.msg_val);
            del_course(ret);
        }
        ret = strstr(str, "DEL_TEACHER");
        if(ret) {
            strcpy(ret, in_msg.msg_val);
            del_teacher(ret);
        }

        printf ("%d: Server MsgQ: message received.\n", num);
        printf("Client msg q name = %s\n", in_msg.client_q);
        printf("Client msg val = %s\n", str);
        num++;

		server_msg_t out_msg; 
		strcpy(out_msg.msg_type, "Server msg");   // strcpy(destPtr, srcPtr)
		sprintf (out_msg.msg_val, "%d", val_client+10);

        FILE *fp;
        fp = fopen("report", "w");
        if(fp == NULL) {
            printf("Error!");   
            exit(1);
        }
        fprintf(fp, "%s", "...........Summary Report........\n");
        fprintf(fp, "%s", "\n");
        fprintf(fp, "No of courses: %d\n", nc);
        for(int i = 0; i < nc; i++) {
            fprintf(fp, "%d: %s\n", i + 1, courses[i]);
        }
        fprintf(fp, "%s", "\n");
        fprintf(fp, "No of teachers %d\n", nt);
        for(int i = 0; i < nt; i++) {
            fprintf(fp, "%d: %s\n", i + 1, teachers[i]);
        }
        fprintf(fp, "%s", "\n");
        fprintf(fp, "%s", "Allocation of teachers and courses:\n");
        for(int i = 0; i < MIN(nc, nt); i++) {
            fprintf(fp, "%d: %s %s\n", i + 1, courses[i], teachers[i]);
        }
        fprintf(fp, "%s", "\n");

        fclose(fp);
            
    } // end of while(1) 
    
}  // end of main()





void *summary(void *pThreadName)
{
    // sleep(100);
    while(1) {  
        sleep(10);
        sem_wait(&bin_sem); 
        printf("...........Summary........\n");
        printf("No of courses: %d\n", nc);
        for(int i = 0; i < nc; i++) {
            printf("%d: %s\n", i + 1, courses[i]);
        }
        printf("\n");
        printf("No of teachers %d\n", nt);
        for(int i = 0; i < nt; i++) {
            printf("%d: %s\n", i + 1, teachers[i]);
        }
        printf("\n");
        printf("Allocation of teachers and courses:\n");
        for(int i = 0; i < MIN(nc, nt); i++) {
            printf("%d: %s %s\n", i + 1, courses[i], teachers[i]);
        }
        printf(".............................................................\n");
        sem_post(&bin_sem);
    }
    pthread_exit("Summary is exiting");
} // end of thread_function()


char* substr(const char *src, int m, int n)
{
    // get the length of the destination string
    int len = n - m;
 
    // allocate (len + 1) chars for destination (+1 for extra null character)
    char *dest = (char*)malloc(sizeof(char) * (len + 1));
 
    // extracts characters between m'th and n'th index from source string
    // and copy them into the destination string
    for (int i = m; i < n && (*(src + i) != '\0'); i++)
    {
        *dest = *(src + i);
        dest++;
    }
 
    // null-terminate the destination string
    *dest = '\0';
 
    // return the destination string
    return dest - len;
}


void add_course(char *ret) {
    for(int i = 0; i < nc; i++) {
        if(!strcmp(courses[i], ret)) {
            return;
        }
    }
    strcpy(courses[nc], ret);
    nc++;
}

void add_teacher(char *ret) {
    for(int i = 0; i < nt; i++) {
        if(!strcmp(teachers[i], ret)) {
            return;
        }
    }
    strcpy(teachers[nt], ret);
    nt++;
}

int del_course(char *ret) {
    for(int i = 0; i < nc; i++) {
        if(!strcmp(courses[i], ret)) {
            while(i < nc - 1) {
                strcpy(courses[i], courses[i + 1]);
                i++;
            }
            nc -= 1;
            return 1;
        }
    }
    return COURSE_NOT_FOUND;
}

int del_teacher(char *ret) {
    for(int i = 0; i < nt; i++) {
        if(!strcmp(teachers[i], ret)) {
            while(i < nt - 1) {
                strcpy(teachers[i], teachers[i + 1]);
                i++;
            }
            nt--;
            return 1;
        }
    }
    return TEACHER_NOT_FOUND;
}