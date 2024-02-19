#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <mutex>
#include <vector>
#include <fstream>
#include <atomic>
#include <sstream>
#include <semaphore.h>
#include <cmath>

using namespace std;

struct pipe_data
{
    int id;
    float n;
};
struct program_data
{
    int no_of_neurons;
    int total_layers;
    int current_layers;
};

struct final_data
{
    float f1;
    float f2;
};

vector<float> output_layer_weights;

void file_read();
void arrange_data();


int no_of_neurons = 0;

sem_t thread_sem;
pthread_mutex_t thread_count_mutex;

float *final_output;
int thread_count = 0;

void *thread_function(void *a)
{
    pipe_data *pd = (pipe_data *)a;
    int id = pd->id;

    // cout << "before sem: " << id <<": " << pd->n <<endl;
    sem_wait(&thread_sem);
    // cout << "after sem: " << id << endl;

    int b = 0;
    pthread_mutex_lock(&thread_count_mutex);
    thread_count++;
    // cout << "inside mutex: " << id << ": " << thread_count << endl;
    pthread_mutex_unlock(&thread_count_mutex);

    final_output[id] = 0;
    final_output[id] = output_layer_weights[id] * pd->n;
    //cout << "in out thread"<< final_output[id] << endl;

    // cout << "after loop: " << id << endl;
    sem_post(&thread_sem);
    // cout << "before thread wait" << endl;
    // pthread_barrier_wait(&thraed_barrier);
    // cout << "after thread wait" << endl;

    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_mutex_init(&thread_count_mutex, NULL);
    sem_init(&thread_sem, 0, 1);

    program_data data;

    char *pipe_name = argv[1];

    file_read();
    
    int fd = open(pipe_name, O_RDONLY);
    if(read(fd, &no_of_neurons, sizeof(no_of_neurons)) == -1)
    {
        perror("output read");
    }
    //cout << "\nnonono" <<no_of_neurons << endl;
    no_of_neurons = 8;
    int n_inputs = 2;
    //cout << current_layers << endl;

    final_output = new float[no_of_neurons];

    // pthread_barrier_init(&thraed_barrier, NULL, no_of_neurons + 1);

    //-------------------------------calculating new values--------------

    // read (no of inputs to read from a pipe) from a pipe
    // this data is sent from previos layers main
    // cout << no_of_neurons << endl;
    pthread_t ID[no_of_neurons];

    // a is an array becasue we have to pass variable by reference
    // and when we do that it changes before the respective
    // thread prints or uses it so i made and array that stores
    // every index and then send it to the respective thread

    int a[no_of_neurons];
    // cout << " before threading" << endl;

    pipe_data pd[no_of_neurons];

    for (int i = 0; i < no_of_neurons; i++)
    {
        if(read(fd, &pd[i], sizeof(pd[i])) == -1)
        {
            
            perror("output in  read");
        }
        //cout << "values: " << pd[i].n << endl;
       //cout << pd[i].id << " " << pd[i].n << endl;
    }
    //cout << "____________________________________________" << endl;
    for (int k = 0; k < no_of_neurons; k++)
    {
        //cout << pd[k].id << " " << pd[k].n << endl;
        pipe_data thread_arg{pd[k].id, pd[k].n};
        pthread_create(&ID[k], NULL, thread_function, (void *)&thread_arg);
    }

    close(fd);

    // this barrier will wait for all the thread to do their job
    // before letting the forks run
    // cout << "before wait";

    while (thread_count < no_of_neurons)
    {
    }
    //cout << "____________________________________________" << endl;

    pthread_mutex_destroy(&thread_count_mutex);
    sem_destroy(&thread_sem);

    float x = 0;
    for (int i = 0; i < no_of_neurons; i++)
    {
        x += final_output[i];
    }

    int no_of_inputs[n_inputs];
    float f[2];

    for (int i = 0; i < n_inputs; i++)
    {
        f[i] = (sqrt(x) + x + 1)/2;
        
    }

    
    fd = open(pipe_name, O_WRONLY);
    if (write(fd, &f, sizeof(f)) == -1)
    {
        perror("output write");
    }
    close(fd);
    
    return 0;
}

void file_read()
{
    fstream infile("test.txt");

    // Read input layer weights
    string line;
    istringstream iss(line);

    ifstream myfile("input_weights.txt");
    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            if (line == "Output layer weights")
            {
                while (getline(myfile, line))
                {
                    if (line.empty())
                    {
                        break;
                    }
                    output_layer_weights.push_back(stof(line));
                }
            }
        }
        myfile.close();
    }
    else
    {
        cout << "Unable to open file";
    }
    return;
}