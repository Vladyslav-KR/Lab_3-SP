#include <iostream>
#include <vector>
#include <random>
#include <iomanip>
#include <algorithm>
#include <queue>
#include <climits>
#include <string>


// Підсумок результатів для алгоритмів

struct ResultSummary {
    double avgWaiting = 0.0;
    double avgTurnaround = 0.0;
    std::string name;
};

// Опис процесу

struct Process {
    int id;             // ID процесу
    int arrivalTime;    // час приходу процесу
    int burstTime;      // необхідний час процесора (тривалість виконання)
    int priority;        // поточний пріоритет (менше значення = вищий пріоритет)
    int initialPriority; // початковий пріоритет (для аналізу/статистики)

    // Поля для результатів симуляції
    int remainingTime;   // залишковий час виконання (для Round Robin / преемптивних алгоритмів)
    int startTime;       // час початку виконання
    int finishTime;      // час завершення
    int waitingTime;     // час очікування
    int turnaroundTime;  // час обороту (finish - arrival)
};


// Генерація випадкових процесів

std::vector<Process> generateProcesses(int count) {
    std::vector<Process> processes;
    processes.reserve(count);

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<int> arrivalDist(0, 10);  // час прибуття [0..10]
    std::uniform_int_distribution<int> burstDist(1, 10);    // тривалість виконання [1..10]
    std::uniform_int_distribution<int> prioDist(1, 5);      // пріоритет [1..5]

    for (int i = 0; i < count; ++i) {
        Process p;
        p.id = i + 1;
        p.arrivalTime = arrivalDist(gen);
        p.burstTime = burstDist(gen);
        p.priority = prioDist(gen);
        p.initialPriority = p.priority;

        p.remainingTime = p.burstTime;
        p.startTime = -1;
        p.finishTime = -1;
        p.waitingTime = 0;
        p.turnaroundTime = 0;

        processes.push_back(p);
    }

    return processes;
}


// Виведення згенерованих процесів

void printProcesses(const std::vector<Process>& processes) {
    std::cout << "Generated processes:\n";
    std::cout << std::left
        << std::setw(5) << "ID"
        << std::setw(10) << "Arrival"
        << std::setw(10) << "Burst"
        << std::setw(10) << "Prio"
        << "\n";

    for (const auto& p : processes) {
        std::cout << std::left
            << std::setw(5) << p.id
            << std::setw(10) << p.arrivalTime
            << std::setw(10) << p.burstTime
            << std::setw(10) << p.priority
            << "\n";
    }
    std::cout << "----------------------------------------\n";
}


// Реалізація FCFS

ResultSummary simulateFCFS(std::vector<Process> processes) {
    int n = static_cast<int>(processes.size());
    if (n == 0) {
        std::cout << "\n=== FCFS Scheduling ===\nNo processes.\n";
        return { 0.0, 0.0, "FCFS" };
    }

    // сортуємо за часом прибуття (якщо однаковий — за ID)
    std::sort(processes.begin(), processes.end(),
        [](const Process& a, const Process& b) {
            if (a.arrivalTime == b.arrivalTime)
                return a.id < b.id;
            return a.arrivalTime < b.arrivalTime;
        });

    int currentTime = 0;
    double totalWaiting = 0.0;
    double totalTurnaround = 0.0;

    std::cout << "\n=== FCFS Scheduling ===\n";
    std::cout << std::left
        << std::setw(5) << "ID"
        << std::setw(10) << "Arrive"
        << std::setw(10) << "Burst"
        << std::setw(10) << "Start"
        << std::setw(10) << "Finish"
        << std::setw(12) << "Waiting"
        << std::setw(12) << "Turnaround"
        << "\n";

    for (auto& p : processes) {
        if (currentTime < p.arrivalTime)
            currentTime = p.arrivalTime;  // процесор простоює, поки процес не прийде

        p.startTime = currentTime;
        p.finishTime = currentTime + p.burstTime;
        p.waitingTime = p.startTime - p.arrivalTime;
        p.turnaroundTime = p.finishTime - p.arrivalTime;

        currentTime = p.finishTime;

        totalWaiting += p.waitingTime;
        totalTurnaround += p.turnaroundTime;

        std::cout << std::left
            << std::setw(5) << p.id
            << std::setw(10) << p.arrivalTime
            << std::setw(10) << p.burstTime
            << std::setw(10) << p.startTime
            << std::setw(10) << p.finishTime
            << std::setw(12) << p.waitingTime
            << std::setw(12) << p.turnaroundTime
            << "\n";
    }

    std::cout << "-----------------------------------------------\n";
    double avgW = totalWaiting / n;
    double avgT = totalTurnaround / n;
    std::cout << "Average waiting time:    " << avgW << "\n";
    std::cout << "Average turnaround time: " << avgT << "\n\n";

    return { avgW, avgT, "FCFS" };
}


