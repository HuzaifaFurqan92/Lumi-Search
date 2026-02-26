#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <map>
#include <chrono> 
#include "nlohmann/json.hpp" // Ensure nlohmann/json.hpp is in the same directory

using json = nlohmann::json;

// Define a type for your posting lists: DocID (int) -> Frequency (int)
using PostingList = std::unordered_map<int, int>; 


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
// Helper: Tokenize query string into a vector of words
// ----------------------------------------------------
std::vector<std::string> tokenize(const std::string& query) {
    std::vector<std::string> tokens;
    std::stringstream ss(query);
    std::string token;
    while (ss >> token) {
        // Normalize to lowercase for matching
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        tokens.push_back(token);
    }
    return tokens;
}

// ----------------------------------------------------
// Retrieve Posting List for a single word (Used by multi-word search)
// ----------------------------------------------------
PostingList getWordPostings(
    const std::string& query,
    const std::unordered_map<std::string, int>& lexMap,
    const std::unordered_map<int, int>& barrelMap,
    const std::string& barrelsDir)
{
    PostingList result;
    auto it = lexMap.find(query);
    if (it == lexMap.end()) return result; // Word not in lexicon

    int lexID = it->second;
    
    // Safety check for barrelMap lookup
    auto map_it = barrelMap.find(lexID);
    if (map_it == barrelMap.end()) return result;
    
    int barrelID = map_it->second;

    // Load only the required barrel
    json barrel = loadBarrel(barrelsDir, barrelID);

    // Find posting list within the barrel
    std::string lexIDstr = std::to_string(lexID);

    if (!barrel.contains(lexIDstr)) return result; // No postings

    // Convert json posting list (DocID_string: Freq_int) to internal map (DocID_int: Freq_int)
    for (auto& [docID_str, freq] : barrel[lexIDstr].items()) {
        try {
            // Note: The conversion to int here is important for the merging logic
            result[std::stoi(docID_str)] = freq; 
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to convert DocID string to int: " << docID_str << "\n";
        }
    }
    return result;
}

// ----------------------------------------------------
// Merge (Intersect) two posting lists and sum their frequencies (AND Logic)
// ----------------------------------------------------
PostingList mergePostingLists(const PostingList& listA, const PostingList& listB) {
    PostingList mergedList;

    // Optimize by iterating over the smaller list
    const PostingList* smaller = (listA.size() < listB.size()) ? &listA : &listB;
    const PostingList* larger = (listA.size() < listB.size()) ? &listB : &listA;

    for (const auto& pair : *smaller) {
        int docID = pair.first;
        int freqA = pair.second;

        auto it = larger->find(docID);
        if (it != larger->end()) {
            // Document is present in both lists (Intersection found)
            int freqB = it->second;
            // Rank by summing the frequencies (Total Term Frequency in Document)
            mergedList[docID] = freqA + freqB;
        }
    }
    return mergedList;
}

// ----------------------------------------------------
// Search for multiple words (Public Interface)
// ----------------------------------------------------
void searchMultiWord(
    const std::string& query,
    const std::unordered_map<std::string, int>& lexMap,
    const std::unordered_map<int, int>& barrelMap,
    const std::string& barrelsDir)
{
    std::vector<std::string> words = tokenize(query);
    if (words.empty()) {
        std::cout << "Query is empty.\n";
        return;
    }
    
    // STEP 1: Get the first word's posting list to initialize the results
    PostingList finalPostings = getWordPostings(words[0], lexMap, barrelMap, barrelsDir);
    
    std::cout << "[DEBUG] First word '" << words[0] << "' retrieved " << finalPostings.size() << " postings.\n";

    if (finalPostings.empty()) {
        std::cout << "No results found.\n";
        return;
    }
    
    // STEP 2: Sequentially intersect with the remaining words
    for (size_t i = 1; i < words.size(); ++i) {
        PostingList nextPostings = getWordPostings(words[i], lexMap, barrelMap, barrelsDir);

        std::cout << "[DEBUG] Merging with '" << words[i] << "' (" << nextPostings.size() << " postings).\n";

        // Perform the intersection and sum frequencies
        finalPostings = mergePostingLists(finalPostings, nextPostings);
        
        // If intersection yields empty, we can stop early
        if (finalPostings.empty()) {
            std::cout << "No results found. (Intersection failed at word '" << words[i] << "')\n";
            return;
        }
    }

    // STEP 3: Ranking (Sort by Total Frequency)
    // Convert map to a vector of pairs for sorting (DocID, Score)
    std::vector<std::pair<int, int>> sortedResults;
    for (const auto& pair : finalPostings) {
        sortedResults.push_back({pair.first, pair.second});
    }

    // Sort by score (frequency, second element of the pair) in descending order
    std::sort(sortedResults.begin(), sortedResults.end(), 
        [](const auto& a, const auto& b) {
            return a.second > b.second; 
        });


    std::cout << "\n=== MULTI-WORD RESULTS: '" << query << "' ===\n";
    if (sortedResults.empty()) {
        std::cout << "No documents contain all query words.\n";
    } else {
        std::cout << "Found " << sortedResults.size() << " documents containing all words, ranked by total frequency:\n";
        for (const auto& pair : sortedResults) {
            std::cout << "Doc " << pair.first 
                      << " (Total freq: " << pair.second 
                      << ")\n";
        }
    }
}

// ----------------------------------------------------
// MAIN
// ----------------------------------------------------

int main(int argc, char* argv[]) {
    // Check if the query is a single argument or multiple
    if (argc < 5) {
        std::cout << "Usage: search <query_string> <lexicon.json> <barrel_mapping.json> <barrels_directory>\n";
        std::cout << "Example (multi-word query): ./search_engine \"data structures\" lexicon.json map.json barrels\n";
        return 1;
    }

    // Combine all arguments after the executable into a single query string
    std::string query;
    for (int i = 1; i < argc - 3; ++i) { // Arguments 1 up to (argc - 4) are query words
        query += argv[i];
        if (i < argc - 4) {
            query += " "; // Add space between query words
        }
    }
    
    // The last three arguments are the file paths
    std::string lexFile    = argv[argc - 3];
    std::string mapFile    = argv[argc - 2];
    std::string barrelsDir = argv[argc - 1];

    // Load static data first
    std::cout << "Loading lexicon...\n";
    auto lexMap    = loadLexicon(lexFile);
    std::cout << "Loading barrel mapping...\n";
    auto barrelMap = loadBarrelMapping(mapFile);
    std::cout << "Data loaded.\n";
    
    // --- Timing the search function ---
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    
    // Record start time
    auto t1 = high_resolution_clock::now();
    
    // Perform the multi-word search
    searchMultiWord(query, lexMap, barrelMap, barrelsDir);
    
    // Record end time
    auto t2 = high_resolution_clock::now();
    
    // Calculate the duration
    auto ms_int = duration_cast<microseconds>(t2 - t1);
    
    // Print the elapsed time
    std::cout << "\nTime taken for search: " 
              << (ms_int.count() / 1000)
              << " milliseconds\n";
    // -------------------------------------

    return 0;
}