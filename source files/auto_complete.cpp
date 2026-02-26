#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <map> 
#include <chrono> // <-- ADDED MISSING HEADER
#include <sstream> // ADDED for argument handling
#include "nlohmann/json.hpp" // Ensure nlohmann/json.hpp is in the same directory

using json = nlohmann::json;

// Define a type for your lexicon map
using LexiconMap = std::unordered_map<std::string, int>;

// --- 1. The Building Block: A Node in the Tree ---
struct TrieNode {
    std::unordered_map<char, TrieNode*> next_letters;
    bool marks_end_of_a_word; 
    TrieNode() : marks_end_of_a_word(false) {}
    ~TrieNode() {
        for (auto const& [character, pointer_to_child] : next_letters) {
            delete pointer_to_child; 
        }
    }
};

// ----------------------------------------------------
// Project Function: Load lexicon: word → lexID
// ----------------------------------------------------
LexiconMap loadLexicon(const std::string& lexFile) {
    std::ifstream fin(lexFile);
    if(!fin) {
        std::cerr << "ERROR: Cannot open lexicon file: " << lexFile << "\n";
        exit(1);
    }
    json lexJson;
    try {
        fin >> lexJson;
    } catch (json::parse_error& e) {
        std::cerr << "ERROR: Failed to parse JSON in lexicon file: " << e.what() << "\n";
        exit(1);
    }
    fin.close();

    LexiconMap lexMap;
    int id = 1;
    // Assuming the lexicon is stored under the key "lexicon" as an array
    if (lexJson.contains("lexicon") && lexJson["lexicon"].is_array()) {
        for (const auto& w : lexJson["lexicon"]) {
            lexMap[w.get<std::string>()] = id++;
        }
    } else {
        std::cerr << "Warning: Lexicon JSON does not contain the expected 'lexicon' array.\n";
    }
    return lexMap;
}

// --- 2. The Autocomplete Engine (Trie Class) ---
class AutocompleteEngine {
private:
    TrieNode* root_node;

    void collectAllWords(TrieNode* start_node, std::string current_word_so_far, std::vector<std::string>& found_results, int max_limit) {
        if (found_results.size() >= max_limit) {
            return;
        }
        
        if (start_node->marks_end_of_a_word) {
            found_results.push_back(current_word_so_far);
        }

        // Use map to ensure alphabetical order in suggestions (nice feature, not essential for speed)
        std::map<char, TrieNode*> sorted_children(start_node->next_letters.begin(), start_node->next_letters.end());

        for (auto const& [next_char, child_node] : sorted_children) {
            collectAllWords(
                child_node, 
                current_word_so_far + next_char, 
                found_results, 
                max_limit
            );
        }
    }

public:
    AutocompleteEngine() {
        root_node = new TrieNode();
    }

    ~AutocompleteEngine() {
        delete root_node;
    }

    void addWordToLexicon(const std::string& word) {
        TrieNode* current_position = root_node;
        
        for (char character : word) {
            // Normalize to lowercase
            character = std::tolower(character); 
            
            if (current_position->next_letters.find(character) == current_position->next_letters.end()) {
                current_position->next_letters[character] = new TrieNode();
            }
            
            current_position = current_position->next_letters[character];
        }
        
        current_position->marks_end_of_a_word = true;
    }

    std::vector<std::string> getSuggestions(const std::string& user_prefix, int max_suggestions = 5) {
        std::vector<std::string> results;
        std::string normalized_prefix = user_prefix;

        std::transform(normalized_prefix.begin(), normalized_prefix.end(), normalized_prefix.begin(), ::tolower);

        TrieNode* current_position = root_node;

        // 1. Traverse to the prefix node
        for (char character : normalized_prefix) {
            if (current_position->next_letters.find(character) == current_position->next_letters.end()) {
                return results; // Prefix not found
            }
            current_position = current_position->next_letters[character];
        }

        // 2. Collect all words below the prefix node
        collectAllWords(
            current_position, 
            normalized_prefix, 
            results, 
            max_suggestions
        );
        
        return results;
    }
};

// --- Main Testing Function ---

int main(int argc, char* argv[]) {
    // Corrected argument check: need at least 2 arguments (./program_name and lexicon.json)
    if (argc < 2) {
        std::cout << "Usage: ./autocomplete_test <lexicon.json>\n";
        std::cout << "Example: ./autocomplete_test lexicon.json\n";
        return 1;
    }

    std::string lexFile = argv[1]; 
    
    // Check for nlohmann/json dependency
   
    // Load static data
    std::cout << "Loading lexicon from: " << lexFile << "...\n";
    LexiconMap lexMap = loadLexicon(lexFile); 
    std::cout << "Lexicon loaded with " << lexMap.size() << " unique words.\n";
    
    if (lexMap.empty()) {
        std::cout << "Cannot run autocomplete: Lexicon is empty.\n";
        return 0;
    }

    // --- Build the Autocomplete Trie (O(Total Characters in Lexicon)) ---
    std::cout << "Building Autocomplete Trie (Trie)... (This should be fast)\n";
    AutocompleteEngine trie_engine;
    
    // Insert ALL words from the loaded lexicon into the Trie
    for (const auto& pair : lexMap) {
        trie_engine.addWordToLexicon(pair.first); 
    }
    std::cout << "Autocomplete Trie built and ready.\n";
    // --------------------------------------------------------

    // --- Interactive Search Loop (Simulating Real-Time Use) ---
    std::cout << "\n------------------------------------------------\n";
    std::cout << "Enter a prefix to test real-time autocomplete.\n";
    std::cout << "------------------------------------------------\n";
    
    std::string query_prefix;
    while (true) {
        std::cout << "\nPrefix (or 'quit'): ";
        // Use std::getline to read the whole line, including spaces (though prefixes won't usually have them)
        if (!std::getline(std::cin, query_prefix) || query_prefix == "quit") {
            break;
        }

        if (query_prefix.empty()) continue;

        // Perform the real-time lookup (and measure performance)
        using std::chrono::high_resolution_clock;
        using std::chrono::duration_cast;
        using std::chrono::microseconds;
        
        auto t1 = high_resolution_clock::now();
        std::vector<std::string> suggestions = trie_engine.getSuggestions(query_prefix, 5);
        auto t2 = high_resolution_clock::now();
        
        auto us_int = duration_cast<microseconds>(t2 - t1);
        
        std::cout << "-> Suggestions (" << suggestions.size() << " found in " 
                  << us_int.count() << "µs):\n";
        
        if (suggestions.empty()) {
            std::cout << "   (None)\n";
        } else {
            for (const std::string& s : suggestions) {
                std::cout << " * " << s << "\n";
            }
        }
        
        // Check performance target (< 100ms = 100,000µs)
        if (us_int.count() < 100000) {
            std::cout << "   (Status: Meets project requirement < 100ms)\n";
        } else {
            std::cout << "   (Status: Warning! Exceeds project requirement of 100ms)\n";
        }
    }

    return 0;
}