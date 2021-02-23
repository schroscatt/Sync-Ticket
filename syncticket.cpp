/*
*
* This program provides a sync-ticket system.
*
* How to compile: g++ -o simulation.o 2017400081.cpp -lpthread
* How to run: ./simulation.o config_path output_path
*
* Sevde Sarıkaya
* CMPE322
*/

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <iostream>
#include <vector>
#include<bits/stdc++.h>

using namespace std;


string clientsInfo[3][4];  // Keeps clients' information that tellers handle currently.

pthread_mutex_t mutx; // Mutex for clients
pthread_mutex_t mTeller; // Mutex for threads


//Conditional variables
pthread_cond_t tellerParam[3];
pthread_cond_t start;

int capacity; // Client capacity
int lowest;  // Lowest empty seat
bool available[3]; // Keeps the current availability of tellers in the order
vector<bool> seat; // Keeps the current availability of seats
int clients; // Remaining client number
int tellerThreads, clientThreads;

ofstream outFile;

/* Function definitions that will be used by threads*/
void *teller(void *param);
void *client(void *param);

int main(int argc, char *argv[]){

    /* input & output files */
    ifstream conf(argv[1]);
    outFile.open(argv[2]);

    string name;
    getline(conf, name);


    int num;
    string clientInfo;

    // Arrange capacity according to the theater name
    if(name == "OdaTiyatrosu") {
        capacity = 60;
    }else if(name == "UskudarStudyoSahne") {
        capacity = 80;
    }else {
        capacity = 200;
    }

    lowest = 1;

    // Initialize seats as available
    seat.resize(capacity+1);
    for(int i=0; i<=capacity; i++) {
        seat.at(i) = false;
    }

    // Read the number of clients
    getline(conf, clientInfo);
    num = stoi(clientInfo);

    // Initialize mutex & conditional variables
    pthread_mutex_init(&mutx, NULL);
    pthread_cond_init(&start, NULL);
    pthread_mutex_init(&mTeller, NULL);

    for(int i = 0; i<3; i++) {
        pthread_cond_init(&tellerParam[i], NULL);
        available[i] = true;
    }


    tellerThreads = 3;
    pthread_t pid[tellerThreads];

    clients = num;
    clientThreads = num;
    pthread_t cid[clientThreads];


    outFile << "Welcome to the Sync-Ticket!" << endl;

    /**
     * Create teller threads
     * Send the id of the teller
     * (0 means Teller A, 1 means Teller B, 2 means Teller C)
    */
    for(int i =0; i < 3; i++) {
        sleep(0.3);
        int *index = static_cast<int *>(malloc(sizeof(int)));
        *index = i;
        pthread_create(&pid[i], NULL, teller,(void *)index);

    }

    /**
     * Create client threads
     * Send the client info
    */
    for(int j = 0; j < clientThreads; j++){
        getline(conf,  clientInfo);
        string *info = new string(clientInfo);
        pthread_create(&cid[j], NULL, client, (void *)info);
    }

    sleep(0.5);
    // Send start signal to client threads for synchronization.
    pthread_cond_broadcast(&start);

    // If remaining number of clients equals to 0, it means all clients received service. Exit.
    while(clients > 0) {
    }
    outFile << "All clients received service." << endl;

    outFile.close();

    return 0;
}

// Finds lowest available seat number
void findLowest(){
    while(lowest <= capacity && seat.at(lowest)) {
        lowest+=1;
    }
}


void *teller(void *param){
    // Get id & the name of the Teller
    int index = *static_cast<int*>(param);
    char name = (char) (index + 65);
    outFile << "Teller "<< name << " has arrived." << endl;


    while(clients > 0) {

        /**
         * Wait for clients.
         * After receiving the signal, lock mutex object to achive synchronization.
         */
        pthread_cond_wait(&tellerParam[index], &mTeller);


        int seatNum = stoi(clientsInfo[index][3]); // Get seat number
        string output = "";

        /**
         * Decide the seat number of the clients according to available seat numbers and the wanted seat number.
         */

        if(seatNum>0 && seatNum <= capacity && !seat.at(seatNum)) { //If seat is not taken, client takes what he/she wants.
            seat.at(seatNum) = true;
            output = clientsInfo[index][0] + " requests seat " + to_string(seatNum) + ", reserves seat " + to_string(seatNum) + ". Signed by Teller " + name + ".";
        }
        else if(seatNum>0 && seatNum <= capacity && seat.at(seatNum)) { //If it is taken, client gets the lowest available seat.
            findLowest();
            if(lowest <= capacity) {
                seat.at(lowest) = true;
                output = clientsInfo[index][0] + " requests seat " + to_string(seatNum) + ", reserves seat " + to_string(lowest) + ". Signed by Teller " + name + ".";
            }
            else { // If there is no available seat, he/she does not get ticket.
                output = clientsInfo[index][0] + " requests seat " + to_string(seatNum) + ", reserves None. Signed by Teller " + name + ".";

            }
        }
        else if(seatNum > capacity) { // If client wants the seat number which is bigger than the capacity, he/she gets the lowest available seat number.
            findLowest();
            seat.at(lowest) = true;
            output = clientsInfo[index][0] + " requests seat " + to_string(seatNum) + ", reserves seat " + to_string(lowest) + ". Signed by Teller " + name + ".";
        }
        else {
            output = clientsInfo[index][0] + " requests seat " + to_string(seatNum) + ", reserves None. Signed by Teller " + name + ".";
        }
        // Unlock mutex
        pthread_mutex_unlock(&mTeller);

        usleep(stoi(clientsInfo[index][2])*1000); // Wait until the process finishes


        /**
         * Mutex lock to achive synchronization for global variables.
         * Print the ticket and decrease the remaining client number.
         * Set the teller as available.
        */
        pthread_mutex_lock(&mTeller);
        outFile << output << endl;
        clients -= 1;
        available[index] = true;
        pthread_mutex_unlock(&mTeller);
    }

    free(param);
    pthread_exit(NULL);

}

void *client(void * param){
    // Wait for start signal
    pthread_cond_wait(&start, &mutx);
    pthread_mutex_unlock(&mutx);

    // Get the client information
    string info = *static_cast<std::string *>(param);

    // Parse the given string in the file, get necessary information
    string params[4];
    string a; int ind = 0;
    for (int i = 0; i < info.length(); i++) {
        if (info[i] == ',') {
            params[ind] = a;
            ind += 1;
            a = "";
        } else {
            a += info[i];
        }
    }
    params[ind] = a;

    //Sleep until the arrival time
    usleep(stoi(params[1])*1000);

    // Mutex lock until finding an available Teller.
    pthread_mutex_lock(&mutx);
    bool find = false;
    int count = -1;

    while (!find) { // Search teller in the order
        count = (count+1)%3;
        find = available[count];
    }

    pthread_mutex_lock(&mTeller);
    available[count] = false; // Set the teller as unavailable. (We used lock because it is a global variable)
    pthread_mutex_unlock(&mTeller);

    // Send the clients' info to the teller by putting it into clientsInfo array.
    for(int i = 0; i < 4; i++) {
        clientsInfo[count][i] = params[i];
    }
    pthread_mutex_unlock(&mutx);

    // Send the signal to the teller to start its process.
    pthread_cond_signal(&tellerParam[count]);

    pthread_exit(NULL);
}