// Реалізація алгоритму Round Robin

ResultSummary simulateRoundRobin(std::vector<Process> processes, int quantum) {
    int n = static_cast<int>(processes.size());
    if (n == 0) {
        std::cout << "\n=== Round Robin Scheduling ===\nNo processes.\n";
        return { 0.0, 0.0, "Round Robin" };
    }

    if (quantum <= 0) {
        std::cout << "Invalid quantum.\n";
        return { 0.0, 0.0, "Round Robin" };
    }

    // Скидаємо поля, пов’язані з симуляцією
    for (auto& p : processes) {
        p.remainingTime = p.burstTime;
        p.startTime = -1;
        p.finishTime = -1;
        p.waitingTime = 0;
        p.turnaroundTime = 0;
    }

    std::cout << "\n=== Round Robin Scheduling ===\n";
    std::cout << "Time quantum = " << quantum << "\n";

    int currentTime = 0;
    int completed = 0;

    std::queue<int> readyQueue;            // індекси процесів у черзі
    std::vector<bool> inQueue(n, false);   // чи знаходиться процес у черзі
    std::vector<bool> finished(n, false);  // чи завершено процес

    // Допоміжна лямбда: додає в чергу всі процеси, які вже прибули до моменту time
    auto addArrived = [&](int time) {
        for (int i = 0; i < n; ++i) {
            if (!finished[i] && !inQueue[i] && processes[i].arrivalTime <= time) {
                readyQueue.push(i);
                inQueue[i] = true;
            }
        }
        };

    // Починаємо з процесу з найменшим часом прибуття
    int firstIndex = 0;
    int earliestArrival = processes[0].arrivalTime;
    for (int i = 1; i < n; ++i) {
        if (processes[i].arrivalTime < earliestArrival) {
            earliestArrival = processes[i].arrivalTime;
            firstIndex = i;
        }
    }
    currentTime = earliestArrival;
    readyQueue.push(firstIndex);
    inQueue[firstIndex] = true;

    double totalWaiting = 0.0;
    double totalTurnaround = 0.0;

    std::cout << "\nExecution log (time slices):\n";

    while (completed < n) {
        if (readyQueue.empty()) {
            // Немає готових процесів — переходимо до наступного процесу, який прибуде
            int nextIndex = -1;
            int nextArrival = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (!finished[i] && processes[i].arrivalTime < nextArrival) {
                    nextArrival = processes[i].arrivalTime;
                    nextIndex = i;
                }
            }
            if (nextIndex == -1) break; // більше процесів немає
            currentTime = nextArrival;
            readyQueue.push(nextIndex);
            inQueue[nextIndex] = true;
        }

        int idx = readyQueue.front();
        readyQueue.pop();
        inQueue[idx] = false;

        Process& p = processes[idx];

        // Вперше цей процес отримує процесор
        if (p.startTime == -1) {
            p.startTime = currentTime;
        }

        int runTime = std::min(quantum, p.remainingTime);
        int startSlice = currentTime;
        currentTime += runTime;
        p.remainingTime -= runTime;

        std::cout << "t=" << startSlice << " .. " << currentTime
            << " | P" << p.id
            << " ran for " << runTime
            << ", remaining = " << p.remainingTime << "\n";

        // За цей час інші процеси могли прибути
        addArrived(currentTime);

        if (p.remainingTime == 0) {
            finished[idx] = true;
            p.finishTime = currentTime;
            p.turnaroundTime = p.finishTime - p.arrivalTime;
            p.waitingTime = p.turnaroundTime - p.burstTime;

            totalWaiting += p.waitingTime;
            totalTurnaround += p.turnaroundTime;
            completed++;
        }
        else {
            // Процес ще не завершився — повертаємо його в чергу
            readyQueue.push(idx);
            inQueue[idx] = true;
        }
    }

    // Виведення підсумкової таблиці
    std::cout << "\nResult table (Round Robin):\n";
    std::cout << std::left
        << std::setw(5) << "ID"
        << std::setw(10) << "Arrive"
        << std::setw(10) << "Burst"
        << std::setw(10) << "Start"
        << std::setw(10) << "Finish"
        << std::setw(12) << "Waiting"
        << std::setw(12) << "Turnaround"
        << "\n";

    for (const auto& p : processes) {
        std::cout << std::left
            << std::setw(5) << p.id
            << std::setw(10) << p.arrivalTime
            << std::setw(10) << p.burstTime
            << std::setw(10) << p.startTime
            << std::setw(10) << p.finishTime
            << std::setw(12) << p.waitingTime
            << std::setw(12) << p.turnaroundTime
            << "\n";
    }

    std::cout << "-----------------------------------------------\n";
    double avgW = totalWaiting / n;
    double avgT = totalTurnaround / n;
    std::cout << "Average waiting time:    " << avgW << "\n";
    std::cout << "Average turnaround time: " << avgT << "\n\n";

    return { avgW, avgT, "Round Robin" };
}


