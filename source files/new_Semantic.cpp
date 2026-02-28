#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <filesystem>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// -------------------- TYPES --------------------
using PostingList = std::unordered_map<int,int>;   // docID -> frequency
using DFMap       = std::unordered_map<int,int>;   // lexID -> df
using ScoreMap    = std::unordered_map<int,float>; // docID -> score

struct SearchResult {
    int docID;
    float score;
};

const int TOTAL_DOCUMENTS = 50000;

// -------------------- TOKENIZER --------------------
std::vector<std::string> tokenize(const std::string& q) {
    std::stringstream ss(q);
    std::vector<std::string> tokens;
    std::string t;
    while (ss >> t) {
        std::transform(t.begin(), t.end(), t.begin(), ::tolower);
        tokens.push_back(t);
    }
    return tokens;
}

// -------------------- BARREL LOGIC --------------------
int getBarrelID(const std::string& word) {
    char c = tolower(word[0]);
    int alphaBucket;

    if (c >= 'a' && c <= 'c') alphaBucket = 0;
    else if (c >= 'd' && c <= 'f') alphaBucket = 1;
    else if (c >= 'g' && c <= 'i') alphaBucket = 2;
    else if (c >= 'j' && c <= 'l') alphaBucket = 3;
    else if (c >= 'm' && c <= 'o') alphaBucket = 4;
    else if (c >= 'p' && c <= 'r') alphaBucket = 5;
    else if (c >= 's' && c <= 'u') alphaBucket = 6;
    else alphaBucket = 7;

    int subBucket = std::hash<std::string>{}(word) % 4;

    return alphaBucket * 4 + subBucket; // 0-31
}

// -------------------- LOAD LEXICON --------------------
// -------------------- Updated Load Lexicon --------------------
std::unordered_map<std::string, int> loadLexicon(const std::string& lexFile) {
    std::ifstream fin(lexFile);
    if (!fin) {
        std::cerr << "ERROR: Cannot open lexicon file\n";
        exit(1);
    }

    json lexJson;
    fin >> lexJson;
    fin.close();

    std::unordered_map<std::string, int> lexicon;

    // Check if the JSON is an object or a specialized structure
    auto& items = lexJson.contains("lexicon") ? lexJson["lexicon"] : lexJson;

    for (auto& [word, val] : items.items()) {
        // Fix: Handle both {"word": 1} and {"word": [1]}
        if (val.is_array() && !val.empty()) {
            lexicon[word] = val[0].get<int>();
        } else if (val.is_number()) {
            lexicon[word] = val.get<int>();
        } else {
            // If it's just a list of words, use the index as ID
            static int autoID = 1;
            lexicon[val.get<std::string>()] = autoID++;
        }
    }
    return lexicon;
}

// -------------------- Updated Load Barrel Mapping --------------------
std::unordered_map<int, int> loadBarrelMap(const std::string& mapFile) {
    std::ifstream fin(mapFile);
    if (!fin) {
        std::cerr << "ERROR: Cannot open barrel map file\n";
        exit(1);
    }

    json mapJson;
    fin >> mapJson;
    fin.close();

    std::unordered_map<int, int> barrelMap;

    for (auto& [lexID, barrelIDVal] : mapJson.items()) {
        int lID = std::stoi(lexID);
        // Fix: If barrelID is [5], extract the 5.
        if (barrelIDVal.is_array() && !barrelIDVal.empty()) {
            barrelMap[lID] = barrelIDVal[0].get<int>();
        } else {
            barrelMap[lID] = barrelIDVal.get<int>();
        }
    }
    return barrelMap;
}
// -------------------- LOAD DF MAP --------------------
DFMap loadDFMap(const std::string& file) {
    std::ifstream fin(file);
    json j; fin >> j;

    DFMap df;
    for (auto& [k,v] : j.items())
        df[std::stoi(k)] = v;
    return df;
}

// -------------------- LOAD BARREL --------------------
json loadBarrel(const std::string& dir, int id) {
    std::ifstream fin(dir + "/barrel_" + std::to_string(id) + ".json");
    json j; fin >> j;
    return j;
}

