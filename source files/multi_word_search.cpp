#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <map>
#include <chrono> 
#include <iomanip> // For std::setprecision
#include <cmath>   // For log/sqrt in TF-IDF/Ranking
#include "nlohmann/json.hpp" 
#include "Scoring.hpp" // Contains ScoreMap, DFMap, SearchResult, rankResults (and presumably other declarations)

// Include the new semantic utilities (Assuming these files contain the implementations from previous turns)
#include "SemanticSearch.hpp"    
#include "DocumentVectors.cpp" 

using json = nlohmann::json;

// Define a type for your posting lists: DocID (int) -> Frequency (int)
using PostingList = std::unordered_map<int, int>; 

// Total number of documents. THIS MUST BE ACCURATE TO YOUR DATASET 
const int TOTAL_DOCUMENTS = 50000; 

// ----------------------------------------------------
// Load Lexicon (Unchanged)
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
// Load Barrel Mapping (Unchanged)
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
// Load Document Frequency (DF) Map (Unchanged)
// ----------------------------------------------------
DFMap loadDFMap(const std::string& dfFile) {
    std::ifstream fin(dfFile);
    if(!fin) {
        std::cerr << "ERROR: Cannot open Document Frequency (DF) file: " << dfFile << "\n";
        exit(1); 
    }
    json dfJson;
    fin >> dfJson;
    fin.close();

    DFMap dfMap;
    for (auto& [lexID_str, df_val] : dfJson.items())
        dfMap[std::stoi(lexID_str)] = df_val;

    return dfMap;
}

// ----------------------------------------------------
// Load Barrel (Unchanged)
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
// Tokenize Query (Unchanged)
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
// Retrieve Posting List (Unchanged)
// ----------------------------------------------------
PostingList getWordPostings(
    const std::string& query,
    const std::unordered_map<std::string, int>& lexMap,
    const std::unordered_map<int, int>& barrelMap,
    const std::string& barrelsDir)
{
    PostingList result;
    auto it = lexMap.find(query);
    if (it == lexMap.end()) return result; 

    int lexID = it->second;
    
    auto map_it = barrelMap.find(lexID);
    if (map_it == barrelMap.end()) return result;
    
    int barrelID = map_it->second;

    json barrel = loadBarrel(barrelsDir, barrelID);

    std::string lexIDstr = std::to_string(lexID);

    if (!barrel.contains(lexIDstr)) return result; 

    for (auto& [docID_str, freq] : barrel[lexIDstr].items()) {
        try {
            result[std::stoi(docID_str)] = freq; 
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to convert DocID string to int: " << docID_str << "\n";
        }
    }
    return result;
}


// ----------------------------------------------------
// Merge (Intersect) two posting lists (Unchanged)
// ----------------------------------------------------
PostingList mergePostingLists(const PostingList& listA, const PostingList& listB) {
    PostingList mergedList;
    const PostingList* smaller = (listA.size() < listB.size()) ? &listA : &listB;
    const PostingList* larger = (listA.size() < listB.size()) ? &listB : &listA;

    for (const auto& pair : *smaller) {
        int docID = pair.first;
        int freqA = pair.second;

        auto it = larger->find(docID);
        if (it != larger->end()) {
            int freqB = it->second;
            // Summing the frequencies (Total Term Frequency in Document)
            mergedList[docID] = freqA + freqB;
        }
    }
    return mergedList;
}

