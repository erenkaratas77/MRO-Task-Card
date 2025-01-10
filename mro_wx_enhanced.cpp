#include <wx/wx.h>
#include <wx/listbox.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <limits>

// --------------------------- Data Structures ---------------------------

// Representation of a Task with steps and required parts
struct Task {
    std::string name;
    std::vector<std::string> steps;
    std::vector<std::string> requiredParts;
};

// Global map: System -> vector of Task
static std::map<std::string, std::vector<Task>> systemTasks;

// Global stock map: part -> quantity
static std::map<std::string, int> stockInventory; 
// NOT initialized in code now; will be loaded from "stock.txt"

// Global string to store user-chosen aircraft type
static std::string g_chosenAircraft = "";

// --------------------------- Helper Functions ---------------------------

// 1) Load tasks from a text file
bool LoadTasksFromFile(const std::string &filename) 
{
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        wxLogError("Failed to open tasks file: %s", filename);
        return false;
    }
    // Format per line: SystemName|TaskName|step1,step2,step3...|part1,part2,part3...
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::vector<std::string> tokens;
        std::string temp;
        while (std::getline(ss, temp, '|')) {
            tokens.push_back(temp);
        }
        if (tokens.size() < 4) {
            wxLogWarning("Invalid task format: %s", line);
            continue;
        }

        std::string sysName   = tokens[0];
        std::string taskName  = tokens[1];
        std::string stepsCSV  = tokens[2];
        std::string partsCSV  = tokens[3];

        // Convert stepsCSV into a vector
        std::vector<std::string> steps;
        {
            std::stringstream ssteps(stepsCSV);
            while (std::getline(ssteps, temp, ',')) {
                if (!temp.empty()) steps.push_back(temp);
            }
        }

        // Convert partsCSV into a vector
        std::vector<std::string> parts;
        {
            std::stringstream sparts(partsCSV);
            while (std::getline(sparts, temp, ',')) {
                if (!temp.empty()) parts.push_back(temp);
            }
        }

        Task t{taskName, steps, parts};
        systemTasks[sysName].push_back(t);
    }
    ifs.close();
    return true;
}

// 2) Load stock from a text file (part|quantity)
bool LoadStockFromFile(const std::string &filename)
{
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        wxLogError("Failed to open stock file: %s", filename);
        return false;
    }
    stockInventory.clear(); // Start fresh
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string partName;
        std::string qtyStr;
        if (!std::getline(ss, partName, '|')) {
            wxLogWarning("Invalid stock format line: %s", line);
            continue;
        }
        if (!std::getline(ss, qtyStr, '|')) {
            // might not have another '|'
            wxLogWarning("Invalid stock format line (missing qty): %s", line);
            continue;
        }
        int quantity = 0;
        try {
            quantity = std::stoi(qtyStr);
        } catch(...) {
            wxLogWarning("Invalid quantity for part: %s in line: %s", qtyStr, line);
            continue;
        }
        stockInventory[partName] = quantity;
    }
    ifs.close();
    return true;
}

// Return current date as a string
std::string GetCurrentDate() {
    time_t now = time(nullptr);
    tm *ltm = localtime(&now);
    char buffer[30];
    strftime(buffer, 30, "%Y-%m-%d", ltm);
    return std::string(buffer);
}

// --------------------------- TaskStepsDialog ---------------------------
// A dialog that shows step-by-step tasks with checkboxes.
// User must complete each step in order before finishing.