// Планування за пріоритетом (непереривчасте)

ResultSummary simulatePriority(std::vector<Process> processes) {
    int n = static_cast<int>(processes.size());
    if (n == 0) {
        std::cout << "\n=== Priority Scheduling (Non-preemptive) ===\nNo processes.\n";
        return { 0.0, 0.0, "Priority" };
    }

    // Reset fields
    for (auto& p : processes) {
        p.remainingTime = p.burstTime; // Залишок часу виконання
        p.startTime = -1; // Час старту
        p.finishTime = -1; // Час завершення
        p.waitingTime = 0; // Час очікування
        p.turnaroundTime = 0; // Оборотний час
        p.priority = p.initialPriority; // Відновлення початкового пріоритету

    }

    std::cout << "\n=== Priority Scheduling (Non-preemptive) ===\n";

    int currentTime = 0;
    int completed = 0;

    double totalWaiting = 0.0;
    double totalTurnaround = 0.0;

    std::vector<bool> done(n, false);

    while (completed < n) {
        int best = -1;
        int bestPriority = INT_MAX;

        //  Пошук процесу з найвищим пріоритетом серед доступних у currentTime
        for (int i = 0; i < n; ++i) {
            if (!done[i] && processes[i].arrivalTime <= currentTime) {
                if (processes[i].priority < bestPriority) {
                    bestPriority = processes[i].priority;
                    best = i;
                }
            }
        }

        // Якщо жоден процес зараз недоступний → переходимо до найближчого за часом прибуття
        if (best == -1) {
            int nextArrival = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (!done[i] && processes[i].arrivalTime < nextArrival) {
                    nextArrival = processes[i].arrivalTime;
                }
            }
            if (nextArrival == INT_MAX) break;
            currentTime = nextArrival;
            continue;
        }

        Process& p = processes[best];

        p.startTime = currentTime;
        p.finishTime = currentTime + p.burstTime;
        p.waitingTime = p.startTime - p.arrivalTime;
        p.turnaroundTime = p.finishTime - p.arrivalTime;

        currentTime = p.finishTime;
        done[best] = true;
        completed++;

        totalWaiting += p.waitingTime;
        totalTurnaround += p.turnaroundTime;
    }

    //  Підсумкова таблиця результатів (планування за пріоритетом)
    std::cout << "\nResult table (Priority Scheduling):\n";
    std::cout << std::left
        << std::setw(5) << "ID" // Ідентифікатор процесу
        << std::setw(10) << "Arrive" // Час приходу
        << std::setw(10) << "Burst" // Тривалість (CPU burst)
        << std::setw(10) << "Prio" // Пріоритет
        << std::setw(10) << "Start" // Час початку
        << std::setw(10) << "Finish" // Час завершення
        << std::setw(12) << "Waiting" // Час очікування
        << std::setw(12) << "Turnaround" // Час обороту
        << "\n";

    for (const auto& p : processes) {
        std::cout << std::left
            << std::setw(5) << p.id
            << std::setw(10) << p.arrivalTime
            << std::setw(10) << p.burstTime
            << std::setw(10) << p.initialPriority
            << std::setw(10) << p.startTime
            << std::setw(10) << p.finishTime
            << std::setw(12) << p.waitingTime
            << std::setw(12) << p.turnaroundTime
            << "\n";
    }

    std::cout << "-----------------------------------------------\n";
    double avgW = totalWaiting / n;
    double avgT = totalTurnaround / n;
    std::cout << "Average waiting time:    " << avgW << "\n";
    std::cout << "Average turnaround time: " << avgT << "\n\n";

    return { avgW, avgT, "Priority" };
}