// ----------------------------------------------------
// FIX: Placeholder for Missing calculateTFIDFScore
// NOTE: This must be in your Scoring.hpp or another included file in a final setup.
// This is added here to make the compilation work.
// ----------------------------------------------------
float calculateTFIDFScore(
    int ttf,
    int docID,
    const std::unordered_map<std::string, int>& lexMap,
    const std::vector<std::string>& queryWords,
    const DFMap& dfMap,
    int totalDocuments) 
{
    // Simple sum of TF * IDF for all query words in the document.
    float totalScore = 0.0f;
    for (const std::string& word : queryWords) {
        auto lexIt = lexMap.find(word);
        if (lexIt != lexMap.end()) {
            int lexID = lexIt->second;
            auto dfIt = dfMap.find(lexID);
            
            if (dfIt != dfMap.end()) {
                int df = dfIt->second;
                
                // Term Frequency (simplified: usually sqrt(freq) or log(freq))
                float tf = (float)ttf; 
                
                // Inverse Document Frequency
                float idf = std::log((float)totalDocuments / (1.0f + (float)df));
                
                totalScore += tf * idf;
            }
        }
    }
    // A simplified normalization or constant boost could be added here.
    return totalScore;
}


// ----------------------------------------------------
// Search for multiple words (UPDATED for Semantic Scoring)
// ----------------------------------------------------
void searchMultiWord(
    const std::string& query,
    const std::unordered_map<std::string, int>& lexMap,
    const std::unordered_map<int, int>& barrelMap,
    const DFMap& dfMap, 
    const std::string& barrelsDir,
    const WordEmbeddingsMap& embeddings,           // <<< NEW: Embeddings Map
    const DocumentVectorsMap& docVectors,         // <<< NEW: Document Vectors
    const EmbeddingVector& queryVector)           // <<< NEW: Pre-calculated Query Vector
{
    // STEP 1 & 2: Intersection Logic (Unchanged)
    std::vector<std::string> words = tokenize(query);
    // ... (Existing intersection and initial posting list retrieval logic) ...
    
    PostingList finalPostings = getWordPostings(words[0], lexMap, barrelMap, barrelsDir);
    
    std::cout << "[DEBUG] First word '" << words[0] << "' retrieved " << finalPostings.size() << " postings.\n";

    if (finalPostings.empty()) {
        std::cout << "No results found.\n";
        return;
    }
    
    for (size_t i = 1; i < words.size(); ++i) {
        PostingList nextPostings = getWordPostings(words[i], lexMap, barrelMap, barrelsDir);
        std::cout << "[DEBUG] Merging with '" << words[i] << "' (" << nextPostings.size() << " postings).\n";
        finalPostings = mergePostingLists(finalPostings, nextPostings);
        
        if (finalPostings.empty()) {
            std::cout << "No results found. (Intersection failed at word '" << words[i] << "')\n";
            return;
        }
    }

    // STEP 3: Scoring and Ranking (Uses Scoring.hpp and SEMANTIC SEARCH)
    
    ScoreMap scores;
    const float SEMANTIC_WEIGHT = 0.35f; // <<< Tunable weight for the semantic score

    std::cout << "[INFO] Scoring " << finalPostings.size() << " documents...\n";

    for (const auto& pair : finalPostings) {
        int docID = pair.first;
        int ttf = pair.second; // Total Term Frequency (TTF) of all query terms in this document

        // 1. Calculate TF-IDF Score
        float tfidf_score = calculateTFIDFScore(ttf, docID, lexMap, words, dfMap, TOTAL_DOCUMENTS); 

        // 2. Calculate Semantic Score (Cosine Similarity)
        float semantic_score = 0.0f;
        
        // Only run semantic search if we have a query vector AND the document vector
        if (!queryVector.empty() && docVectors.count(docID)) {
            const EmbeddingVector& docVector = docVectors.at(docID);

            if (docVector.size() == queryVector.size()) {
                 semantic_score = calculateCosineSimilarity(queryVector, docVector);
            }
        }
        
        // 3. Calculate Combined Final Score
        float finalScore = tfidf_score + (SEMANTIC_WEIGHT * semantic_score); 
        scores[docID] = finalScore;
    }
    

    // Rank the results
    std::vector<SearchResult> sortedResults = rankResults(scores);


    std::cout << "\n=== MULTI-WORD RESULTS: '" << query << "' ===\n";
    if (sortedResults.empty()) {
        std::cout << "No documents contain all query words.\n";
    } else {
        std::cout << "Found " << sortedResults.size() << " documents containing all words, ranked by combined score (TFIDF + Semantic):\n";
        
        for (const auto& result : sortedResults) {
            std::cout << "Doc " << result.docID 
                      << " (Score: " << std::fixed << std::setprecision(4) << result.score 
                      << ")\n";
        }
    }
}


