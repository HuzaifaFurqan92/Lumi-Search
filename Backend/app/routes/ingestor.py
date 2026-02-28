import chromadb
from sentence_transformers import SentenceTransformer
from fastapi import FastAPI, UploadFile, File
import shutil
import os

app = FastAPI()

# 1. Initialize the Embedding Model (This is the Deep Learning part)
# We use 'all-MiniLM-L6-v2' because it's fast and standard for RAG
model = SentenceTransformer('all-MiniLM-L6-v2')

# 2. Setup ChromaDB (Your Vector Database)
client = chromadb.PersistentClient(path="./chroma_db")
collection = client.get_or_create_collection(name="lumi_search_docs")

@app.post("/upload")
async def upload_document(file: UploadFile = File(...)):
    # Save file locally (so your C++ indexer can also access it later)
    file_location = f"data/{file.filename}"
    with open(file_location, "wb+") as file_object:
        shutil.copyfileobj(file.file, file_object)

    # Read content for the Vector DB
    with open(file_location, "r", encoding="utf-8") as f:
        content = f.read()

    # 3. Generate Embeddings (Deep Learning Inference)
    # This turns your text into a 384-dimensional vector
    embedding = model.encode(content).tolist()

    # 4. Store in Vector DB
    collection.add(
        embeddings=[embedding],
        documents=[content],
        metadatas=[{"filename": file.filename}],
        ids=[file.filename]
    )

    return {"status": "Successfully encoded and stored", "filename": file.filename}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)