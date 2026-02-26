// In DocumentVectors.cpp

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include "nlohmann/json.hpp" 

using json = nlohmann::json;
using EmbeddingVector = std::vector<float>;
using DocumentVectorsMap = std::unordered_map<int, EmbeddingVector>;

/**
 * @brief Loads pre-calculated document vectors from a JSON file.
 * The file is assumed to be structured as: {"1": [0.1, 0.2, 0.3...], "2": [...], ...}
 */
DocumentVectorsMap loadDocumentVectors(const std::string& docVecFile) {
    DocumentVectorsMap docVectors;
    std::ifstream fin(docVecFile);
    
    if(!fin.is_open()) {
        std::cerr << "ERROR: Cannot open Document Vectors file: " << docVecFile << "\n";
        return docVectors; 
    }
    
    json docVecJson;
    try {
        fin >> docVecJson;
    } catch (const std::exception& e) {
        std::cerr << "ERROR parsing document vectors JSON: " << e.what() << "\n";
        return docVectors;
    }
    fin.close();

    for (auto& [docID_str, vector_json] : docVecJson.items()) {
        try {
            int docID = std::stoi(docID_str);
            docVectors[docID] = vector_json.get<EmbeddingVector>();
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to convert DocID or vector data: " << docID_str << "\n";
        }
    }
    std::cout << "SUCCESS: Loaded " << docVectors.size() << " document vectors.\n";
    return docVectors;
}