// ProjectTwo.cpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Jordan Gales
// CS 300 Project Two - (ABCU Advising Program)

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

struct Course {
    string courseNumber;
    string courseTitle;
    vector<string> prerequisites;
};

string trim(const string& value) {
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos) {
        return "";
    }
    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

string toUpper(string value) {
    for (char& ch : value) {
        ch = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

// Splits the simple comma-separated course file used for this assignment.
vector<string> splitCSVLine(const string& line) {
    vector<string> fields;
    string field;
    stringstream stream(line);

    while (getline(stream, field, ',')) {
        fields.push_back(trim(field));
    }

    // Preserve a final empty field if the line ends in a comma.
    if (!line.empty() && line.back() == ',') {
        fields.push_back("");
    }
    return fields;
}

class BinarySearchTree {
private:
    struct Node {
        Course course;
        unique_ptr<Node> left;
        unique_ptr<Node> right;

        explicit Node(const Course& value) : course(value) {}
    };

    unique_ptr<Node> root;
    size_t courseCount = 0;

    // Compare course numbers at each node. Smaller keys go left; larger keys go right.
    // A new node is created at the first empty child position.
    bool insertNode(unique_ptr<Node>& node, const Course& course) {
        if (!node) {
            node = make_unique<Node>(course);
            return true;
        }

        if (course.courseNumber < node->course.courseNumber) {
            return insertNode(node->left, course);
        }
        if (course.courseNumber > node->course.courseNumber) {
            return insertNode(node->right, course);
        }

        // A matching course number already exists.
        return false;
    }

    // This BST search follows only one branch per comparison: left for smaller, right for larger.
    const Course* searchNode(const Node* node, const string& number) const {
        if (node == nullptr) {
            return nullptr;
        }
        if (number == node->course.courseNumber) {
            return &node->course;
        }
        if (number < node->course.courseNumber) {
            return searchNode(node->left.get(), number);
        }
        return searchNode(node->right.get(), number);
    }

    // In-order traversal (left, node, right) prints course numbers in sorted order.
    void inOrder(const Node* node, vector<Course>& result) const {
        if (node == nullptr) {
            return;
        }
        inOrder(node->left.get(), result);
        result.push_back(node->course);
        inOrder(node->right.get(), result);
    }

public:
    bool insert(const Course& course) {
        if (insertNode(root, course)) {
            ++courseCount;
            return true;
        }
        return false;
    }

    const Course* search(const string& number) const {
        return searchNode(root.get(), number);
    }

    vector<Course> getCoursesInOrder() const {
        vector<Course> result;
        result.reserve(courseCount);
        inOrder(root.get(), result);
        return result;
    }

    size_t size() const {
        return courseCount;
    }

    void clear() {
        root.reset();
        courseCount = 0;
    }
};

bool loadCourses(const string& filename, BinarySearchTree& tree) {
    ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        cout << "Unable to open file: " << filename << '\n';
        return false;
    }

    vector<Course> parsedCourses;
    unordered_set<string> courseNumbers;
    string line;
    int lineNumber = 0;

    while (getline(inputFile, line)) {
        ++lineNumber;
        line = trim(line);

        if (line.empty()) {
            continue;
        }

        vector<string> fields = splitCSVLine(line);
        if (fields.size() < 2 || fields[0].empty() || fields[1].empty()) {
            cout << "Skipping malformed record on line " << lineNumber
                << ": a course number and title are required.\n";
            continue;
        }

        Course course;
        course.courseNumber = toUpper(fields[0]);
        course.courseTitle = fields[1];

        // The remaining CSV fields are prerequisite course numbers.
        for (size_t i = 2; i < fields.size(); ++i) {
            string prerequisite = toUpper(trim(fields[i]));
            if (!prerequisite.empty() && prerequisite != "-1") {
                course.prerequisites.push_back(prerequisite);
            }
        }

        if (!courseNumbers.insert(course.courseNumber).second) {
            cout << "Skipping duplicate course number " << course.courseNumber
                << " on line " << lineNumber << ".\n";
            continue;
        }

        parsedCourses.push_back(course);
    }

    inputFile.close();

    if (parsedCourses.empty()) {
        cout << "No valid course records were found. Data was not loaded.\n";
        return false;
    }

    // Validate prerequisite IDs after reading every row, so prerequisites may
    // appear before or after the course that references them.
    bool prerequisitesValid = true;
    for (const Course& course : parsedCourses) {
        for (const string& prerequisite : course.prerequisites) {
            if (courseNumbers.find(prerequisite) == courseNumbers.end()) {
                cout << "Warning: " << course.courseNumber
                    << " lists missing prerequisite " << prerequisite << ".\n";
                prerequisitesValid = false;
            }
        }
    }

    // Replace the old tree only after parsing. This avoids clearing existing data
    // when the file cannot be opened or contains no valid course records.
    tree.clear();
    for (const Course& course : parsedCourses) {
        tree.insert(course);
    }

    cout << "Loaded " << tree.size() << " courses from " << filename << ".\n";
    if (!prerequisitesValid) {
        cout << "Please verify the prerequisite warnings above.\n";
    }
    return true;
}

void printCourseList(const BinarySearchTree& tree) {
    if (tree.size() == 0) {
        cout << "No course data is loaded. Please choose option 1 first.\n";
        return;
    }

    cout << "\nHere is the course list:\n\n";
    for (const Course& course : tree.getCoursesInOrder()) {
        cout << course.courseNumber << ", " << course.courseTitle << '\n';
    }
}

void printCourseInformation(const BinarySearchTree& tree,
    const string& requestedNumber) {
    if (tree.size() == 0) {
        cout << "No course data is loaded. Please choose option 1 first.\n";
        return;
    }

    const string courseNumber = toUpper(trim(requestedNumber));
    const Course* course = tree.search(courseNumber);

    if (course == nullptr) {
        cout << "Course " << courseNumber << " not found.\n";
        return;
    }

    cout << course->courseNumber << ", " << course->courseTitle << '\n';

    if (course->prerequisites.empty()) {
        cout << "Prerequisites: None\n";
        return;
    }

    cout << "Prerequisites:\n";
    for (const string& prerequisiteNumber : course->prerequisites) {
        const Course* prerequisite = tree.search(prerequisiteNumber);
        if (prerequisite != nullptr) {
            cout << "  " << prerequisite->courseNumber << ", "
                << prerequisite->courseTitle << '\n';
        }
        else {
            // Still show the ID if the file contains an unresolved prerequisite.
            cout << "  " << prerequisiteNumber << " (course title unavailable)\n";
        }
    }
}

void displayMenu() {
    cout << "\n1. Load Data Structure.\n"
        << "2. Print Course List.\n"
        << "3. Print Course.\n"
        << "9. Exit\n\n"
        << "What would you like to do? ";
}

int main() {
    BinarySearchTree courseTree;
    string choice;

    cout << "Welcome to the course planner.\n";

    while (true) {
        displayMenu();

        if (!getline(cin, choice)) {
            cout << "\nThank you for using the course planner!\n";
            break;
        }

        choice = trim(choice);

        if (choice == "1") {
            cout << "Enter the course data file name: ";
            string filename;
            if (!getline(cin, filename)) {
                cout << "Unable to read file name.\n";
                break;
            }
            filename = trim(filename);
            if (filename.empty()) {
                cout << "File name cannot be blank.\n";
            }
            else {
                loadCourses(filename, courseTree);
            }
        }
        else if (choice == "2") {
            printCourseList(courseTree);
        }
        else if (choice == "3") {
            cout << "What course do you want to know about? ";
            string courseNumber;
            if (!getline(cin, courseNumber)) {
                cout << "Unable to read course number.\n";
                break;
            }
            if (trim(courseNumber).empty()) {
                cout << "Course number cannot be blank.\n";
            }
            else {
                printCourseInformation(courseTree, courseNumber);
            }
        }
        else if (choice == "9") {
            cout << "Thank you for using the course planner!\n";
            break;
        }
        else {
            cout << choice << " is not a valid option.\n";
        }
    }

    return 0;
}

