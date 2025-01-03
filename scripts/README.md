<div align="center">
    <img alt="edie_logo" src=../resources/novatel-edie-logo-body.png width="40%">
</div>

# NovAtel EDIE Scripts

This folder contains scripts to help programmers use EDIE.

## Database to HTML

The `database_to_html.py` script generates an interactive website to explore the EDIE database. 
This is useful to learn which logs and commands are supported by EDIE and which fields are available.
To run the script, follow these steps:

1. Install Python 3.11 or newer.
2. Install json2html: `pip install json2html`
3. Run the script: `python [path_to_repo]\scripts\database_to_html.py [path_to_repo]\database\messages_public.json`
4. Open `[path_to_repo]\database\messages_public.html` in a web browser. 

## Generate Flat C++ Structs

The `gen_flat_cpp_structs.py` script generates a `novatel_message_definitions.hpp` file with C++ structs.
Each struct is a log used to cast EDIE's flattened binary output. By casting the output, the user can access the log's fields.
To run the script, follow these steps:

1. Install Python 3.11 or newer.
2. Run the script: `python [path_to_repo]\scripts\gen_flat_cpp_structs.py [path_to_repo]\database\messages_public.json`
3. Import `[path_to_repo]\novatel_message_definitions.hpp` and cast your data to the appropriate log struct.
