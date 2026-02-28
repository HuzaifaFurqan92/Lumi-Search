#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// -------------------- Updated Load Lexicon --------------------
std::unordered_map<std::string, int>
loadLexicon(const std::string& lexFile)
{
    std::ifstream fin(lexFile);
    if (!fin) {
        std::cerr << "ERROR: Cannot open lexicon file\n";
        exit(1);
    }

    json lexJson;
    fin >> lexJson;
    fin.close();

    std::unordered_map<std::string, int> lexicon;

    if (!lexJson.contains("lexicon") ||
        !lexJson["lexicon"].is_array())
    {
        std::cerr << "ERROR: Invalid lexicon format\n";
        exit(1);
    }

    auto& arr = lexJson["lexicon"];

    for (size_t i = 0; i < arr.size(); ++i) {
        if (!arr[i].is_string()) continue;

        std::string word = arr[i].get<std::string>();

        // IMPORTANT: choose ONE (see below)
        lexicon[word] = static_cast<int>(i);     // 0-based
        // lexicon[word] = static_cast<int>(i+1); // 1-based
    }

    return lexicon;
}


// -------------------- Updated Load Barrel Mapping --------------------
std::unordered_map<int, int> loadBarrelMapping(const std::string& mapFile) {
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


// -------------------- Load Single Barrel --------------------
json loadBarrelFile(
        const std::string& barrelDir,
        int barrelID)
{
    std::string fname =
        barrelDir + "/barrel_" + std::to_string(barrelID) + ".json";

    std::ifstream fin(fname);

    if(!fin){
        std::cerr << "ERROR: Cannot open barrel file: "
                  << fname << "\n";
        exit(1);
    }

    json barrelJson;
    fin >> barrelJson;
    fin.close();

    return barrelJson;
}


// -------------------- Normalize Query Word --------------------
std::string normalize(const std::string& w)
{
    std::string s = w;

    std::transform(s.begin(), s.end(),
                   s.begin(), ::tolower);

    return s;
}


// -------------------- SINGLE WORD SEARCH --------------------
void singleWordSearch(
    const std::string& query,
    const std::unordered_map<std::string,int>& lexicon,
    const std::unordered_map<int,int>& barrelMap,
    const std::string& barrelDir)
{
    std::string keyword = normalize(query);

    // Step 1: Find word ID
    auto it = lexicon.find(keyword);

    if(it == lexicon.end()){
        std::cout << "❌ Word not found in lexicon\n";
        return;
    }

    int wordID = it->second;

    std::cout << "✓ Word: \"" << keyword << "\"\n";
    std::cout << "✓ Word ID: " << wordID << "\n";

    // Step 2: Find barrel containing this word
    auto bIt = barrelMap.find(wordID);

    if(bIt == barrelMap.end()){
        std::cerr << "ERROR: No barrel mapping for wordID\n";
        return;
    }

    int barrelID = bIt->second;

    std::cout << "✓ Barrel: " << barrelID << "\n";

    // Step 3: Load only required barrel
    json barrel = loadBarrelFile(barrelDir, barrelID);

    std::string wordKey = std::to_string(wordID);

    // Step 4: Retrieve posting list
    if(!barrel.contains(wordKey)){
        std::cout << "❌ Word not present in barrel\n";
        return;
    }

    json postingList = barrel[wordKey];

    // Step 5: Display results
    std::cout << "\n🔍 RESULTS:\n";
    std::cout << "Documents containing \""
              << keyword << "\":\n";

   for(auto& item : postingList.items())
{
    std::string docID = item.key();
    json freqs = item.value();  // can be number or array

    std::cout << " - Doc: " << docID << ", Frequencies: ";
    
    if(freqs.is_number()) {
        std::cout << freqs.get<int>();
    } else if(freqs.is_array()) {
        for(auto& f : freqs) std::cout << f << " ";
    } else {
        std::cout << "?";
    }

    std::cout << "\n";
}

    std::cout
      << "\nTotal matches: "
      << postingList.size()
      << "\n";
}


// -------------------- MAIN --------------------
// int main(int argc, char* argv[])
// {
//     if(argc < 5)
//     {
//         std::cout << "Usage:\n";
//         std::cout << "single_search <lexicon.json> "
//                   << "<barrel_map.json> "
//                   << "<barrels_dir> "
//                   << "<search_word>\n";
//         return 1;
//     }

//     std::string lexFile    = argv[1];
//     std::string mapFile    = argv[2];
//     std::string barrelDir = argv[3];
//     std::string query     = argv[4];

//     // Load files
//     auto lexicon   = loadLexicon(lexFile);
//     auto barrelMap = loadBarrelMapping(mapFile);

//     std::cout << "Lexicon size: "
//               << lexicon.size() << "\n";

//     std::cout << "Barrel map size: "
//               << barrelMap.size() << "\n\n";

//     // Execute search
//     singleWordSearch(query, lexicon,
//                      barrelMap, barrelDir);

//     return 0;
// }