// -------------------- GET POSTINGS --------------------
PostingList getPostings(const std::string& word,
                        const std::unordered_map<std::string,int>& lex,
                        const std::unordered_map<int,int>& barrelMap,
                        const std::string& barrelDir)
{
    PostingList list;
    if (!lex.count(word)) return list;

    int lexID = lex.at(word);
    if (!barrelMap.count(lexID)) return list;

    json barrel = loadBarrel(barrelDir, barrelMap.at(lexID));
    std::string key = std::to_string(lexID);

    if (!barrel.contains(key)) return list;

    for (auto& [doc, freq] : barrel[key].items())
        list[std::stoi(doc)] = freq;

    return list;
}

// -------------------- MERGE POSTINGS --------------------
PostingList intersect(const PostingList& A, const PostingList& B) {
    PostingList R;
    const PostingList *small = &A, *large = &B;
    if (A.size() > B.size()) std::swap(small, large);

    for (auto& [doc,f] : *small)
        if (large->count(doc))
            R[doc] = f + large->at(doc);

    return R;
}

// -------------------- TF-IDF --------------------
float tfidfScore(int ttf,
                 const std::vector<std::string>& words,
                 const std::unordered_map<std::string,int>& lex,
                 const DFMap& df)
{
    float score = 0.0f;
    for (const auto& w : words) {
        int id = lex.at(w);
        if (df.count(id)) {
            float idf = std::log((float)TOTAL_DOCUMENTS / (1.0f + df.at(id)));
            score += ttf * idf;
        }
    }
    return score;
}

// -------------------- SEMANTIC BOOST --------------------
float semanticBoost(const PostingList& docTerms,
                    const std::vector<std::string>& words,
                    const std::unordered_map<std::string,int>& lex)
{
    int count = 0;
    for (const auto& w : words) {
        if (lex.count(w) && docTerms.count(lex.at(w))) count++;
    }
    return (float)count / (float)words.size();
}

// -------------------- SEARCH --------------------
void search(const std::string& query,
            const std::unordered_map<std::string,int>& lex,
            const std::unordered_map<int,int>& barrelMap,
            const DFMap& df,
            const std::string& barrelDir)
{
    auto words = tokenize(query);

    PostingList result = getPostings(words[0], lex, barrelMap, barrelDir);
    if (result.empty()) { std::cout << "No results\n"; return; }

    for (size_t i=1;i<words.size();++i) {
        result = intersect(result, getPostings(words[i], lex, barrelMap, barrelDir));
        if (result.empty()) { std::cout << "No results\n"; return; }
    }

    std::vector<SearchResult> ranked;
    const float SEMANTIC_WEIGHT = 0.35f;

    for (auto& [doc, ttf] : result) {
        float baseScore = tfidfScore(ttf, words, lex, df);

        PostingList docTerms;
        for (auto& w : words) {
            if (lex.count(w)) {
                int lexID = lex.at(w);
                docTerms[lexID] = getPostings(w, lex, barrelMap, barrelDir)[doc];
            }
        }

        float semantic = semanticBoost(docTerms, words, lex);
        float finalScore = baseScore + SEMANTIC_WEIGHT * semantic;
        ranked.push_back({doc, finalScore});
    }

    std::sort(ranked.begin(), ranked.end(),
              [](auto&a, auto&b){ return a.score > b.score; });

    std::cout << "\n=== RESULTS ===\n";
    for (size_t i=0;i<std::min(ranked.size(), size_t(10));++i)
        std::cout << i+1 << ". Doc " << ranked[i].docID
                  << " Score: " << ranked[i].score << "\n";
}

