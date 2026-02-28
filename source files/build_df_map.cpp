#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include "nlohmann/json.hpp" // For reading barrels and writing the final map

using json = nlohmann::json;

// LexID (int) -> Document Frequency (int)
using DFMap = std::unordered_map<int, int>; 

// --- Existing function placeholder (You need to adapt this from main.cpp) ---
json loadBarrel(const std::string& barrelsDir, int barrelID) {
    std::string path = barrelsDir + "/barrel_" + std::to_string(barrelID) + ".json";
    std::ifstream fin(path);
    if(!fin) {
        std::cerr << "ERROR: Cannot open barrel file " << path << "\n";
        // Do not exit here, just return an empty object to continue to the next barrel
        return json::object(); 
    }
    json barrel;
    fin >> barrel;
    fin.close();
    return barrel;
}
// ----------------------------------------------------------------------------


DFMap generateDFMap(const std::string& barrelsDir, int totalBarrels) {
    DFMap dfMap;

    std::cout << "Starting DF Map generation by scanning " << totalBarrels << " barrels...\n";

    for (int barrelID = 1; barrelID <= totalBarrels; ++barrelID) {
        std::cout << "Processing Barrel " << barrelID << "...\n";
        json barrel = loadBarrel(barrelsDir, barrelID);

        // Iterate through all LexIDs stored in this barrel
        // The structure inside the barrel is: { "LexID_str": { "DocID_str": freq, ... }, ... }
        for (auto& [lexID_str, postings] : barrel.items()) {
            try {
                int lexID = std::stoi(lexID_str);
                
                // The Document Frequency (DF) is simply the number of entries
                // in the posting list for this LexID.
                int df = postings.size();
                
                // Store the result
                dfMap[lexID] = df; 
                
            } catch (const std::exception& e) {
                std::cerr << "Warning: Skipping invalid entry in barrel " << barrelID << ": " << e.what() << "\n";
            }
        }
    }
    return dfMap;
}

void saveDFMap(const std::string& dfFile, const DFMap& dfMap) {
    json dfJson;
    
    // Convert the C++ map to a JSON object (LexID -> DF)
    for (const auto& pair : dfMap) {
        // Convert LexID int to string key for JSON serialization
        dfJson[std::to_string(pair.first)] = pair.second;
    }

    std::ofstream fout(dfFile);
    if(!fout) {
        std::cerr << "FATAL: Could not open output file for DF Map: " << dfFile << "\n";
        exit(1);
    }
    
    // Write the JSON data with a human-readable indentation
    fout << dfJson.dump(4);
    fout.close();
    
    std::cout << "SUCCESS: DF Map saved to " << dfFile << " with " << dfMap.size() << " entries.\n";
}

// int main(int argc, char* argv[]) {
//     // NOTE: This program requires 3 command-line arguments:
//     // 1. barrels_directory (e.g., "../barrels")
//     // 2. total_barrels_count (e.g., "5")
//     // 3. output_df_map.json (e.g., "../df_map.json")
//     if (argc < 4) { 
//         std::cerr << "Usage: ./build_df_map <barrels_directory> <total_barrels_count> <output_df_map.json>\n";
//         std::cerr << "Example: ./build_df_map ../barrels 5 ../df_map.json\n";
//         return 1;
//     }

//     std::string barrelsDir = argv[1];
//     int totalBarrels;
    
//     try {
//         // Safely convert the second argument (count) to an integer
//         totalBarrels = std::stoi(argv[2]);
//     } catch (const std::exception& e) {
//         std::cerr << "ERROR: Invalid number provided for total_barrels_count.\n";
//         return 1;
//     }
    
//     std::string dfFile = argv[3];

//     std::cout << "--- Starting Document Frequency Map Builder ---\n";
//     std::cout << "Input Barrels Dir: " << barrelsDir << " (" << totalBarrels << " total)\n";
//     std::cout << "Output File: " << dfFile << "\n";
    
//     // 1. Generate the map
//     DFMap dfMap = generateDFMap(barrelsDir, totalBarrels);

//     // 2. Save the map
//     if (!dfMap.empty()) {
//         saveDFMap(dfFile, dfMap);
//     } else {
//         std::cerr << "Warning: DF Map is empty. Ensure barrel files contain data.\n";
//     }

//     return 0;
// }