// ----------------------------------------------------
// MAIN (CLEANED and UPDATED for Semantic Search)
// ----------------------------------------------------

// ----------------------------------------------------
// MAIN (CLEANED and UPDATED for Semantic Search)
// ----------------------------------------------------

int main(int argc, char* argv[]) {
    // NEW: Expect 7 arguments now (query, lexicon, map, df, barrels, embeddings, doc_vectors)
    if (argc < 8) { 
        std::cout << "Usage: search <query> <lexicon.json> <barrel_mapping.json> <df_map.json> <barrels_dir> <embeddings.vec> <doc_vectors.json>\n";
        std::cout << "Example: ./search_engine \"data structures\" ./lexicon.json ./map.json ../df.json ../barrels ../embeddings.vec ../doc_vectors.json\n";
        return 1;
    }

    // --- Cleaned Argument Extraction ---
    std::string query           = argv[1]; 
    std::string lexFile         = argv[2];
    std::string mapFile         = argv[3];
    std::string dfFile          = argv[4];
    std::string barrelsDir      = argv[5];
    std::string embeddingsFile  = argv[6]; // Word Embeddings File
    std::string docVecFile      = argv[7]; // Document Vectors File
    // --- End of Argument Extraction ---

    // Load static data
    std::cout << "Loading lexicon...\n";
    auto lexMap         = loadLexicon(lexFile);
    std::cout << "Loading barrel mapping...\n";
    auto barrelMap      = loadBarrelMapping(mapFile);
    std::cout << "Loading Document Frequency map...\n";
    auto dfMap          = loadDFMap(dfFile);
    
    // --- Semantic Search Loading ---
    WordEmbeddingsMap embeddings = loadWordEmbeddings(embeddingsFile);
    DocumentVectorsMap docVectors = loadDocumentVectors(docVecFile);
    // -------------------------------
    
    std::cout << "Data loaded (" << lexMap.size() << " terms, " << dfMap.size() << " DF entries, " << embeddings.size() << " embeddings).\n";
    
    // 1. Calculate Query Vector
    EmbeddingVector queryVector;
    if (!embeddings.empty()) {
        queryVector = getQueryVector(query, embeddings);
        if (queryVector.empty()) {
            std::cout << "[INFO] Query vector is empty. Semantic score will be zero.\n";
        }
    }
    
    std::vector<std::string> queryWords = tokenize(query); 
    size_t numWords = queryWords.size();
    
    // --- Timing the search function ---
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    
    auto t1 = high_resolution_clock::now();
    
    // Perform the multi-word search
    searchMultiWord(query, lexMap, barrelMap, dfMap, barrelsDir, embeddings, docVectors, queryVector); 
    
    auto t2 = high_resolution_clock::now();
    
    auto ms_int = duration_cast<microseconds>(t2 - t1);
    double time_ms = ms_int.count() / 1000.0;
    std::cout << "\nTime taken for search: " 
              << time_ms
              << " milliseconds\n";
    
    if (numWords == 1 && time_ms < 500.0) {
        std::cout << "QUERY PERFORMANCE: Single word query target met (Target: < 500ms).\n";
    } else if (numWords > 1 && numWords <= 5 && time_ms < 1500.0) {
        std::cout << "QUERY PERFORMANCE: Multi-word query target met (Target: < 1.5 seconds).\n";
    } else if (numWords > 5) {
        std::cout << "QUERY PERFORMANCE: Query is > 5 words. Performance should remain stable.\n";
    }

    return 0;
}
