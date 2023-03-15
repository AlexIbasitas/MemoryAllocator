# MemoryAllocator

ABOUT

This is a memory allocator that allows a user to allocate memory, free memory, and peek into the current state of the heap. 

HOW TO USE

INDEX CREATION

    Download the DEV folder to the project folder
    source venv/bin/activate
    python3 invertedIndex.py
    This will create files "docHash.json", "offsets.json", and "index.txt" (as well as numerous fragment*.txt files) which should automatically be used by the query search programs based on filename

QUERY SEARCH

    Text Interface:

        source venv/bin/activate
        python3 gui.py
        type a query and press enter to search
        type "quit()" to end
        
    Local GUI (Tkinter):

        source venv/bin/activate
        python3 textGui.py
        type a query in the search box
        press search
        type "quit()" to end
        
    Web GUI (Flask):
    
        source venv/bin/activate
        type into root directory: flask run
        go to link provided by flask (typically localhost 5000)
        type query in search box
        press submit
        new page will contain links along with the ability to continue to search for more
        