class TaskStepsDialog : public wxDialog
{
public:
    TaskStepsDialog(wxWindow* parent,
                    const wxString& title,
                    const Task &task)
        : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(400, 300)),
          m_task(task)
    {
        wxPanel *panel = new wxPanel(this, wxID_ANY);
        wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

        // Instructions
        wxStaticText *label = new wxStaticText(panel, wxID_ANY,
            "Check each step in order. You cannot check step i+1 before step i.");
        mainSizer->Add(label, 0, wxALL | wxEXPAND, 5);

        // Create checkboxes for steps
        for (size_t i=0; i<m_task.steps.size(); i++) {
            wxString stepLabel = wxString::Format("%zu. %s", i+1, m_task.steps[i]);
            wxCheckBox *cb = new wxCheckBox(panel, 1000 + i, stepLabel);
            cb->Bind(wxEVT_CHECKBOX, &TaskStepsDialog::OnCheckBox, this);
            mainSizer->Add(cb, 0, wxLEFT | wxRIGHT | wxTOP, 5);
            m_checkBoxes.push_back(cb);
        }

        // Initially, only the first checkbox is enabled
        for (size_t i=1; i<m_checkBoxes.size(); i++) {
            m_checkBoxes[i]->Enable(false);
        }

        // Finish button
        m_finishButton = new wxButton(panel, wxID_OK, "Finish");
        m_finishButton->Bind(wxEVT_BUTTON, &TaskStepsDialog::OnFinish, this);
        m_finishButton->Enable(false); // disabled until all steps are checked
        mainSizer->Add(m_finishButton, 0, wxALL | wxALIGN_RIGHT, 10);

        panel->SetSizer(mainSizer);
        mainSizer->Fit(this);
    }

private:
    Task m_task;
    std::vector<wxCheckBox*> m_checkBoxes;
    wxButton *m_finishButton;

    void OnCheckBox(wxCommandEvent &event)
    {
        // Find which checkbox triggered
        wxCheckBox *cb = dynamic_cast<wxCheckBox*>(event.GetEventObject());
        if (!cb) return;

        // If this is not the last step, enable the next step's checkbox
        for (size_t i=0; i<m_checkBoxes.size(); i++) {
            if (m_checkBoxes[i] == cb && cb->IsChecked()) {
                // enable the next
                if (i+1 < m_checkBoxes.size()) {
                    m_checkBoxes[i+1]->Enable(true);
                }
            }
        }

        // Check if all are checked
        bool allChecked = true;
        for (auto &c : m_checkBoxes) {
            if (!c->IsChecked()) {
                allChecked = false;
                break;
            }
        }
        // If all steps are done, enable the Finish button
        m_finishButton->Enable(allChecked);
    }

    void OnFinish(wxCommandEvent &)
    {
        // All steps done, close dialog
        EndModal(wxID_OK);
    }
};

// --------------------------- AircraftSelectPanel ---------------------------

class AircraftSelectPanel : public wxPanel
{
public:
    AircraftSelectPanel(wxWindow *parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, "AircraftSelectPanel")
    {
        // Some example aircraft
        m_aircraftTypes = {"Boeing 737", "Airbus A320", "Gulfstream G550"};

        wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
        wxStaticText *title = new wxStaticText(this, wxID_ANY, "Select Aircraft to Service:");
        mainSizer->Add(title, 0, wxALL, 10);

        m_choice = new wxChoice(this, wxID_ANY);
        for (auto &ac : m_aircraftTypes) {
            m_choice->Append(ac);
        }
        mainSizer->Add(m_choice, 0, wxALL | wxEXPAND, 10);

        wxButton *btnSelect = new wxButton(this, wxID_ANY, "Confirm Aircraft");
        btnSelect->Bind(wxEVT_BUTTON, &AircraftSelectPanel::OnConfirmAircraft, this);
        mainSizer->Add(btnSelect, 0, wxALL | wxALIGN_RIGHT, 10);

        SetSizer(mainSizer);
    }

private:
    wxChoice *m_choice;
    std::vector<std::string> m_aircraftTypes;

    void OnConfirmAircraft(wxCommandEvent &)
    {
        int sel = m_choice->GetSelection();
        if (sel == wxNOT_FOUND) {
            wxMessageBox("Please select an aircraft.", "Error", wxOK | wxICON_ERROR);
            return;
        }
        g_chosenAircraft = m_choice->GetString(sel).ToStdString();
        wxMessageBox("Chosen Aircraft: " + m_choice->GetString(sel), "Info", wxOK | wxICON_INFORMATION);

        // Parent is MainFrame
        wxFrame* parentFrame = dynamic_cast<wxFrame*>(GetParent());
        if (parentFrame) {
            parentFrame->SetTitle("MRO Management System - " + m_choice->GetString(sel));
        }

        // Hide this panel, show the MaintenancePanel
        this->Hide();
        wxWindow *maintenancePanel = GetParent()->FindWindowByName("MaintenancePanel");
        if (maintenancePanel) {
            maintenancePanel->Show();
        }
        GetParent()->Layout();
    }
};