// Планування Shortest Job First (SJF)

ResultSummary simulateSJF(std::vector<Process> processes) {
    int n = static_cast<int>(processes.size());
    if (n == 0) {
        std::cout << "\n=== Shortest Job First (SJF) ===\nNo processes.\n";
        return { 0.0, 0.0, "SJF" };
    }

    // Скидання полів, пов’язаних із симуляцією
    for (auto& p : processes) {
        p.remainingTime = p.burstTime;
        p.startTime = -1;
        p.finishTime = -1;
        p.waitingTime = 0;
        p.turnaroundTime = 0;
    }

    std::cout << "\n=== Shortest Job First (SJF, non-preemptive) ===\n";

    int currentTime = 0;
    int completed = 0;

    double totalWaiting = 0.0;
    double totalTurnaround = 0.0;

    std::vector<bool> done(n, false);

    // Починаємо з найранішого часу прибуття
    int earliestArrival = INT_MAX;
    for (int i = 0; i < n; ++i) {
        if (processes[i].arrivalTime < earliestArrival) {
            earliestArrival = processes[i].arrivalTime;
        }
    }
    if (earliestArrival == INT_MAX) {
        return { 0.0, 0.0, "SJF" };
    }
    currentTime = earliestArrival;

    while (completed < n) {
        int best = -1;
        int bestBurst = INT_MAX;

        // Серед доступних процесів (arrivalTime <= currentTime) шукаємо той, що має найменший burstTime
        for (int i = 0; i < n; ++i) {
            if (!done[i] && processes[i].arrivalTime <= currentTime) {
                if (processes[i].burstTime < bestBurst) {
                    bestBurst = processes[i].burstTime;
                    best = i;
                }
                else if (processes[i].burstTime == bestBurst) {
                    if (best != -1 && processes[i].id < processes[best].id) {
                        best = i;
                    }
                }
            }
        }

        // Якщо жоден процес зараз недоступний → переходимо до найближчого за часом прибуття
        if (best == -1) {
            int nextArrival = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (!done[i] && processes[i].arrivalTime < nextArrival) {
                    nextArrival = processes[i].arrivalTime;
                }
            }
            if (nextArrival == INT_MAX) break;
            currentTime = nextArrival;
            continue;
        }

        Process& p = processes[best];

        p.startTime = currentTime;
        p.finishTime = currentTime + p.burstTime;
        p.waitingTime = p.startTime - p.arrivalTime;
        p.turnaroundTime = p.finishTime - p.arrivalTime;

        currentTime = p.finishTime;
        done[best] = true;
        completed++;

        totalWaiting += p.waitingTime;
        totalTurnaround += p.turnaroundTime;
    }

    std::cout << "\nResult table (SJF):\n";
    std::cout << std::left
        << std::setw(5) << "ID"
        << std::setw(10) << "Arrive"
        << std::setw(10) << "Burst"
        << std::setw(10) << "Start"
        << std::setw(10) << "Finish"
        << std::setw(12) << "Waiting"
        << std::setw(12) << "Turnaround"
        << "\n";

    for (const auto& p : processes) {
        std::cout << std::left
            << std::setw(5) << p.id
            << std::setw(10) << p.arrivalTime
            << std::setw(10) << p.burstTime
            << std::setw(10) << p.startTime
            << std::setw(10) << p.finishTime
            << std::setw(12) << p.waitingTime
            << std::setw(12) << p.turnaroundTime
            << "\n";
    }

    std::cout << "-----------------------------------------------\n";
    double avgW = totalWaiting / n;
    double avgT = totalTurnaround / n;
    std::cout << "Average waiting time:    " << avgW << "\n";
    std::cout << "Average turnaround time: " << avgT << "\n\n";

    return { avgW, avgT, "SJF" };
}


