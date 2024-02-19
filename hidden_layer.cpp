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

vector<vector<double>> hidden_layer_weights;
vector<float> input_layer_weights;
vector<float> output_layer_weights;
vector<float> input_data;

void file_read();
void arrange_data();

int no_of_neurons;
int no_of_layers;
int current_layers;

sem_t thread_sem;
pthread_mutex_t thread_count_mutex;

float **hd_data;

vector<vector<float>> new_input;
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
    for (int i = 0; i < no_of_neurons; i++)
    {
        final_output[id] += hd_data[i][id] * pd->n;
    }

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

    mkfifo(pipe_name, 0666);
    int fd = open(pipe_name, O_RDONLY);
    read(fd, &data, sizeof(data));

    no_of_neurons = data.no_of_neurons;
    current_layers = data.current_layers;
    no_of_layers = data.total_layers;
    arrange_data();
    cout << "Running Hidden layer no: ";
    cout << current_layers << endl;
    int cc = current_layers;

    final_output = new float[no_of_neurons];

    // pthread_barrier_init(&thraed_barrier, NULL, no_of_neurons + 1);

    //-------------------------------calculating new values--------------

    new_input.resize(data.no_of_neurons);
    for (auto &i : new_input)
    {
        i.resize(data.no_of_neurons);
    }
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
        read(fd, &pd[i], sizeof(pd[i]));
        // cout << pd[i].id << " " << pd[i].n << endl;
    }
    // cout << "____________________________________________" << endl;
    for (int k = 0; k < no_of_neurons; k++)
    {
        // cout << pd[k].id << " " << pd[k].n << endl;
        pipe_data thread_arg{pd[k].id, pd[k].n};
        pthread_create(&ID[k], NULL, thread_function, (void *)&thread_arg);
    }

    // this barrier will wait for all the thread to do their job
    // before letting the forks run
    // cout << "before wait";

    while (thread_count < no_of_neurons)
    {
        
    }
    // cout << "____________________________________________" << endl;
    pthread_mutex_destroy(&thread_count_mutex);
    sem_destroy(&thread_sem);

    // then loop in each thread waits for input from all the previous
    // neurons

    // for (int i = 0; i < no_of_neurons; i++)
    // {
    //     cout << final_output[i] << endl;
    // }
    // cout << "close0: " << close(fd) << endl;
    //cout << current_layers << endl;

    int gd;
    char *myString = const_cast<char *>(to_string(current_layers + 100).c_str());
    char buffer[256];

    snprintf(buffer, sizeof(buffer), "%d", current_layers + 100);
    mkfifo(buffer, 0666);
    //cout << "a";
    if (current_layers > 1)
    {
        //cout << "b" << endl;
        //cout << "before fork" << endl;
        pid_t f_id = fork();
        if (f_id == -1)
            cout << "failed";
        if (f_id == 0)
        {
            // cout << "bb" << endl;
            char *args[] = {"./EXEC", buffer, NULL};
            if (execv(args[0], args) == -1)
            {
                perror("execv");
            }
            exit(0);
        }
        else
        {
            current_layers--;
            program_data prog_d{no_of_neurons, no_of_layers, current_layers};
            //cout << "cc" << endl;
            gd = open(myString, O_WRONLY);

            if (write(gd, &prog_d, sizeof(prog_d)) == -1)
            {
                perror("1write:");
            }
            else
            {
                for (int i = 0; i < no_of_neurons; i++)
                {
                    pipe_data pd;
                    pd.id = i;
                    pd.n = final_output[i];
                    if (write(gd, &pd, sizeof(pd)) == -1)
                    {
                        perror("2write");
                    }
                }
            }

            close(gd);
        }
    }
    else
    {
        pid_t f_id2 = fork();
        if (f_id2 == -1)
            cout << "failed";
        if (f_id2 == 0)
        {
            //cout << "bb" << endl;
            char *args[] = {"./out", buffer, NULL};
            if (execv(args[0], args) == -1)
            {
                perror("execv");
            }
            exit(0);
        }
        else
        {
            // cout << "cc" << endl;
            gd = open(myString, O_WRONLY);

            if (write(gd, &no_of_neurons, sizeof(no_of_neurons)) == -1)
            {
                perror("1write:");
            }
            else
            {
                for (int i = 0; i < no_of_neurons; i++)
                {
                    pipe_data pd;
                    pd.id = i;
                    pd.n = final_output[i];
                    if (write(gd, &pd, sizeof(pd)) == -1)
                    {
                        perror("2write");
                    }
                }
            }
            close(gd);
        }
        
    }

    gd = open(buffer, O_RDONLY);
    float *f;
    if (read(gd, &f, sizeof(f)) == -1)
    {
        perror("2write");
    }
    //cout << f[0] << endl;
    cout << "\n--------------------------------------\nHidden Layer no: ";
    cout << cc << endl;
    for (int i = 0; i < no_of_neurons; i++)
    {
        cout << i << "\t";
        cout << final_output[i] << endl;
    }

    close(gd);
    unlink(buffer);

    close(fd);

    fd = open(pipe_name, O_WRONLY);
    if (write(fd, &f, sizeof(f)) == -1)
    {
        perror("2write");
    }

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
    return;
}

void arrange_data()
{
    int cl = no_of_layers - current_layers ;
    //cout << "inside ad"<< cl <<endl;
    int ROWS = 8;
    int COLS = 8;

    ROWS = COLS = no_of_neurons;

    hd_data = new float *[ROWS];
    for (int i = 0; i < ROWS; i++)
    {
        hd_data[i] = new float[COLS];
    }

    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            hd_data[i][j] = hidden_layer_weights[cl][i * COLS + j];
        }
    }
}