// --------------------------- MaintenancePanel -----------------------------

class MaintenancePanel : public wxPanel
{
public:
    MaintenancePanel(wxWindow *parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, "MaintenancePanel")
    {
        wxPanel *panel = this;
        wxStaticText *labSys = new wxStaticText(panel, wxID_ANY, "Select System:");
        m_systemChoice = new wxChoice(panel, wxID_ANY);
        m_systemChoice->Append("Avionics");
        m_systemChoice->Append("Hydraulic");
        m_systemChoice->Append("Mechanical");
        m_systemChoice->Bind(wxEVT_CHOICE, &MaintenancePanel::OnSelectSystem, this);

        // Task List
        wxStaticText *labTasks = new wxStaticText(panel, wxID_ANY, "Available Tasks:");
        m_taskList = new wxListBox(panel, wxID_ANY);
        m_taskList->Bind(wxEVT_LISTBOX, &MaintenancePanel::OnTaskSelected, this);

        // Task Details
        wxStaticText *labDetails = new wxStaticText(panel, wxID_ANY, "Task Details:");
        m_taskDetails = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(400, 180), wxTE_MULTILINE | wxTE_READONLY);

        // Start Steps Button
        m_startStepsButton = new wxButton(panel, wxID_ANY, "Start Task Steps");
        m_startStepsButton->Bind(wxEVT_BUTTON, &MaintenancePanel::OnStartSteps, this);
        m_startStepsButton->Enable(false);

        // Change Aircraft (go back) Button
        wxButton *btnChangeAircraft = new wxButton(panel, wxID_ANY, "Change Aircraft");
        btnChangeAircraft->Bind(wxEVT_BUTTON, &MaintenancePanel::OnChangeAircraft, this);

        // Stock Display
        wxStaticText *labStock = new wxStaticText(panel, wxID_ANY, "Current Stock:");
        m_stockDisplay = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(300, 120), wxTE_MULTILINE | wxTE_READONLY);

        // Report Output
        wxStaticText *labReport = new wxStaticText(panel, wxID_ANY, "Maintenance Report Log:");
        m_reportOutput = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(400, 150), wxTE_MULTILINE | wxTE_READONLY);

        // Layout
        wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

        // System + Tasks
        wxBoxSizer *topSizer = new wxBoxSizer(wxHORIZONTAL);
        wxBoxSizer *leftSizer = new wxBoxSizer(wxVERTICAL);
        leftSizer->Add(labSys, 0, wxALL, 5);
        leftSizer->Add(m_systemChoice, 0, wxALL, 5);
        leftSizer->Add(labTasks, 0, wxALL, 5);
        leftSizer->Add(m_taskList, 1, wxEXPAND | wxALL, 5);
        leftSizer->Add(m_startStepsButton, 0, wxALL, 5);

        // Add the "Change Aircraft" button below or above
        leftSizer->Add(btnChangeAircraft, 0, wxALL, 5);

        wxBoxSizer *rightSizer = new wxBoxSizer(wxVERTICAL);
        rightSizer->Add(labDetails, 0, wxALL, 5);
        rightSizer->Add(m_taskDetails, 0, wxALL | wxEXPAND, 5);
        rightSizer->Add(labStock, 0, wxALL, 5);
        rightSizer->Add(m_stockDisplay, 0, wxALL | wxEXPAND, 5);

        topSizer->Add(leftSizer, 0, wxEXPAND);
        topSizer->AddSpacer(15);
        topSizer->Add(rightSizer, 0, wxEXPAND);

        // Report area
        wxBoxSizer *bottomSizer = new wxBoxSizer(wxVERTICAL);
        bottomSizer->Add(labReport, 0, wxALL, 5);
        bottomSizer->Add(m_reportOutput, 1, wxEXPAND | wxALL, 5);

        mainSizer->Add(topSizer, 0, wxEXPAND | wxALL, 5);
        mainSizer->Add(bottomSizer, 1, wxEXPAND | wxALL, 5);

        panel->SetSizer(mainSizer);

        UpdateStockDisplay();
    }

