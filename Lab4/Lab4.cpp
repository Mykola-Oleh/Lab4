#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <locale>
using namespace std;

class DataStruct {
private:
    static constexpr int M = 3;                
    array<int, M> fields{};               
    array<mutex, M> mtx;            

public:
    DataStruct() noexcept = default;

    int read(int idx) {
        lock_guard<mutex> lock(mtx[idx]);
        return fields[idx];
    }

    void write(int idx, int value) {
        lock_guard<mutex> lock(mtx[idx]);
        fields[idx] = value;
    }

    string to_string() {
        scoped_lock lock(mtx[0], mtx[1], mtx[2]);
        return "[" + std::to_string(fields[0]) + ", "
            + std::to_string(fields[1]) + ", "
            + std::to_string(fields[2]) + "]";
    }
};

void generate_file(const string& filename,
    const vector<double>& probs,
    int total_ops = 1000000)
{
    ofstream out(filename);
    random_device rd;
    mt19937 gen(rd());
    discrete_distribution<> dist(probs.begin(), probs.end());

    for (int i = 0; i < total_ops; ++i) {
        int op = dist(gen);
        if (op == 0) out << "read 0\n";
        else if (op == 1) out << "write 0 1\n";
        else if (op == 2) out << "read 1\n";
        else if (op == 3) out << "write 1 1\n";
        else if (op == 4) out << "read 2\n";
        else if (op == 5) out << "write 2 1\n";
        else out << "string\n";
    }
}

void execute_actions(DataStruct& ds, const string& filename)
{
    ifstream in(filename);
    string cmd;
    int idx, val;

    while (in >> cmd) {
        if (cmd == "read") {
            in >> idx;
            ds.read(idx);
        }
        else if (cmd == "write") {
            in >> idx >> val;
            ds.write(idx, val);
        }
        else if (cmd == "string") {
            ds.to_string();
        }
    }
}

int main()
{
    setlocale(LC_ALL, "ukr");
    using namespace chrono;
    DataStruct ds;

    vector<pair<string, vector<double>>> scenarios = {
        {"a_condition", { 10,5,10,5,10,20,40 }},
        {"b_equal", { 1,1,1,1,1,1,1 }},        
        {"c_mismatch", { 40,1,2,30,1,5,21 }} 
    };

    cout << "Генерація файлів...\n";
    for (const auto& scenario : scenarios) {
        for (int i = 1; i <= 3; ++i) {
            generate_file(scenario.first + "_" + to_string(i) + ".txt",
                scenario.second);
        }
    }
    cout << "Генерація завершена.\n";

    cout << "===== Результати вимірювання часу (варіант 4) =====\n";
    for (const auto& scenario : scenarios) {
        cout << "\n--- Сценарій: " << scenario.first << " ---\n";

        for (int threads = 1; threads <= 3; ++threads) {

            vector<string> files_to_use;
            for (int i = 1; i <= threads; ++i) {
                files_to_use.push_back(scenario.first + "_" + to_string(i) + ".txt");
            }

            auto start = high_resolution_clock::now();

            std::vector<std::thread> pool;
            for (int i = 0; i < threads; ++i) {
                pool.emplace_back(execute_actions, ref(ds), files_to_use[i]);
            }
            for (auto& t : pool) t.join();

            auto end = high_resolution_clock::now();
            double ms = duration<double, milli>(end - start).count();

            cout << "Режим: " << threads << " потік(и)"
                << " | Загальний час виконання (max): " << ms << " мс\n";
        }
    }

    cout << "\nПрограма завершена.\n";
    return 0;
}