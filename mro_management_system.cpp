#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <fstream>
#include <iomanip>
using namespace std;

/********************************************
 * Class: Problem
 * Represents a maintenance problem on the aircraft.
 ********************************************/
class Problem {
private:
    string problemID;
    string systemAffected;  // e.g., "Avionics", "Mechanical", "Hydraulic"
    string description;
public:
    Problem() {}
    Problem(const string& pID, const string& sys, const string& desc)
        : problemID(pID), systemAffected(sys), description(desc) {}

    string getProblemID() const { return problemID; }
    string getSystemAffected() const { return systemAffected; }
    string getDescription() const { return description; }
};

/********************************************
 * Class: Task
 * Represents a maintenance task with required steps and parts.
 ********************************************/
class Task {
private:
    string taskID;
    string taskName;
    string systemType; // Which system this task is for
    vector<string> steps;
    vector<string> requiredParts;
public:
    Task() {}
    Task(const string& tID, const string& tName, const string& sysType,
         const vector<string>& s, const vector<string>& parts)
        : taskID(tID), taskName(tName), systemType(sysType), steps(s), requiredParts(parts) {}

    string getTaskID() const { return taskID; }
    string getTaskName() const { return taskName; }
    string getSystemType() const { return systemType; }
    const vector<string>& getSteps() const { return steps; }
    const vector<string>& getRequiredParts() const { return requiredParts; }

    void printTaskDetails() const {
        cout << "Task ID: " << taskID << "\n"
             << "Task Name: " << taskName << "\n"
             << "System: " << systemType << "\n"
             << "Steps:\n";
        for (size_t i=0; i<steps.size(); i++){
            cout << i+1 << ". " << steps[i] << "\n";
        }
        cout << "Required Parts:\n";
        for (auto &p : requiredParts) {
            cout << "- " << p << "\n";
        }
    }
};

/********************************************
 * Class: Stock
 * Manages parts inventory.
 ********************************************/
class Stock {
private:
    // For simplicity, a map of partName -> quantity
    // We will store it in a vector of pairs here, but a map would be more suitable.
    // Using vector for demonstration.
    vector<pair<string,int>> partsInventory;
public:
    Stock() {}

    void addPart(const string& partName, int quantity) {
        for (auto &p : partsInventory) {
            if (p.first == partName) {
                p.second += quantity;
                return;
            }
        }
        partsInventory.push_back({partName, quantity});
    }

    bool checkPartAvailability(const string& partName, int qty) {
        for (auto &p : partsInventory) {
            if (p.first == partName) {
                return p.second >= qty;
            }
        }
        return false;
    }

    bool deductParts(const vector<string>& requiredParts) {
        // For simplicity, we will assume each required part needs a quantity of 1.
        // In a more complex system, the required quantity would also be tracked.
        for (auto &partName : requiredParts) {
            bool found = false;
            for (auto &inv : partsInventory) {
                if (inv.first == partName) {
                    found = true;
                    if (inv.second < 1) {
                        cout << "Not enough stock for part: " << partName << "\n";
                        return false;
                    }
                }
            }
            if (!found) {
                cout << "Part not found in stock: " << partName << "\n";
                return false;
            }
        }

        // Deduct now
        for (auto &partName : requiredParts) {
            for (auto &inv : partsInventory) {
                if (inv.first == partName) {
                    inv.second -= 1;
                    break;
                }
            }
        }

        return true;
    }

    void printStock() {
        cout << "\n--- Current Stock ---\n";
        for (auto &p : partsInventory) {
            cout << p.first << ": " << p.second << "\n";
        }
        cout << "---------------------\n";
    }
};

/********************************************
 * Class: MaintenanceReportCard
 * Records information about completed maintenance.
 ********************************************/
class MaintenanceReportCard {
private:
    string reportID;
    string date;
    vector<string> completedTasks;
    vector<string> usedParts;
public:
    MaintenanceReportCard() {}
    MaintenanceReportCard(const string& rID, const string& dt,
                          const vector<string>& tasks, const vector<string>& parts)
        : reportID(rID), date(dt), completedTasks(tasks), usedParts(parts) {}

    void printReport() const {
        cout << "Maintenance Report ID: " << reportID << "\n"
             << "Date: " << date << "\n"
             << "Completed Tasks:\n";
        for (auto &t : completedTasks) {
            cout << "- " << t << "\n";
        }
        cout << "Used Parts:\n";
        for (auto &p : usedParts) {
            cout << "- " << p << "\n";
        }
    }

    void saveToFile() const {
        ofstream ofs("maintenance_reports.txt", ios::app);
        if (ofs) {
            ofs << "Maintenance Report ID: " << reportID << "\n"
                << "Date: " << date << "\n"
                << "Completed Tasks:\n";
            for (auto &t : completedTasks) {
                ofs << "- " << t << "\n";
            }
            ofs << "Used Parts:\n";
            for (auto &p : usedParts) {
                ofs << "- " << p << "\n";
            }
            ofs << "------------------------\n";
            ofs.close();
        } else {
            cerr << "Could not open report file for writing.\n";
        }
    }
};