private:
    wxChoice    *m_systemChoice;
    wxListBox   *m_taskList;
    wxTextCtrl  *m_taskDetails;
    wxButton    *m_startStepsButton;
    wxTextCtrl  *m_stockDisplay;
    wxTextCtrl  *m_reportOutput;

    // Current selections
    std::string m_currentSystem;
    Task        m_currentTask;

    // --- Event Handlers ---
    void OnSelectSystem(wxCommandEvent &)
    {
        m_currentSystem.clear();
        m_currentTask = Task{};
        m_taskList->Clear();
        m_taskDetails->Clear();
        m_startStepsButton->Enable(false);

        int sel = m_systemChoice->GetSelection();
        if (sel == wxNOT_FOUND) return;

        m_currentSystem = m_systemChoice->GetString(sel).ToStdString();

        // Find tasks in systemTasks map
        auto it = systemTasks.find(m_currentSystem);
        if (it == systemTasks.end()) {
            // no tasks found
            return;
        }
        // Populate m_taskList
        for (auto &t : it->second) {
            m_taskList->Append(t.name);
        }
    }

    void OnTaskSelected(wxCommandEvent &)
    {
        int sel = m_taskList->GetSelection();
        if (sel == wxNOT_FOUND) {
            m_currentTask = Task{};
            m_taskDetails->Clear();
            m_startStepsButton->Enable(false);
            return;
        }

        std::string chosenTaskName = m_taskList->GetString(sel).ToStdString();

        // Find the task in systemTasks[m_currentSystem]
        auto it = systemTasks.find(m_currentSystem);
        if (it == systemTasks.end()) return;

        for (auto &t : it->second) {
            if (t.name == chosenTaskName) {
                m_currentTask = t;
                break;
            }
        }
        UpdateTaskDetails(m_currentTask);
        m_startStepsButton->Enable(true);
    }

    void OnStartSteps(wxCommandEvent &)
    {
        if (m_currentSystem.empty() || m_currentTask.name.empty()) {
            wxMessageBox("Please select a system and a task first.", "Error", wxOK | wxICON_ERROR);
            return;
        }
        // Show the TaskStepsDialog
        TaskStepsDialog dlg(this, "Task Steps", m_currentTask);
        if (dlg.ShowModal() == wxID_OK) {
            // Step-based tasks completed
            // Now we check/deduct parts, then generate a report
            if (!CheckAndDeductParts(m_currentTask.requiredParts)) {
                wxMessageBox("Not enough parts in stock. Please restock!", "Error", wxOK | wxICON_ERROR);
                return;
            }
            // Append to report
            AppendReport(m_currentSystem, m_currentTask, m_currentTask.requiredParts);

            // Clear selection
            m_taskList->SetSelection(wxNOT_FOUND);
            m_taskDetails->Clear();
            m_currentTask = Task{};
            m_startStepsButton->Enable(false);

            // Update stock
            UpdateStockDisplay();
        }
    }

    void OnChangeAircraft(wxCommandEvent &)
    {
        // Kullanıcı geri dönmek istediğinde bu fonksiyon tetiklenecek.
        // Paneli gizle, AircraftSelectPanel'i göster.
        wxWindow *selectPanel = GetParent()->FindWindowByName("AircraftSelectPanel");
        if (selectPanel) {
            selectPanel->Show();
        }
        this->Hide();

        // Mevcut pencere başlığını sıfırla
        wxFrame* parentFrame = dynamic_cast<wxFrame*>(GetParent());
        if (parentFrame) {
            parentFrame->SetTitle("MRO Management System");
        }
        // Global aircraft değişkenini temizliyoruz (opsiyonel)
        g_chosenAircraft.clear();

        GetParent()->Layout();
    }

    // --- Utility ---
    void UpdateStockDisplay()
    {
        m_stockDisplay->Clear();
        for (auto &item : stockInventory) {
            m_stockDisplay->AppendText(item.first + ": " + std::to_string(item.second) + "\n");
        }
    }

    void UpdateTaskDetails(const Task &task)
    {
        m_taskDetails->Clear();
        m_taskDetails->AppendText("Task Name: " + task.name + "\n\n");
        m_taskDetails->AppendText("Steps:\n");
        for (size_t i=0; i<task.steps.size(); i++) {
            m_taskDetails->AppendText(std::to_string(i+1) + ". " + task.steps[i] + "\n");
        }
        m_taskDetails->AppendText("\nRequired Parts:\n");
        for (auto &p : task.requiredParts) {
            m_taskDetails->AppendText("- " + p + "\n");
        }
    }

    bool CheckAndDeductParts(const std::vector<std::string>& parts)
    {
        for (auto &pt : parts) {
            auto it = stockInventory.find(pt);
            if (it == stockInventory.end() || it->second < 1) {
                return false;
            }
        }
        // If all good, deduct
        for (auto &pt : parts) {
            stockInventory[pt] -= 1;
        }
        return true;
    }

    void AppendReport(const std::string &system, const Task &task, const std::vector<std::string> &usedParts)
    {
        static int reportCounter = 1000;
        reportCounter++;
        std::string reportID = "RPT-" + std::to_string(reportCounter);

        // Current date
        std::string dateStr = GetCurrentDate();

        std::string report = "=== Maintenance Report ===\n";
        report += "Report ID: " + reportID + "\n";
        report += "Date: " + dateStr + "\n";
        if (!g_chosenAircraft.empty())
            report += "Aircraft: " + g_chosenAircraft + "\n";
        report += "System: " + system + "\n";
        report += "Completed Task: " + task.name + "\n";
        report += "Used Parts:\n";
        for (auto &p : usedParts) {
            report += "  - " + p + "\n";
        }
        report += "==========================\n\n";

        m_reportOutput->AppendText(report);

        // Optionally save to file
        std::ofstream ofs("maintenance_reports.txt", std::ios::app);
        if (ofs) {
            ofs << report;
            ofs.close();
        }
    }
};

