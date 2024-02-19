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

using namespace std;

struct pipe_data{
    int id;
    float n;
};
struct program_data{
    int n_neorons;
    int total_layers;
    int current_layer;
};

vector<vector<double>> hidden_layer_weights;
vector<float> input_layer_weights;
vector<float> output_layer_weights;
vector<float> input_data;

void file_read();

pthread_barrier_t thraed_barrier;

sem_t my_sem;

float *input;
vector<vector<float>> new_input;

void *calculate_next_inputs(void *a)
{

    sem_wait(&my_sem);
    int *id = (int *)a;

    for (int i = 0; i < (input_layer_weights.size()); i++)
    {
        new_input[*id][i] = input_data[*id] * input_layer_weights[i];
        // cout << *id <<" "<< i <<" "<<new_input[*id][i] << endl;
    }

    sem_post(&my_sem);

    pthread_barrier_wait(&thraed_barrier);

    return NULL;
}

int main()
{

    file_read();
    sem_init(&my_sem, 0, 1);

    int no_of_neurons = 8;
    int no_of_layers = 5;
    int n_inputs = 2;

    new_input.resize(n_inputs);
    for (auto &i : new_input)
    {
        i.resize(no_of_neurons);
    }
    //cout << "main before data" << endl;
    
    pipe_data pd;

    pthread_barrier_init(&thraed_barrier, NULL, n_inputs + 1);

    input = new float[n_inputs];

    pthread_t ID[n_inputs];

    // a is an array becasue we have to pass variable by reference
    // and when we do that it changes before the respective
    // thread prints or uses it so i made and array that stores
    // every index and then send it to the respective thread
    int a[n_inputs];
    pthread_attr_t attr[n_inputs];

    for (int i = 0; i < n_inputs; i++)
    {
        a[i] = i;
        pthread_create(&ID[i], NULL, calculate_next_inputs, (void *)&a[i]);
    }

    // this barrier will wait for all the thread to do their job
    // before letting the forks run

    pthread_barrier_wait(&thraed_barrier);

    float final_output[no_of_neurons];

    // calculating new values

    for (int i = 0; i < no_of_neurons; i++)
    {
        final_output[i] = 0;
    }

    for (auto &vv : new_input)
    {
        for (int i = 0; i < no_of_neurons; i++)
        {
            final_output[i] += vv[i];
            //cout << final_output[i] << endl;
        }
    }

    int fd;
    const char *fifo_name = "my_pipe";

    mkfifo(fifo_name, 0666);
    pid_t id;

    id = fork();
    if(id == -1)
        cout << "failed";
    if (id == 0)
    {
        //cout << "bb" << endl;
        char *args[] = {"./EXEC", "my_pipe"};
        execv(args[0], args);
        exit(0);
    }
    else
    {
        program_data prog_d{no_of_neurons, no_of_layers, no_of_layers};
        //cout << "cc" << endl;
        fd = open(fifo_name, O_WRONLY);
        write(fd, &prog_d, sizeof(prog_d));

        for (int i = 0; i < no_of_neurons; i++)
        {
            pd.id = i;
            pd.n = final_output[i];
            write(fd, &pd, sizeof(pd));
        }
        

        
    }

    // this while loop will ensure that all the forked processes end before
    // the main loop ends
    wait(NULL);
    

    pthread_barrier_destroy(&thraed_barrier);
    
    fd = open(fifo_name, O_RDONLY);
    float *f;
    if (read(fd, &f, sizeof(f)) == -1)
    {
        perror("2write");
    }


    cout << "\nx1:\t" << f[0] << endl;
    cout << "\nx2:\t" << f[1] << endl;
    unlink(fifo_name);
    return 0;
}

void file_read()
{
    fstream infile("test.txt");

    // Read input layer weights
    string line;
    istringstream iss(line);
    double val;

    // Read hidden layer weights
    ;
    while (getline(infile, line))
    {
        if (line.find("Hidden layer") == 0)
        {
            vector<double> layer_weights;
            while (getline(infile, line) && !line.empty())
            {
                istringstream iss(line);
                while (iss >> val)
                {
                    layer_weights.push_back(val);
                    if (iss.peek() == ',')
                    {
                        iss.ignore();
                    }
                }
            }
            hidden_layer_weights.push_back(layer_weights);
        }
    }

    // int layer_num = 1;
    // for (auto layer_weights : hidden_layer_weights) {
    //     cout << "Hidden layer " << layer_num << " weights:" << endl;
    //     for (auto val : layer_weights) {
    //         cout << val << " ";
    //     }
    //     cout << endl;
    //     layer_num++;
    // }

    ifstream myfile("input_weights.txt");
    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            if (line == "Input layer weights")
            {
                getline(myfile, line);
                stringstream ss(line);
                float val;
                while (ss >> val)
                {
                    input_layer_weights.push_back(val);
                    if (ss.peek() == ',')
                        ss.ignore();
                }
            }
            else if (line == "Output layer weights")
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
            else if (line == "Input data")
            {
                getline(myfile, line);
                stringstream ss(line);
                float val;
                while (ss >> val)
                {
                    input_data.push_back(val);
                    if (ss.peek() == ',')
                        ss.ignore();
                }
            }
        }
        myfile.close();
    }
    else
    {
        cout << "Unable to open file";
    }

    // cout << "Input layer weights: ";
    // for (auto i : input_layer_weights) {
    //     cout << i << " ";
    // }
    // cout << endl;

    // cout << "Output layer weights: ";
    // for (auto i : output_layer_weights) {
    //     cout << i << " ";
    // }
    // cout << endl;

    // cout << "Input data: ";
    // for (auto i : input_data) {
    //     cout << i << " ";
    // }
    // cout << endl;
    return;
}