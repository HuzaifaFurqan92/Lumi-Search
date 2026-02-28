#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// 1. Include the Search Logic first (this defines SearchResult and loadLexicon)
#include "new_Semantic.cpp" 

// 2. Include the Autocomplete Logic second
#include "auto_complete.cpp" 

namespace py = pybind11;

// The Engine class now uses logic from BOTH files
class LumiEngine {
public:
    std::unordered_map<std::string, int> lex;
    std::unordered_map<int, int> barrelMap;
    DFMap df;
    std::string barrelDir;
    AutocompleteEngine trie; // From auto_complete.cpp

    LumiEngine(std::string lexPath, std::string mapPath, std::string dfPath, std::string bDir) 
        : barrelDir(bDir) {
        
        lex = loadLexicon(lexPath); // From new_Semantic.cpp
        barrelMap = loadBarrelMap(mapPath);
        df = loadDFMap(dfPath);
        
        for (auto const& [word, id] : lex) {
            trie.addWordToLexicon(word);
        }
    }

    // This calls the function in new_Semantic.cpp
    std::vector<SearchResult> search(std::string query) {
        return run_search(query, lex, barrelMap, df, barrelDir);
    }

    // This calls the method in auto_complete.cpp
    std::vector<std::string> complete(std::string prefix) {
        return trie.getSuggestions(prefix, 5);
    }
};
PYBIND11_MODULE(lumi_core, m) {
    py::class_<SearchResult>(m, "SearchResult")
        .def_readwrite("docID", &SearchResult::docID)
        .def_readwrite("score", &SearchResult::score);

    // Inside PYBIND11_MODULE(lumi_core, m)
py::class_<LumiEngine>(m, "LumiEngine")
    .def(py::init<std::string, std::string, std::string, std::string>())
    .def("search", &LumiEngine::search)
    .def("complete", &LumiEngine::complete)
    .def_readwrite("lex", &LumiEngine::lex); // <--- Add this line to allow Python to see 'lex'
}