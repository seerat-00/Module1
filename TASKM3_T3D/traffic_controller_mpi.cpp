#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>

using namespace std;

struct TrafficData {
    string timestamp;
    string light_id;
    int cars_passed;
};

string extractHour(const string& timestamp) {
    return timestamp.substr(0, 13);
}

bool compareByCars(const pair<string, int>& a, const pair<string, int>& b) {
    return a.second > b.second;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    const int TOP_N = 3;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    map<string, vector<TrafficData>> hourlyData;

    if (rank == 0) {
        ifstream file("traffic_input.csv");
        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;

            stringstream ss(line);
            string timestamp, light_id, cars_str;

            getline(ss, timestamp, ',');
            getline(ss, light_id, ',');
            getline(ss, cars_str);

            timestamp.erase(timestamp.find_last_not_of(" \n\r\t") + 1);
            light_id.erase(0, light_id.find_first_not_of(" \n\r\t"));
            light_id.erase(light_id.find_last_not_of(" \n\r\t") + 1);
            int cars_passed = stoi(cars_str);

            string hour_key = extractHour(timestamp);
            hourlyData[hour_key].push_back({timestamp, light_id, cars_passed});
        }

        for (const auto& [hour, vec] : hourlyData) {
            int target = ((hash<string>{}(hour)) % (size - 1)) + 1;

            int hour_len = hour.length();
            int count = vec.size();
            MPI_Send(&hour_len, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
            MPI_Send(hour.c_str(), hour_len, MPI_CHAR, target, 0, MPI_COMM_WORLD);
            MPI_Send(&count, 1, MPI_INT, target, 0, MPI_COMM_WORLD);

            for (const auto& d : vec) {
                int ts_len = d.timestamp.length();
                int id_len = d.light_id.length();
                MPI_Send(&ts_len, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
                MPI_Send(d.timestamp.c_str(), ts_len, MPI_CHAR, target, 0, MPI_COMM_WORLD);
                MPI_Send(&id_len, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
                MPI_Send(d.light_id.c_str(), id_len, MPI_CHAR, target, 0, MPI_COMM_WORLD);
                MPI_Send(&d.cars_passed, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
            }
        }

        for (int i = 1; i < size; ++i) {
            int stop_signal = -1;
            MPI_Send(&stop_signal, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        cout << "\n🚦 Traffic Congestion Report (Top " << TOP_N << " per hour):\n";
        for (int i = 1; i < size; ++i) {
            while (true) {
                int has_data;
                MPI_Recv(&has_data, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (has_data == 0) break;

                int hour_len, result_count;
                MPI_Recv(&hour_len, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                char* hour_buf = new char[hour_len + 1];
                MPI_Recv(hour_buf, hour_len, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                hour_buf[hour_len] = '\0';
                string hour(hour_buf);
                delete[] hour_buf;

                MPI_Recv(&result_count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                cout << "[Hour " << hour << "] Top " << result_count << " Most Congested Traffic Lights:\n";
                for (int j = 0; j < result_count; ++j) {
                    int id_len, cars;
                    MPI_Recv(&id_len, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    char* id_buf = new char[id_len + 1];
                    MPI_Recv(id_buf, id_len, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    id_buf[id_len] = '\0';
                    string light_id(id_buf);
                    delete[] id_buf;

                    MPI_Recv(&cars, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    cout << "   " << light_id << ": " << cars << " cars\n";
                }
            }
        }

    } else {
        while (true) {
            int hour_len;
            MPI_Recv(&hour_len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (hour_len == -1) break;

            char* hour_buf = new char[hour_len + 1];
            MPI_Recv(hour_buf, hour_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            hour_buf[hour_len] = '\0';
            string hour(hour_buf);
            delete[] hour_buf;

            int count;
            MPI_Recv(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            unordered_map<string, int> lightCounts;

            for (int i = 0; i < count; ++i) {
                int ts_len, id_len;
                MPI_Recv(&ts_len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                char* ts_buf = new char[ts_len + 1];
                MPI_Recv(ts_buf, ts_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                ts_buf[ts_len] = '\0';
                string timestamp(ts_buf);
                delete[] ts_buf;

                MPI_Recv(&id_len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                char* id_buf = new char[id_len + 1];
                MPI_Recv(id_buf, id_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                id_buf[id_len] = '\0';
                string light_id(id_buf);
                delete[] id_buf;

                int cars;
                MPI_Recv(&cars, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                lightCounts[light_id] += cars;
            }

            vector<pair<string, int>> sortedLights(lightCounts.begin(), lightCounts.end());
            sort(sortedLights.begin(), sortedLights.end(), compareByCars);
            int send_count = min(TOP_N, (int)sortedLights.size());

            int has_data = 1;
            MPI_Send(&has_data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            int hour_str_len = hour.length();
            MPI_Send(&hour_str_len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(hour.c_str(), hour_str_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&send_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

            for (int i = 0; i < send_count; ++i) {
                int id_len = sortedLights[i].first.length();
                MPI_Send(&id_len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(sortedLights[i].first.c_str(), id_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&sortedLights[i].second, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            }
        }

        int has_data = 0;
        MPI_Send(&has_data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
