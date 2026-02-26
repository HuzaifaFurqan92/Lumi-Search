#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <algorithm>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// -------------------- Barrel Mapping Function --------------------
int getBarrelID(const std::string& word) {
    char c = tolower(word[0]);
    int alphaBucket;

    // 8 primary alphabetical buckets: A–C, D–F, G–I, J–L, M–O, P–R, S–U, V–Z
    if (c >= 'a' && c <= 'c') alphaBucket = 0;
    else if (c >= 'd' && c <= 'f') alphaBucket = 1;
    else if (c >= 'g' && c <= 'i') alphaBucket = 2;
    else if (c >= 'j' && c <= 'l') alphaBucket = 3;
    else if (c >= 'm' && c <= 'o') alphaBucket = 4;
    else if (c >= 'p' && c <= 'r') alphaBucket = 5;
    else if (c >= 's' && c <= 'u') alphaBucket = 6;
    else alphaBucket = 7; // v-z

    // 4 sub-buckets per alphabetical bucket
    int subBucket = std::hash<std::string>{}(word) % 4;

    // Global barrel ID (0–31)
    return alphaBucket * 4 + subBucket;
}

// -------------------- Load Lexicon --------------------
std::unordered_map<std::string,int> loadLexicon(const std::string& lexFile) {
    std::ifstream fin(lexFile);
    if (!fin) {
        std::cerr << "ERROR: Cannot open lexicon file\n";
        exit(1);
    }
    json lexJson;
    fin >> lexJson;
    fin.close();

    std::unordered_map<std::string,int> lexMap;
    int id = 1;
    for (const auto& w : lexJson["lexicon"]) {
        lexMap[w.get<std::string>()] = id++;
    }
    return lexMap;
}

// -------------------- Generate Barrel Mapping --------------------
std::unordered_map<int,int> generateBarrelMapping(const std::unordered_map<std::string,int>& lexMap) {
    std::unordered_map<int,int> barrelMap; // lexID -> barrelID

    for (const auto& p : lexMap) {
        const std::string& word = p.first;
        int lexID = p.second;
        int barrelID = getBarrelID(word);
        barrelMap[lexID] = barrelID;
    }
    return barrelMap;
}

// -------------------- Save Barrel Mapping --------------------
void saveBarrelMapping(const std::unordered_map<int,int>& barrelMap, const std::string& outFile) {
    json outJson;
    for (const auto& p : barrelMap) {
        outJson[std::to_string(p.first)] = p.second;
    }
    std::ofstream fout(outFile);
    if (!fout) { std::cerr << "ERROR: Cannot open output file\n"; exit(1); }
    fout << outJson.dump(4);
    fout.close();
    std::cout << "✓ Barrel mapping saved: " << outFile << "\n";
}

// -------------------- Main --------------------
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: member1_barrel_mapping <lexicon_json> <barrel_mapping_json>\n";
        return 1;
    }

    std::string lexFile = argv[1];
    std::string outFile = argv[2];

    // Load lexicon
    auto lexMap = loadLexicon(lexFile);
    std::cout << "Loaded lexicon size: " << lexMap.size() << "\n";

    // Generate barrel mapping
    auto barrelMap = generateBarrelMapping(lexMap);

    // Save mapping
    saveBarrelMapping(barrelMap, outFile);

    // Simple test
    std::string testWord = "virus";
    int testID = lexMap[testWord];
    int barrelID = barrelMap[testID];
    std::cout << "Test word: '" << testWord << "' -> LexID: " << testID
              << " -> BarrelID: " << barrelID << "\n";

    return 0;
}
