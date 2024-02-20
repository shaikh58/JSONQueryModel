In this project, I have created an in-memory JSON model, stored in a graph structure, to handle arbitrary JSON file formats. An off-the-shelf parser has been used to parse JSON files, 
and this file is converted into an in-memory model. 

I have also created a basic query language with the ability to select, filter, sum, count, and display.
- Select allows the user to access any key in the JSON file at any level in the structure
- Filter allows the user to choose elements in a list based on their index (using comparison operators), or elements in key-value pairs in the JSON file. Substring key searches
are also supported e.g. "key contains user" will find user_age and user_name
- Get serializes the JSON data and displays the selected and filtered data as a string
