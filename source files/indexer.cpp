#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include "nlohmann.json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// --- CONFIGURATION ---
const std::string DATA_DIR = "data";       // Put your raw .txt files here
const std::string BARRELS_DIR = "barrels"; // Output folder
const int TOTAL_BARRELS = 32;              // As defined by your alpha*sub logic

// --- GLOBAL STRUCTURES ---
// lexicon: word -> lexID
std::unordered_map<std::string, int> lexicon; 
// invertedIndex: lexID -> { docID -> frequency }
std::unordered_map<int, std::unordered_map<int, int>> invertedIndex; 

int nextLexID = 1;

// ---------------------------------------------------------
// LOGIC 1: TOKENIZATION (Standardizing Input)
// ---------------------------------------------------------
std::string cleanWord(std::string word) {
    std::string clean;
    for (char c : word) {
        if (std::isalnum(c)) clean += std::tolower(c);
    }
    return clean;
}

// ---------------------------------------------------------
// LOGIC 2: YOUR BARREL MAPPING ALGORITHM
// ---------------------------------------------------------
int getBarrelID(const std::string& word) {
    if (word.empty()) return 0;
    
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
    else alphaBucket = 7; // v-z, and symbols fallback

    // 4 sub-buckets per alphabetical bucket
    int subBucket = std::hash<std::string>{}(word) % 4;

    // Global barrel ID (0–31)
    return alphaBucket * 4 + subBucket;
}

// ---------------------------------------------------------
// LOGIC 3: DYNAMIC INDEXING (Processing Files)
// ---------------------------------------------------------
void processFile(const fs::path& filepath, int docID) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open " << filepath << "\n";
        return;
    }

    std::string rawWord;
    while (file >> rawWord) {
        std::string word = cleanWord(rawWord);
        if (word.empty()) continue;

        // 1. Dynamic Lexicon Insertion
        if (lexicon.find(word) == lexicon.end()) {
            lexicon[word] = nextLexID++;
        }
        int lexID = lexicon[word];

        // 2. Build Inverted Index in RAM
        invertedIndex[lexID][docID]++;
    }
    file.close();
}

// ---------------------------------------------------------
// LOGIC 4: SYSTEM ARCHITECT (Saving the System)
// ---------------------------------------------------------
void saveSystem() {
    std::cout << "[Saver] Generating system files...\n";

    // --- A. Save Lexicon (Format: {"lexicon": ["word", ...]}) ---
    json lexJson;
    std::vector<std::string> lexVector(lexicon.size());
    // Convert map to vector where index matches ID (ID is 1-based)
    for(const auto& [word, id] : lexicon) {
        if(id - 1 < lexVector.size()) {
            lexVector[id - 1] = word;
        }
    }
    lexJson["lexicon"] = lexVector;

    std::ofstream lexOut("lexicon.json");
    lexOut << lexJson.dump(4);
    lexOut.close();
    std::cout << "✓ Saved lexicon.json (" << lexicon.size() << " terms)\n";

    // --- B. Generate & Save Barrel Mapping (Format: {"lexID": barrelID}) ---
    json mapJson;
    std::unordered_map<int, int> idToBarrel;

    for(const auto& [word, id] : lexicon) {
        int bid = getBarrelID(word);
        idToBarrel[id] = bid;
        mapJson[std::to_string(id)] = bid;
    }

    std::ofstream mapOut("map.json");
    mapOut << mapJson.dump(4);
    mapOut.close();
    std::cout << "✓ Saved map.json\n";

    // --- C. Build & Save Barrels (Format: {"lexID": {"docID": freq}}) ---
    if (!fs::exists(BARRELS_DIR)) fs::create_directory(BARRELS_DIR);

    // Initialize 32 empty JSON objects
    std::vector<json> barrels(TOTAL_BARRELS);

    // Distribute Inverted Index into Barrels
    for (const auto& [lexID, docMap] : invertedIndex) {
        int barrelID = idToBarrel[lexID]; // Use the mapping we just calculated
        
        // Convert internal int-map to json object
        json docListJson;
        for(const auto& [docID, freq] : docMap) {
            docListJson[std::to_string(docID)] = freq;
        }

        // Store: barrels[ID]["lexID"] = { "docID": freq }
        barrels[barrelID][std::to_string(lexID)] = docListJson;
    }

    // Write to disk
    int savedCount = 0;
    for (int i = 0; i < TOTAL_BARRELS; ++i) {
        if (barrels[i].empty()) continue; // Optional: skip empty barrels

        std::string fname = BARRELS_DIR + "/barrel_" + std::to_string(i) + ".json";
        std::ofstream bOut(fname);
        bOut << barrels[i].dump(4);
        bOut.close();
        savedCount++;
    }
    std::cout << "✓ Saved " << savedCount << " barrel files in '" << BARRELS_DIR << "/'\n";
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main() {
    std::cout << "=== MEMBER 2: SYSTEM ARCHITECT ENGINE ===\n";

    // 1. Check Data Directory
    if (!fs::exists(DATA_DIR)) {
        fs::create_directory(DATA_DIR);
        std::cerr << "Created '" << DATA_DIR << "' folder. Please add .txt files and run again.\n";
        return 1;
    }

    // 2. Read All Files (Dynamic Indexing)
    int docCount = 0;
    for (const auto& entry : fs::directory_iterator(DATA_DIR)) {
        if (entry.path().extension() == ".txt") {
            docCount++;
            std::cout << "Indexing Doc " << docCount << ": " << entry.path().filename() << "\n";
            processFile(entry.path(), docCount);
        }
    }

    if (docCount == 0) {
        std::cout << "No .txt files found in '" << DATA_DIR << "'.\n";
        return 0;
    }

    // 3. Save All Components
    saveSystem();

    std::cout << "=== Indexing Complete. Ready for Search. ===\n";
    return 0;
}