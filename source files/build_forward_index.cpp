#include <iostream>
#include <fstream>
#include <unordered_map>
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
void tokenize(const std::string& text,
              std::unordered_map<std::string,int>& termFreq)
{
    //This creates an expression and ignore characters like @? and others
    static const std::regex wordRegex("[A-Za-z0-9]+");


    auto begin = std::sregex_iterator(text.begin(), text.end(), wordRegex);
    auto end   = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string word = it->str();
        //Word is lowercased
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        termFreq[word]++;
    }
}

int main(int argc, char* argv[]) {

    if (argc < 4) {
        std::cout << "Usage: build_forward_index <dataset_folder> <lexicon_json> <output_json>\n";
        return 1;
    }

    std::string datasetDir  = argv[1];
    std::string lexiconFile = argv[2];
    std::string outputFile  = argv[3];

    // -------------------- Load Lexicon JSON --------------------
    std::ifstream lexIn(lexiconFile);
    json lexJson;
    lexIn >> lexJson;

    std::unordered_map<std::string,int> lexiconMap;

    int id = 1;
    for (const auto& w : lexJson["lexicon"]) {
        lexiconMap[w.get<std::string>()] = id++;
    }

    std::cout << "Loaded lexicon size: " << lexiconMap.size() << "\n";

    // -------------------- Prepare Forward Index JSON --------------------
    json forwardIndex;
    forwardIndex["documents"] = json::array();

    int docID = 0;

    std::vector<fs::path> files;

    // -------------------- Traverse dataset --------------------
    for (auto& entry : fs::recursive_directory_iterator(datasetDir)) {

        if (!fs::is_regular_file(entry.path()) || !isReadableFile(entry.path()))
            continue;
        if (fs::is_regular_file(entry.path()) && isReadableFile(entry.path()))
        files.push_back(entry.path());

    // Sort files alphabetically for deterministic docID assignment
    std::sort(files.begin(), files.end());
        docID++;

        std::ifstream fin(entry.path());
        if (!fin) continue;

        std::string content(
            (std::istreambuf_iterator<char>(fin)),
             std::istreambuf_iterator<char>()
        );
        fin.close();

        // -------------------- Local term frequency --------------------
        std::unordered_map<std::string,int> localTF;
        tokenize(content, localTF);

        // -------------------- Convert words → lexicon IDs --------------------
        json termsObj = json::object();

        for (auto& p : localTF) {
            const std::string& word = p.first;
            int freq = p.second;

            if (lexiconMap.count(word)) {
                int lexID = lexiconMap[word];
                termsObj[std::to_string(lexID)] = freq;
            }
        }

        // -------------------- Add document entry --------------------
        json d;
        d["doc_id"] = docID;
        d["file"]   = entry.path().string();
        d["terms"]  = termsObj;

        forwardIndex["documents"].push_back(d);

        std::cout << "Indexed: " << entry.path().string() << "\n";
    }

    // -------------------- Save Forward Index JSON --------------------
    std::ofstream out(outputFile);
    out << forwardIndex.dump(4);
    out.close();

    std::cout << "\n✓ Forward index built successfully.\n";
    std::cout << "✓ Documents indexed: " << docID << "\n";
    std::cout << "✓ Output: " << outputFile << "\n";

    return 0;
}