// --------------------------- MainFrame ---------------------------

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString &title)
        : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1000, 700))
    {
        // Create two main panels
        m_aircraftSelectPanel = new AircraftSelectPanel(this);
        m_maintenancePanel    = new MaintenancePanel(this);
        m_maintenancePanel->SetName("MaintenancePanel");
        m_maintenancePanel->Hide(); // hide initially

        // Give the first panel a name to find it easily
        m_aircraftSelectPanel->SetName("AircraftSelectPanel");

        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(m_aircraftSelectPanel, 1, wxEXPAND);
        sizer->Add(m_maintenancePanel, 1, wxEXPAND);
        SetSizer(sizer);
    }

private:
    AircraftSelectPanel *m_aircraftSelectPanel;
    MaintenancePanel    *m_maintenancePanel;
};

// --------------------------- Application Class ---------------------------

class MROApp : public wxApp 
{
public:
    virtual bool OnInit();
};

wxIMPLEMENT_APP(MROApp);

bool MROApp::OnInit()
{
    // 1) Load tasks from "tasks.txt"
    if (!LoadTasksFromFile("tasks.txt")) {
        wxMessageBox("Could not load tasks from tasks.txt. Proceeding with empty tasks!", 
                     "Warning", wxOK | wxICON_WARNING);
    }
    // 2) Load stock from "stock.txt"
    if (!LoadStockFromFile("stock.txt")) {
        wxMessageBox("Could not load stock from stock.txt. Proceeding with empty stock!", 
                     "Warning", wxOK | wxICON_WARNING);
    }

    // 3) Create MainFrame
    MainFrame *frame = new MainFrame("MRO Management System");
    frame->Show(true);
    return true;
}
