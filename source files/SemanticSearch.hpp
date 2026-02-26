#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>

// =================================================================
// TYPE DEFINITIONS
// =================================================================

// Define the type for a single word's embedding vector
using EmbeddingVector = std::vector<float>;

// Define the type for the entire embedding vocabulary
// Word (string) -> Vector (vector of floats)
using WordEmbeddingsMap = std::unordered_map<std::string, EmbeddingVector>;

// =================================================================
// VECTOR UTILITY FUNCTIONS
// =================================================================

/**
 * @brief Calculates the Cosine Similarity between two embedding vectors.
 */
float calculateCosineSimilarity(const EmbeddingVector& vecA, const EmbeddingVector& vecB) {
    if (vecA.size() != vecB.size() || vecA.empty()) {
        return 0.0f; // Vectors must be the same size and non-empty
    }

    float dotProduct = 0.0f;
    float magnitudeSqA = 0.0f;
    float magnitudeSqB = 0.0f;

    // Calculate all three sums in a single loop
    for (size_t i = 0; i < vecA.size(); ++i) {
        dotProduct += vecA[i] * vecB[i];
        magnitudeSqA += vecA[i] * vecA[i];
        magnitudeSqB += vecB[i] * vecB[i];
    }
    
    float magnitudeA = std::sqrt(magnitudeSqA);
    float magnitudeB = std::sqrt(magnitudeSqB);

    if (magnitudeA == 0.0f || magnitudeB == 0.0f) {
        return 0.0f; // Avoid division by zero
    }

    return dotProduct / (magnitudeA * magnitudeB);
}


/**
 * @brief Creates a single Query Vector by averaging the vectors of all its component words.
 *
 * NOTE: This is a simple implementation. Tokenization is simplified.
 */
EmbeddingVector getQueryVector(const std::string& query, const WordEmbeddingsMap& embeddings) {
    std::stringstream ss(query);
    std::string word;
    int wordCount = 0;
    EmbeddingVector finalVector;
    
    // Tokenize the query (very basic: assumes space-separated words)
    while (ss >> word) {
        // Simple lower-casing for better matching
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        
        // Check if the word exists in the embeddings map
        if (embeddings.count(word)) {
            const EmbeddingVector& currentVector = embeddings.at(word);
            
            // Initialize the finalVector size on the first word found
            if (finalVector.empty()) {
                finalVector.resize(currentVector.size(), 0.0f);
            }
            
            // Sum the vectors
            for (size_t i = 0; i < currentVector.size(); ++i) {
                finalVector[i] += currentVector[i];
            }
            wordCount++;
        }
    }

    // Average the vectors
    if (wordCount > 0) {
        for (float& value : finalVector) {
            value /= wordCount;
        }
    }
    
    return finalVector;
}

// =================================================================
// DATA LOADING
// =================================================================

/**
 * @brief Loads word embeddings from a file into the WordEmbeddingsMap.
 */
WordEmbeddingsMap loadWordEmbeddings(const std::string& filePath) {
    WordEmbeddingsMap embeddings;
    std::ifstream file(filePath);
    
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open word embeddings file: " << filePath << "\n";
        return embeddings;
    }

    std::string line;
    std::cout << "Loading word embeddings from " << filePath << "...\n";
    
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string word;
        float value;
        
        if (!(ss >> word)) continue; 

        EmbeddingVector vector;
        while (ss >> value) {
            vector.push_back(value);
        }

        if (!vector.empty()) {
            embeddings[word] = vector;
        }
    }
    
    std::cout << "SUCCESS: Loaded " << embeddings.size() << " word embeddings.\n";
    return embeddings;
}