/********************************************
 * Helper Functions
 ********************************************/

string getCurrentDate() {
    time_t now = time(nullptr);
    tm *ltm = localtime(&now);
    char buffer[30];
    strftime(buffer, 30, "%Y-%m-%d", ltm);
    return string(buffer);
}

/********************************************
 * Main Program Demonstration
 ********************************************/

int main() {
    // Initialize some mock data
    // Example tasks:
    // 1) Avionics system check
    // 2) Hydraulic system leak fix
    // 3) Mechanical system gear lubrication

    vector<Task> taskDatabase;
    taskDatabase.push_back(
        Task("T001", "Avionics Diagnostic Check", "Avionics",
             {"Power off the avionics unit", "Remove protective covers", "Run diagnostic software", "Replace faulty modules if detected", "Reassemble and test system"},
             {"AvionicsModule", "ScrewSet", "DiagnosticKit"})
    );

    taskDatabase.push_back(
        Task("T002", "Hydraulic Leak Repair", "Hydraulic",
             {"Identify leak location", "Drain hydraulic fluid from reservoir", "Replace damaged O-rings", "Refill hydraulic fluid", "Test hydraulic pressure"},
             {"O-Ring", "HydraulicFluid", "WrenchSet"})
    );

    taskDatabase.push_back(
        Task("T003", "Landing Gear Lubrication", "Mechanical",
             {"Lift aircraft and secure", "Clean landing gear joints", "Apply lubrication grease", "Lower aircraft and perform operational check"},
             {"LubricationGrease", "RagSet"})
    );

    // Initialize stock
    Stock stock;
    stock.addPart("AvionicsModule", 2);
    stock.addPart("ScrewSet", 10);
    stock.addPart("DiagnosticKit", 1);
    stock.addPart("O-Ring", 5);
    stock.addPart("HydraulicFluid", 3);
    stock.addPart("WrenchSet", 2);
    stock.addPart("LubricationGrease", 4);
    stock.addPart("RagSet", 10);

    // Print current stock
    stock.printStock();

    // Create a problem scenario
    // Suppose we have a problem with Avionics system
    Problem p("P001", "Avionics", "Faulty avionics display detected during flight.");

    cout << "\n--- New Problem Reported ---\n";
    cout << "Problem ID: " << p.getProblemID() << "\n";
    cout << "System Affected: " << p.getSystemAffected() << "\n";
    cout << "Description: " << p.getDescription() << "\n";

    // Find tasks related to the affected system
    cout << "\n--- Suggested Tasks for " << p.getSystemAffected() << " ---\n";
    vector<Task> candidateTasks;
    for (auto &t : taskDatabase) {
        if (t.getSystemType() == p.getSystemAffected()) {
            candidateTasks.push_back(t);
        }
    }

    for (size_t i=0; i<candidateTasks.size(); i++) {
        cout << i+1 << ". " << candidateTasks[i].getTaskName() << " (Task ID: " << candidateTasks[i].getTaskID() << ")\n";
    }

    // Let's say the technician chooses the first task
    int choice = 1;
    Task chosenTask = candidateTasks[choice-1];

    cout << "\n--- Chosen Task Details ---\n";
    chosenTask.printTaskDetails();

    // Check part availability
    cout << "\nChecking required parts in stock...\n";
    bool partsAvailable = true;
    for (auto &part : chosenTask.getRequiredParts()) {
        if (!stock.checkPartAvailability(part, 1)) {
            cout << "Part not available in stock: " << part << ". Need to order.\n";
            partsAvailable = false;
        }
    }

    if (!partsAvailable) {
        cout << "Not all parts are available. Suppose we order them and restock.\n";
        // For simplicity, let's just add them now
        for (auto &part : chosenTask.getRequiredParts()) {
            if (!stock.checkPartAvailability(part, 1)) {
                stock.addPart(part, 5); // Add more parts
            }
        }
    }

    // Deduct parts from stock after completing the task
    cout << "\nDeducting parts from stock to perform the task...\n";
    if (stock.deductParts(chosenTask.getRequiredParts())) {
        cout << "Parts successfully deducted from stock.\n";
    } else {
        cout << "Could not complete task due to parts shortage.\n";
        return 0; // End scenario early
    }

    // After performing the task, create a maintenance report card
    // Generate a unique report ID
    string reportID = "RPT-" + to_string(rand()%10000 + 1000);
    vector<string> completedTasks;
    completedTasks.push_back(chosenTask.getTaskName());
    vector<string> usedParts = chosenTask.getRequiredParts();

    MaintenanceReportCard reportCard(reportID, getCurrentDate(), completedTasks, usedParts);
    cout << "\n--- Maintenance Report Card ---\n";
    reportCard.printReport();

    // Save the report
    reportCard.saveToFile();

    // Print final stock after deduction
    stock.printStock();

    cout << "\nProcess completed successfully.\n";

    return 0;
}
