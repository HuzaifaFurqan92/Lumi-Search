#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// -------------------- Load Barrel Mapping --------------------
std::unordered_map<int,int> loadBarrelMapping(const std::string& mapFile)
{
    std::ifstream fin(mapFile);
    if(!fin){
        std::cerr << "ERROR: Cannot open barrel mapping file\n";
        exit(1);
    }

    json mapJson;
    fin >> mapJson;
    fin.close();

    std::unordered_map<int,int> barrelMap;

    for(auto& [lexID, barrelID] : mapJson.items())
        barrelMap[std::stoi(lexID)] = barrelID;

    return barrelMap;
}


// -------------------- Load Inverted Index --------------------
json loadInvertedIndex(const std::string& invFile)
{
    std::ifstream fin(invFile);

    if(!fin){
        std::cerr << "ERROR: Cannot open inverted index file\n";
        exit(1);
    }

    json invJson;
    fin >> invJson;
    fin.close();

    return invJson;
}


// -------------------- Create Barrel Files --------------------
void buildBarrels(
    const json& invertedIndex,
    const std::unordered_map<int,int>& barrelMap,
    const std::string& outDir)
{
    fs::create_directories(outDir);

    std::vector<json> barrels(32);   // 32 total barrels

    // Iterate through inverted index
    for(auto& [lexIDstr, docList] : invertedIndex.items())
    {
        int lexID = std::stoi(lexIDstr);

        auto it = barrelMap.find(lexID);
        if(it == barrelMap.end())
            continue;

        int barrelID = it->second;

        // Store word's posting list inside its barrel
        barrels[barrelID][lexIDstr] = docList;
    }

    // Save barrel files
    for(int i = 0; i < 32; i++)
    {
        std::string filename =
            outDir + "/barrel_" + std::to_string(i) + ".json";

        std::ofstream fout(filename);

        if(!fout){
            std::cerr << "ERROR: Cannot write " << filename << "\n";
            continue;
        }

        fout << barrels[i].dump(4);
        fout.close();

        std::cout << "✓ Barrel " << i
                  << " saved with "
                  << barrels[i].size()
                  << " terms\n";
    }
}


// -------------------- Main --------------------
int main(int argc, char* argv[])
{
    if(argc < 4){
        std::cout << "Usage: member2_barrel_builder "
                  << "<inverted_index.json> "
                  << "<barrel_mapping.json> "
                  << "<output_dir>\n";
        return 1;
    }

    std::string invertedFile = argv[1];
    std::string mapFile      = argv[2];
    std::string outputDir   = argv[3];

    // Load data
    json invertedIndex = loadInvertedIndex(invertedFile);
    auto barrelMap     = loadBarrelMapping(mapFile);

    std::cout << "Loaded inverted index: "
              << invertedIndex.size()
              << " terms\n";

    std::cout << "Loaded barrel mapping: "
              << barrelMap.size()
              << " terms\n";

    // Build barrels
    buildBarrels(invertedIndex, barrelMap, outputDir);

    std::cout << "✅ Barrel generation complete.\n";

    return 0;
}
