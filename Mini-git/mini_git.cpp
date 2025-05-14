#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <unordered_map>
#include <iomanip>
#include <sys/stat.h> // for mkdir() on POSIX
#include <cstdlib> // for system()

const std::string REPO_DIR = ".mini_git";
const std::string COMMITS_DIR = ".mini_git/commits";
const std::string INDEX_FILE = ".mini_git/index.txt";

struct Commit {
    std::string id;
    std::string message;
    std::string timestamp;
};

std::string currentDateTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::string str = std::ctime(&now_time);
    str.pop_back(); // remove newline
    return str;
}

std::string generateCommitID(const std::string& content) {
    std::hash<std::string> hasher;
    size_t hash = hasher(content);
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str().substr(0, 7);
}

void createDirectory(const std::string& dir) {
    // Using system() to create a directory (works for Unix-like systems)
    std::string cmd = "mkdir -p " + dir; // On Linux/Mac
    system(cmd.c_str());
}

void init() {
    if (!std::ifstream(INDEX_FILE)) {
        createDirectory(REPO_DIR);
        createDirectory(COMMITS_DIR);
        std::ofstream index(INDEX_FILE);
        index << "[FILES]\n[COMMITS]\n";
        std::cout << "Initialized Mini Git repository." << std::endl;
    } else {
        std::cout << "Repository already exists." << std::endl;
    }
}

void addFile(const std::string& filename) {
    if (!std::ifstream(filename)) {
        std::cout << "File doesn't exist." << std::endl;
        return;
    }

    std::ifstream index(INDEX_FILE);
    std::string line;
    bool alreadyTracked = false;

    // Find the [FILES] section
    while (std::getline(index, line)) {
        if (line == "[FILES]") break;
    }
    // Check if the file is already tracked
    while (std::getline(index, line)) {
        if (line == "[COMMITS]") break;
        if (line == filename) {
            alreadyTracked = true;
            break;
        }
    }

    index.close();

    // Add the file to the [FILES] section if not already tracked
    if (alreadyTracked) {
        std::cout << "File is already being tracked." << std::endl;
        return;
    }

    // Open index.txt in append mode and add the file
    std::ofstream indexAppend(INDEX_FILE, std::ios::app);
    indexAppend << filename << std::endl;
    std::cout << "Added file to tracking: " << filename << std::endl;
}

std::vector<std::string> getTrackedFiles() {
    std::ifstream index(INDEX_FILE);
    std::vector<std::string> files;
    std::string line;

    while (std::getline(index, line)) {
        if (line == "[FILES]") break;
    }
    while (std::getline(index, line)) {
        if (line == "[COMMITS]") break;
        files.push_back(line);
    }

    return files;
}

void commit(const std::string& message) {
    std::vector<std::string> files = getTrackedFiles();
    if (files.empty()) {
        std::cout << "No files are being tracked." << std::endl;
        return;
    }

    std::string allContent;
    for (const auto& file : files) {
        std::ifstream in(file);
        std::stringstream buffer;
        buffer << in.rdbuf();
        allContent += buffer.str();
    }

    std::string commitID = generateCommitID(allContent + message);
    std::string commitFolder = COMMITS_DIR + "/" + commitID;
    createDirectory(commitFolder);

    for (const auto& file : files) {
        std::ifstream src(file, std::ios::binary);
        std::ofstream dest(commitFolder + "/" + file, std::ios::binary);
        dest << src.rdbuf();
    }

    std::ofstream index(INDEX_FILE, std::ios::app);
    index << "[COMMIT]" << std::endl;
    index << "ID: " << commitID << std::endl;
    index << "Message: " << message << std::endl;
    index << "Time: " << currentDateTime() << std::endl;
    index << std::endl;

    std::cout << "Committed changes. ID: " << commitID << std::endl;
}

void showLog() {
    std::ifstream index(INDEX_FILE);
    std::string line;
    bool inCommit = false;

    while (std::getline(index, line)) {
        if (line == "[COMMIT]") {
            inCommit = true;
            std::cout << "---------------------------------" << std::endl;
        } else if (line.empty()) {
            inCommit = false;
        }

        if (inCommit && !line.empty() && line != "[COMMIT]") {
            std::cout << line << std::endl;
        }
    }
}

void checkout(const std::string& commitID) {
std::string folder = COMMITS_DIR + "/" + commitID;

// Check if the commit directory exists (simple check using stat)
struct stat buffer;
if (stat(folder.c_str(), &buffer) != 0) {
    std::cout << "Commit not found." << std::endl;
    return;
}

// If commit exists, copy files back to the current directory
for (const auto& file : getTrackedFiles()) {
    std::string filePath = folder + "/" + file;
    if (std::ifstream(filePath)) {
        std::ifstream src(filePath, std::ios::binary);
        std::ofstream dest(file, std::ios::binary);
        dest << src.rdbuf();
        std::cout << "Restored file: " << file << std::endl;
    } else {
        std::cout << "File " << file << " not found in commit." << std::endl;
    }
}

std::cout << "Restored files from commit " << commitID << std::endl;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./mini_git <command> [args]" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "init") {
        init();
    } else if (command == "add") {
        if (argc < 3) {
            std::cout << "Usage: ./mini_git add <filename>" << std::endl;
        } else {
            addFile(argv[2]);
        }
    } else if (command == "commit") {
        if (argc < 3) {
            std::cout << "Usage: ./mini_git commit <message>" << std::endl;
        } else {
            std::string msg;
            for (int i = 2; i < argc; ++i) {
                msg += argv[i];
                if (i != argc - 1) msg += " ";
            }
            commit(msg);
        }
    } else if (command == "log") {
        showLog();
    } else if (command == "checkout") {
        if (argc < 3) {
            std::cout << "Usage: ./mini_git checkout <commit_id>" << std::endl;
        } else {
            checkout(argv[2]);
        }
    } else {
        std::cout << "Unknown command." << std::endl;
    }

    return 0;
}