// Динамічне планування за пріоритетом (з витісненням та старінням)

ResultSummary simulateDynamicPriority(std::vector<Process> processes) {
    int n = static_cast<int>(processes.size());
    if (n == 0) {
        std::cout << "\n=== Dynamic Priority Scheduling ===\nNo processes.\n";
        return { 0.0, 0.0, "Dynamic Priority" };
    }

    // Скидаємо поля та відновлюємо початкові пріоритети
    for (auto& p : processes) {
        p.remainingTime = p.burstTime;
        p.startTime = -1;
        p.finishTime = -1;
        p.waitingTime = 0;
        p.turnaroundTime = 0;
        p.priority = p.initialPriority;
    }

    std::cout << "\n=== Dynamic Priority Scheduling (Preemptive with Aging) ===\n";

    int currentTime = 0;
    int completed = 0;

    double totalWaiting = 0.0;
    double totalTurnaround = 0.0;

    std::vector<bool> finished(n, false);

    // Знаходимо найраніший час прибуття
    int earliestArrival = INT_MAX;
    for (int i = 0; i < n; ++i) {
        if (processes[i].arrivalTime < earliestArrival)
            earliestArrival = processes[i].arrivalTime;
    }
    if (earliestArrival == INT_MAX) {
        return { 0.0, 0.0, "Dynamic Priority" };
    }
    currentTime = earliestArrival;

    std::cout << "\nExecution log (time = 1 unit per step):\n";

    while (completed < n) {
        // Обираємо найкращий готовий процес за поточним (динамічним) пріоритетом
        int best = -1;
        int bestPrio = INT_MAX;

        for (int i = 0; i < n; ++i) {
            if (!finished[i] &&
                processes[i].arrivalTime <= currentTime &&
                processes[i].remainingTime > 0) {

                if (processes[i].priority < bestPrio) {
                    bestPrio = processes[i].priority;
                    best = i;
                }
            }
        }

        // Якщо жоден процес не готовий — перескакуємо до наступного часу прибуття (процесор простоює)
        if (best == -1) {
            int nextArrival = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (!finished[i] && processes[i].arrivalTime < nextArrival) {
                    nextArrival = processes[i].arrivalTime;
                }
            }
            if (nextArrival == INT_MAX) break;
            currentTime = nextArrival;
            continue;
        }

        Process& p = processes[best];

        if (p.startTime == -1) {
            p.startTime = currentTime; // перший вихід процесу на процесор
        }

        int startT = currentTime;

        // Виконуємо процес протягом 1 одиниці часу (можливе витіснення на кожному кроці)
        p.remainingTime--;
        currentTime++;

        std::cout << "t=" << startT
            << " | running P" << p.id
            << " (prio=" << p.priority << "), remaining="
            << p.remainingTime << "\n";

        // Старіння (aging): підвищуємо пріоритет усіх інших готових і тих, що очікують
        for (int i = 0; i < n; ++i) {
            if (i == best) continue;
            if (!finished[i] &&
                processes[i].arrivalTime <= currentTime &&
                processes[i].remainingTime > 0) {

                // Зменшуємо числове значення пріоритету, але не нижче 1
                if (processes[i].priority > 1)
                    processes[i].priority--;
            }
        }

        // Якщо процес щойно завершився
        if (p.remainingTime == 0) {
            finished[best] = true;
            p.finishTime = currentTime;
            p.turnaroundTime = p.finishTime - p.arrivalTime;
            p.waitingTime = p.turnaroundTime - p.burstTime;

            totalWaiting += p.waitingTime;
            totalTurnaround += p.turnaroundTime;
            completed++;
        }
    }

    // Підсумкова таблиця
    std::cout << "\nResult table (Dynamic Priority):\n";
    std::cout << std::left
        << std::setw(5) << "ID"
        << std::setw(10) << "Arrive"
        << std::setw(10) << "Burst"
        << std::setw(12) << "InitPrio"
        << std::setw(12) << "FinalPrio"
        << std::setw(10) << "Start"
        << std::setw(10) << "Finish"
        << std::setw(12) << "Waiting"
        << std::setw(12) << "Turnaround"
        << "\n";

    for (const auto& p : processes) {
        std::cout << std::left
            << std::setw(5) << p.id
            << std::setw(10) << p.arrivalTime
            << std::setw(10) << p.burstTime
            << std::setw(12) << p.initialPriority
            << std::setw(12) << p.priority
            << std::setw(10) << p.startTime
            << std::setw(10) << p.finishTime
            << std::setw(12) << p.waitingTime
            << std::setw(12) << p.turnaroundTime
            << "\n";
    }

    std::cout << "-----------------------------------------------\n";
    double avgW = totalWaiting / n;
    double avgT = totalTurnaround / n;
    std::cout << "Average waiting time:    " << avgW << "\n";
    std::cout << "Average turnaround time: " << avgT << "\n\n";

    return { avgW, avgT, "Dynamic Priority" };
}


