#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <string>
#include <io.h>
#include <sys/stat.h>
#include <algorithm>
#include "nlohmann/json.hpp"
using json = nlohmann::json;


using namespace std;

// Clean a word: lowercase + remove punctuation
string cleanWord(const string& w) {
    string result;
    for (char c : w) {
        if (isalnum(static_cast<unsigned char>(c)))
            result += tolower(c);
    }
    return result;
}

// Process any text file (txt, csv, tsv, log, md)
void processTextFile(const string& filepath, unordered_map<string, int>& lexicon) {
    cout << "Reading file: " << filepath << endl;

    ifstream file(filepath);
    if (!file.is_open()) {
        cout << "Error opening file: " << filepath << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string word;
        while (ss >> word) {
            string cleaned = cleanWord(word);
            if (!cleaned.empty())
                lexicon[cleaned]++;
        }
    }

    file.close();
}

// Windows-safe recursive folder walk
void readAllFiles(const string& path, unordered_map<string, int>& lexicon) {
    struct _finddata_t data;
    intptr_t handle;

    // Check if path is a folder
    struct _stat info;
    if (_stat(path.c_str(), &info) != 0) {
        cout << "Cannot access: " << path << endl;
        return;
    }

    if (info.st_mode & _S_IFDIR) {
        // Folder → recurse
        string searchPath = path + "\\*";
        handle = _findfirst(searchPath.c_str(), &data);
        if (handle == -1) {
            cout << "Cannot open folder: " << path << endl;
            return;
        }

        do {
            string name = data.name;
            if (name == "." || name == "..") continue;

            string fullPath = path + "\\" + name;

            if (data.attrib & _A_HIDDEN) continue;
            if (data.attrib & _A_SYSTEM) continue;

            if (data.attrib & _A_SUBDIR) {
                readAllFiles(fullPath, lexicon);
            }
            else {
                string ext = fullPath.substr(fullPath.find_last_of(".") + 1);
                transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (ext == "txt" || ext == "csv" || ext == "tsv" || ext == "log" || ext == "md")
                    processTextFile(fullPath, lexicon);
            }

        } while (_findnext(handle, &data) == 0);

        _findclose(handle);
    }
    else {
        // Path is a single file
        string ext = path.substr(path.find_last_of(".") + 1);
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == "txt" || ext == "csv" || ext == "tsv" || ext == "log" || ext == "md")
            processTextFile(path, lexicon);
        else
            cout << "Skipping unsupported file: " << path << endl;
    }
}
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: build_lexicon <folder_or_file_path>\n";
        return 1;
    }

    // <-- Get path from command-line argument instead of hardcoding
    std::string path = argv[1];

    unordered_map<string, int> lexicon;

    cout << "Starting...\n";

    readAllFiles(path, lexicon);

    cout << "\nFinished! Total unique words in lexicon: " << lexicon.size() << "\n";

    // -------------------- Save Lexicon JSON --------------------
    json lexJson;
    lexJson["lexicon"] = json::array();

    // only export words (not counts)
    for (auto& p : lexicon)
        lexJson["lexicon"].push_back(p.first);

    // write lexicon.json file
    std::ofstream out("lexicon.json");
    out << lexJson.dump(4);
    out.close();

    cout << "\n✓ Lexicon saved to lexicon.json\n";

   

    return 0;
}


