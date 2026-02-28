import streamlit as st
import lumi_core
from google import genai
import os

# --- PAGE SETUP ---
st.set_page_config(page_title="LumiSearch AI", page_icon="🔍", layout="wide")
st.title("🔍 LumiSearch: C++ Indexed AI")

# --- INITIALIZE ENGINES ---
@st.cache_resource
def init_engines():
    # Use absolute paths to avoid "File Not Found" errors
    base = os.path.dirname(os.path.abspath(__file__))
    engine = lumi_core.LumiEngine(
        os.path.join(base, "lexicon.json"),
        os.path.join(base, "barrel_map.json"),
        os.path.join(base, "df_map.json"),
        os.path.join(base, "barrels/")
    )
    # Replace with your actual Gemini Key
    client = genai.Client(api_key="YOUR_GEMINI_API_KEY")
    return engine, client

engine, ai_client = init_engines()

# --- SIDEBAR (Autocomplete Test) ---
st.sidebar.header("Trie Autocomplete (C++)")
prefix = st.sidebar.text_input("Test Trie Prefix:", value="da")
suggestions = engine.complete(prefix)
st.sidebar.write(suggestions)

# --- MAIN SEARCH ---
query = st.text_input("Enter your search query:", placeholder="Type something to search...")

if query:
    col1, col2 = st.columns([1, 2])
    
    with col1:
        st.subheader("C++ Retrieval")
        results = engine.search(query)
        if results:
            for res in results[:5]:
                st.success(f"📄 DocID: {res.docID} (Score: {round(res.score, 4)})")
        else:
            st.warning("No local matches found.")

    with col2:
        st.subheader("Gemini AI Summary")
        if results:
            with st.spinner("AI is analyzing local data..."):
                doc_list = [r.docID for r in results[:5]]
                prompt = f"User searched for '{query}'. C++ found these DocIDs: {doc_list}. Summarize the relevance."
                response = ai_client.models.generate_content(model='gemini-2.0-flash', contents=prompt)
                st.info(response.text)
        else:
            st.write("Waiting for valid search results...")

st.markdown("---")
st.caption("Powered by C++ Engine (Custom Inverted Index) & Gemini 2.0")