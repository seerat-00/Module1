#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <chrono>
#include <random>
#include <algorithm>  

using namespace std;

const int NUM_PRODUCERS = 2; //thread number
const int NUM_CONSUMERS = 2; //thread number
const int QUEUE_SIZE = 10; //queue size
const int TOP_N = 3; //top congested traffic 
const int NUM_SIGNALS = 5; //unique traffic signal ID
const int MEASUREMENTS_PER_HOUR = 12; //number of data points in an hour

queue<pair<string, pair<string, int>>> traffic_queue; //shared queue where traffic is added and processed
mutex queue_mutex, data_mutex; //mutex to protect access data and traffic_data map 
condition_variable queue_cv; //synchronize producer and consumer
unordered_map<string, int> traffic_data; //unordered map for number of cars passing through each traffic light

random_device rd; //random number
mt19937 gen(rd()); //
uniform_int_distribution<> signal_dist(1, NUM_SIGNALS); //generate traffic signal id
uniform_int_distribution<> car_dist(1, 50); //random number of cars

bool done = false;  //producers finished generating data

//producer thread
void producer(int producer_id) { 
    for (int i = 0; i < MEASUREMENTS_PER_HOUR; ++i) {
        time_t now = time(0); //current time 
        string timestamp = ctime(&now); //converts the time to readable stamp
        string signal_id = "TL_" + to_string(signal_dist(gen)); //random id generated 
        int cars_passed = car_dist(gen); //random no cars passed

        unique_lock<mutex> lock(queue_mutex); //locks the queue mutex to ensure access
        queue_cv.wait(lock, []{ return traffic_queue.size() < QUEUE_SIZE; }); //producer wait id the queue is full

        traffic_queue.push({timestamp, {signal_id, cars_passed}}); //add the generated data
        cout << "[Producer " << producer_id << "] Generated -> " 
             << timestamp << " " << signal_id << " " << cars_passed << " cars" << endl; //print the message

        queue_cv.notify_all(); //notify all the threads, queue has been updated
        this_thread::sleep_for(chrono::milliseconds(5000));  //sleeps for 100 ms for delay in producing data
    }

    unique_lock<mutex> lock(queue_mutex); //locks the queue mutex to ensure access
    done = true;  // producers are done
    queue_cv.notify_all(); //notifies all threads
}

//consumer thread
void consumer(int consumer_id) {
    while (true) {
        unique_lock<mutex> lock(queue_mutex);
        queue_cv.wait(lock, []{ return !traffic_queue.empty() || done; }); //wait until the queue has data or done is set 

        if (traffic_queue.empty() && done) {
            break;  
        }
         //break the loop and stop execution if producers are done

        auto data = traffic_queue.front(); //retrive data from the queue 
        traffic_queue.pop(); //remove the data
        queue_cv.notify_all(); //notifies the thread
        lock.unlock(); //unlock after accessing

        string timestamp = data.first; 
        string signal_id = data.second.first;
        int cars_passed = data.second.second;
        //data is unpacket
        
        lock_guard<mutex> data_lock(data_mutex); //update the traffic_data map
        traffic_data[signal_id] += cars_passed; //add the number of cars passing

        cout << "[Consumer " << consumer_id << "] Processed <- " 
             << timestamp << " " << signal_id << " " << cars_passed << " cars" << endl; //print

        this_thread::sleep_for(chrono::milliseconds(5000));  //wait for 150 ms before processing again
    }
}

//report congested lights
void report_top_n() {
    vector<pair<string, int>> top_signals(traffic_data.begin(), traffic_data.end()); //vector created to map to easily sort

    sort(top_signals.begin(), top_signals.end(), [](auto& a, auto& b) { return a.second > b.second; }); //sorting in descending order based on the number of cars

    cout << "\n *Top Congested Traffic Lights:*" << endl;
    for (int i = 0; i < min(TOP_N, (int)top_signals.size()); ++i) {
        cout << "   " << top_signals[i].first << ": " << top_signals[i].second << " cars passed" << endl; //prints the data
    }
}

int main() {
    vector<thread> producers, consumers; //initlaize to hold the threads

    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        producers.push_back(thread(producer, i)); //create thread
    }
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.push_back(thread(consumer, i)); //create thread
    }

    for (auto& p : producers) {
        p.join(); //wait for threads to finsig before processing
    }

    queue_cv.notify_all(); 
    for (auto& c : consumers) {
        c.join(); //same as producers 
    }

    report_top_n(); //print the top_n traffic lights
    return 0;
}
