#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include "nlohmann/json.hpp"
#include <chrono> // Required for timing

using json = nlohmann::json;


// ----------------------------------------------------
// Load lexicon: word → lexID
// ----------------------------------------------------
std::unordered_map<std::string, int> loadLexicon(const std::string& lexFile) {
    std::ifstream fin(lexFile);
    if(!fin) {
        std::cerr << "ERROR: Cannot open lexicon file\n";
        exit(1);
    }

    json lexJson;
    fin >> lexJson;
    fin.close();

    std::unordered_map<std::string, int> lexMap;
    int id = 1;

    for (const auto& w : lexJson["lexicon"]) {
        lexMap[w.get<std::string>()] = id++;
    }
    
    return lexMap;
}

// ----------------------------------------------------
// Load barrel mapping: lexID → barrelID
// ----------------------------------------------------
std::unordered_map<int, int> loadBarrelMapping(const std::string& mapFile) {
    std::ifstream fin(mapFile);
    if(!fin) {
        std::cerr << "ERROR: Cannot open barrel mapping file\n";
        exit(1);
    }

    json mapJson;
    fin >> mapJson;
    fin.close();

    std::unordered_map<int, int> barrelMap;
    for (auto& [lexID, barrelID] : mapJson.items())
        barrelMap[std::stoi(lexID)] = barrelID;

    return barrelMap;
}

// ----------------------------------------------------
// Load only ONE required barrel file
// ----------------------------------------------------
json loadBarrel(const std::string& barrelsDir, int barrelID) {
    std::string path = barrelsDir + "/barrel_" + std::to_string(barrelID) + ".json";

    std::ifstream fin(path);
    if(!fin) {
        std::cerr << "ERROR: Cannot open barrel file " << path << "\n";
        exit(1);
    }

    json barrel;
    fin >> barrel;
    fin.close();
    return barrel;
}

// ----------------------------------------------------
// Search for a word
// ----------------------------------------------------
void searchWord(
    const std::string& query,
    const std::unordered_map<std::string, int>& lexMap,
    const std::unordered_map<int, int>& barrelMap,
    const std::string& barrelsDir)
{
    // STEP 1: map word → lexID
    auto it = lexMap.find(query);
    if (it == lexMap.end()) {
        std::cout << "No results found. Word not in lexicon.\n";
        return;
    }

    // it->second holds the lexicon ID of the word and it->first is the word itself
    int lexID = it->second;

    // STEP 2: get lexID → barrelID
    int barrelID = barrelMap.at(lexID);

    std::cout << "[DEBUG] Word '" << query << "' maps to:\n";
    std::cout << " - LexID: " << lexID << "\n";
    std::cout << " - Barrel: " << barrelID << "\n";

    // STEP 3: load only that barrel
    json barrel = loadBarrel(barrelsDir, barrelID);

    // STEP 4: find posting list
    std::string lexIDstr = std::to_string(lexID);

    if (!barrel.contains(lexIDstr)) {
        std::cout << "Word exists in lexicon but has no postings.\n";
        return;
    }

    json postingList = barrel[lexIDstr];

    std::cout << "\n=== RESULTS ===\n";
    for (auto& [docID, freq] : postingList.items()) {
        std::cout << "Doc " << docID << " (freq: " << freq << ")\n";
    }
}

// ----------------------------------------------------
// MAIN
// ----------------------------------------------------

int main(int argc, char* argv[]) {

    if (argc < 5) {
        std::cout << "Usage: search <word> <lexicon.json> <barrel_mapping.json> <barrels_directory>\n";
        return 1;
    }

    std::string query      = argv[1];
    std::string lexFile    = argv[2];
    std::string mapFile    = argv[3];
    std::string barrelsDir = argv[4];

    // Load static data first
    std::cout << "Loading lexicon...\n";
    auto lexMap    = loadLexicon(lexFile);
    std::cout << "Loading barrel mapping...\n";
    auto barrelMap = loadBarrelMapping(mapFile);
    std::cout << "Data loaded.\n";
    
    // --- Timing the searchWord function ---
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::microseconds;
    
    // Record start time
    auto t1 = high_resolution_clock::now();
    
    // Perform the search
    searchWord(query, lexMap, barrelMap, barrelsDir);
    
    // Record end time
    auto t2 = high_resolution_clock::now();
    
    // Calculate the duration
    auto ms_int = duration_cast<microseconds>(t2 - t1);
    
    // Print the elapsed time
    std::cout << "\nTime taken for search: " 
              << ms_int.count() 
              << " microseconds\n";
    // -------------------------------------

    return 0;
}