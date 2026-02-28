import os
import lumi_core
from google import genai

# Get the directory where THIS script is saved
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Build absolute paths to your data files
lexicon_path = os.path.join(BASE_DIR, "lexicon.json") # or just "lexicon"
map_path = os.path.join(BASE_DIR, "barrel_map.json")
df_path = os.path.join(BASE_DIR, "df_map.json")
barrel_dir = os.path.join(BASE_DIR, "barrels/")

# Print these out so YOU can see what C++ is looking for
print(f"Loading Lexicon from: {lexicon_path}")

# Initialize the engine with absolute paths
engine = lumi_core.LumiEngine(lexicon_path, map_path, df_path, barrel_dir)

# 2. Start Gemini (The Brain)
# Get your key from api.env or paste it here
client = genai.Client(api_key="YOUR_API_KEY")

def search_with_ai(query):
    # A. Custom C++ Retrieval
    print(f"Checking C++ Index for: {query}")
    results = engine.search(query)
    
    if not results:
        return "C++ Engine: No matches found in local barrels."

    # B. AI Summary
    # We send the DocIDs found by your C++ code to Gemini
    doc_ids = [r.docID for r in results]
    
    response = client.models.generate_content(
        model='gemini-2.0-flash',
        contents=f"The user searched for '{query}'. My C++ engine found these DocIDs: {doc_ids}. "
                 f"Explain that we found matches and ask if they'd like to see the full text."
    )
    return response.text

if __name__ == "__main__":
    query = "data" # Test with a word you know is in your lexicon
    print("\n--- HYBRID SEARCH RESULT ---")
    print(search_with_ai(query))