// Запуск усіх алгоритмів та підсумкова таблиця

void runAllAlgorithms(const std::vector<Process>& base) {
    std::cout << "\n=== RUNNING ALL ALGORITHMS ON SAME PROCESS SET ===\n";

    std::vector<ResultSummary> results;

    // FCFS
    results.push_back(simulateFCFS(base));

    // Round Robin з фіксованим квантом (для порівняння)
    int quantum = 2;
    std::cout << "\n[INFO] Using quantum = " << quantum << " for Round Robin in summary mode.\n";
    results.push_back(simulateRoundRobin(base, quantum));

    // Пріоритетний 
    results.push_back(simulatePriority(base));

    // Динамічний пріоритет (зі старінням)
    results.push_back(simulateDynamicPriority(base));

    // SJF
    results.push_back(simulateSJF(base));

    std::cout << "\n=== SUMMARY TABLE (AVERAGE TIMES) ===\n";
    std::cout << std::left
        << std::setw(20) << "Algorithm"
        << std::setw(20) << "Avg Waiting"
        << std::setw(20) << "Avg Turnaround"
        << "\n";

    for (const auto& r : results) {
        std::cout << std::left
            << std::setw(20) << r.name
            << std::setw(20) << r.avgWaiting
            << std::setw(20) << r.avgTurnaround
            << "\n";
    }

    std::cout << "---------------------------------------------\n";
}

// ------------------------
// MAIN
// ------------------------
int main() {
    int n;
    std::cout << "Enter number of processes: ";
    std::cin >> n;

    if (n <= 0) {
        std::cout << "Invalid number.\n";
        return 0;
    }

    auto processes = generateProcesses(n);
    printProcesses(processes);

    while (true) {
        std::cout << "\nChoose algorithm:\n";
        std::cout << "1 - FCFS (First-Come-First-Served)\n";
        std::cout << "2 - Round Robin\n";
        std::cout << "3 - Priority Scheduling (non-preemptive)\n";
        std::cout << "4 - Dynamic Priority (preemptive, with aging)\n";
        std::cout << "5 - Shortest Job First (SJF)\n";
        std::cout << "6 - Run ALL algorithms and show summary\n";
        std::cout << "0 - Exit\n";
        std::cout << "Your choice: ";

        int choice;
        std::cin >> choice;

        if (!std::cin) {
            std::cout << "Input error. Exiting.\n";
            break;
        }

        if (choice == 0) {
            break;
        }
        else if (choice == 1) {
            simulateFCFS(processes);
        }
        else if (choice == 2) {
            int q;
            std::cout << "Enter time quantum: ";
            std::cin >> q;
            simulateRoundRobin(processes, q);
        }
        else if (choice == 3) {
            simulatePriority(processes);
        }
        else if (choice == 4) {
            simulateDynamicPriority(processes);
        }
        else if (choice == 5) {
            simulateSJF(processes);
        }
        else if (choice == 6) {
            runAllAlgorithms(processes);
        }
        else {
            std::cout << "Invalid choice.\n";
        }
    }

    return 0;
}


