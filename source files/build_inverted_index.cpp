#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <filesystem>
#include <regex>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// -------------------- File Filter --------------------
bool isReadableFile(const fs::path& p) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".txt" || ext == ".json" || ext == ".csv" ||
           ext == ".xml" || ext == ".html" || ext == ".md" ||
           ext == ".log" || ext == ".tsv" || ext == ".yaml" ||
           ext == ".ini" || ext == ".cfg";
}

// -------------------- Tokenizer --------------------
void tokenize(const std::string& text, std::unordered_map<std::string,int>& termFreq) {
    static const std::regex wordRegex("[A-Za-z0-9]+");
    auto begin = std::sregex_iterator(text.begin(), text.end(), wordRegex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string word = it->str();
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        termFreq[word]++;
    }
}

// int main(int argc, char* argv[]) {
//     if (argc < 4) {
//         std::cout << "Usage: build_inverted_index <dataset_folder> <lexicon_json> <output_json>\n";
//         return 1;
//     }

//     std::string datasetDir = argv[1];
//     std::string lexiconFile = argv[2];
//     std::string outputFile = argv[3];

//     // -------------------- Load Lexicon --------------------
//     std::ifstream lexIn(lexiconFile);
//     if (!lexIn) { std::cerr << "ERROR: Cannot open lexicon file\n"; return 1; }
//     json lexJson; lexIn >> lexJson;

//     std::unordered_map<std::string,int> lexiconMap;
//     int id = 1;
//     for (const auto& w : lexJson["lexicon"]) lexiconMap[w.get<std::string>()] = id++;

//     std::cout << "Loaded lexicon size: " << lexiconMap.size() << "\n";

//     // -------------------- Build Inverted Index --------------------
//     std::unordered_map<int, std::unordered_map<int,int>> invertedIndex;
//     int docID = 0;

//     std::vector<fs::path> files;
//     for (auto& entry : fs::recursive_directory_iterator(datasetDir)) {
//         if (!fs::is_regular_file(entry.path()) || !isReadableFile(entry.path()))
//             continue;

//           if (fs::is_regular_file(entry.path()) && isReadableFile(entry.path()))
//         files.push_back(entry.path());

//     // Sort files alphabetically for deterministic docID assignment
//     std::sort(files.begin(), files.end());
//         docID++;
//         std::ifstream fin(entry.path());
//         if (!fin) continue;

//         std::string content((std::istreambuf_iterator<char>(fin)),
//                              std::istreambuf_iterator<char>());
//         fin.close();

//         std::unordered_map<std::string,int> localTF;
//         tokenize(content, localTF);

//         for (auto& p : localTF) {
//             const std::string& word = p.first;
//             int freq = p.second;
//             if (lexiconMap.count(word)) {
//                 int lexID = lexiconMap[word];
//                 invertedIndex[lexID][docID] = freq;
//             }
//         }

//         std::cout << "Processed: " << entry.path().string() << "\n";
//     }

//     // -------------------- Save Inverted Index JSON --------------------
//     json outJson;
//     for (auto& p : invertedIndex) {
//         int termID = p.first;
//         outJson[std::to_string(termID)] = json::object();
//         for (auto& doc : p.second) {
//             outJson[std::to_string(termID)][std::to_string(doc.first)] = doc.second;
//         }
//     }

//     std::ofstream out(outputFile);
//     if (!out) { std::cerr << "ERROR: Cannot open output file\n"; return 1; }
//     out << outJson.dump(4);
//     out.close();

//     std::cout << "\n✓ Inverted index built successfully.\n";
//     std::cout << "✓ Terms indexed: " << invertedIndex.size() << "\n";
//     std::cout << "✓ Output: " << outputFile << "\n";

//     return 0;
// }