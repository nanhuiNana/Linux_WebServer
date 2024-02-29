#ifndef __LOCKER_H__
#define __LOCKER_H__

#include<semaphore.h>
#include<stdio.h>
#include<errno.h>

/*·â×°ÐÅºÅÁ¿*/
class Sem{
private:
    sem_t mySem;
public:
    Sem(){
        if(sem_init(&mySem,0,1)==-1){
            perror("Sem error");
        }
    }
    Sem(int num){
        if(sem_init(&mySem,0,num)==-1){
            perror("Sem_int error");
        }
    }
    ~Sem(){
        sem_destroy(&mySem);
    }
    bool wait(){
        return sem_wait(&mySem);
    }
    bool post(){
        return sem_post(&mySem);
    }
};

#endif