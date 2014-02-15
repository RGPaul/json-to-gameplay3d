#JSON to Gameplay3D Property converter

Gameplay3D uses a bespoke Property format to load plain text data. However, many tools can already export data as JSON and many frameworks/api's exit to read/write it. This tool allows you to convert JSON directly to the Property format.

### Pre-requisites
- CMake
- A C++ compiler *(Tested on GCC 4.8.1 and MSVC v120)*

### Setup

- Run the setup script for your environment, this will use CMake to generate a project in a directory called *build*.
- Run the output project to generate the executable.

### Usage

json2gp3d -i <*json_file_to_convert*> -o <*gp_property_file_to_ouput*>

### Samples

The samples directory contains properties files that were output using the tool along with the JSON that was input to generate them.