// -------------------- DYNAMIC ADDITION --------------------
void addDocument(int docID,
                 const std::string& content,
                 std::unordered_map<std::string,int>& lex,
                 DFMap& df,
                 std::unordered_map<int,int>& barrelMap,
                 const std::string& barrelDir)
{
    auto words = tokenize(content);
    std::unordered_map<int,int> termFreq;

    for (auto& w : words) {
        if (!lex.count(w)) {
            int newID = lex.size() + 1;
            lex[w] = newID;
            df[newID] = 1;
            barrelMap[newID] = getBarrelID(w);
        } else {
            int id = lex[w];
            if (!termFreq.count(id)) df[id] += 1;
        }
        termFreq[lex[w]]++;
    }

    std::unordered_map<int,json> barrelsToUpdate;
    for (auto& [lexID, freq] : termFreq) {
        int barrelID = barrelMap[lexID];
        std::string barrelFile = barrelDir + "/barrel_" + std::to_string(barrelID) + ".json";

        if (!barrelsToUpdate.count(barrelID)) {
            if (fs::exists(barrelFile)) {
                std::ifstream fin(barrelFile);
                fin >> barrelsToUpdate[barrelID];
                fin.close();
            }
        }
        barrelsToUpdate[barrelID][std::to_string(lexID)][std::to_string(docID)] = freq;
    }

    for (auto& [barrelID, barrelJson] : barrelsToUpdate) {
        std::ofstream fout(barrelDir + "/barrel_" + std::to_string(barrelID) + ".json");
        fout << std::setw(2) << barrelJson << std::endl;
    }

    {
        std::ofstream fout(barrelDir + "/barrelMap.json"); json jMap(barrelMap); fout << std::setw(2) << jMap << std::endl;
    }
    {
        std::ofstream fout(barrelDir + "/df.json"); json jDF(df); fout << std::setw(2) << jDF << std::endl;
    }
    {
        std::ofstream fout(barrelDir + "/lexicon.json"); json jLex(lex); fout << std::setw(2) << jLex << std::endl;
    }

    std::cout << "Document " << docID << " added successfully!\n";
}

std::vector<SearchResult> run_search(
    const std::string& query,
    const std::unordered_map<std::string,int>& lex,
    const std::unordered_map<int,int>& barrelMap,
    const DFMap& df,
    const std::string& barrelDir)
{
    auto words = tokenize(query);

    PostingList result = getPostings(words[0], lex, barrelMap, barrelDir);
    if (result.empty()) return {};

    for (size_t i=1;i<words.size();++i) {
        result = intersect(result, getPostings(words[i], lex, barrelMap, barrelDir));
        if (result.empty()) return {};
    }

    std::vector<SearchResult> ranked;
    const float SEMANTIC_WEIGHT = 0.35f;

    for (auto& [doc, ttf] : result) {
        float baseScore = tfidfScore(ttf, words, lex, df);

        PostingList docTerms;
        for (auto& w : words) {
            if (lex.count(w)) {
                int lexID = lex.at(w);
                docTerms[lexID] = getPostings(w, lex, barrelMap, barrelDir)[doc];
            }
        }

        float semantic = semanticBoost(docTerms, words, lex);
        float finalScore = baseScore + SEMANTIC_WEIGHT * semantic;

        ranked.push_back({doc, finalScore});
    }

    std::sort(ranked.begin(), ranked.end(),
              [](auto&a, auto&b){ return a.score > b.score; });

    if (ranked.size() > 10)
        ranked.resize(10);

    return ranked;
}

// // -------------------- MAIN --------------------
// int main(int argc, char* argv[]) {
//     if (argc < 5) {
//         std::cout << "Usage:\nSearchEngine lexicon.json barrel_map.json df_map.json barrels_dir\n";
//         return 1;
//     }

//     auto lex  = loadLexicon(argv[1]);
//     auto map  = loadBarrelMap(argv[2]);
//     auto df   = loadDFMap(argv[3]);
//     std::string barrels_dir = argv[4];

//     int lastDocID = TOTAL_DOCUMENTS;

//     while (true) {
//         std::cout << "\nEnter command (add \"document\" / search \"query\" / exit): ";
//         std::string line;
//         std::getline(std::cin, line);

//         if (line.substr(0,4) == "add ") {
//             std::string content = line.substr(4);
//             addDocument(++lastDocID, content, lex, df, map, barrels_dir);
//         } else if (line.substr(0,7) == "search ") {
//             std::string query = line.substr(7);
//             auto t1 = std::chrono::high_resolution_clock::now();
//             search(query, lex, map, df, barrels_dir);
//             auto t2 = std::chrono::high_resolution_clock::now();
//             std::cout << "\nTime: "
//                       << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count()
//                       << " ms\n";
//         } else if (line == "exit") break;
//         else std::cout << "Unknown command.\n";
//     }
// }
