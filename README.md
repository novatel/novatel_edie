<div align="center">
    <img alt="edie_logo" src=./resources/novatel-edie-logo-body.png width="40%">
</div>

# EDIE
EDIE, the Encode Decode Interface Engine software development kit allows interfacing and decoding data output from [**NovAtel's**](https://www.novatel.com) [**OEM7**](https://novatel.com/products/receivers/oem-receiver-boards/oem7-receivers) receivers.

## Building EDIE from source code
### Prerequisites
* Linux (Ubuntu tested) or Windows 10
* TODO: Compilers

### Compiling binaries
#### Linux
These instructions assume that you are using Ubuntu 18.04 or newer.

1. Open terminal 
2. Update the system: ```apt-get update```
3. Install make, cmake tools and g++ compiler: `apt-get install -y cmake make g++`
4. Install multilib for gcc anf g++: `apt-get install --yes gcc-multilib g++-multilib`
5. Clone the EDIE repository and change the folder permission: `sudo chmod -R 777 nov-decoder/`
6. Create a build folder in the root directory: `mkdir build`
7. Go to build folder: `cd build`
8. Configure cmake for static library: `cmake -DCMAKE_BUILD_TYPE=Release ..`
9. Configure cmake for shared along with static library: `cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_LIB_SHARED=1 ..`
10. Build: `make`

After compiling the binaries you can also run `make install` to copy the binaries to the /usr/ directory.
1. Archives will be copied to `/usr/lib`
2. Libraries will be copied to `/usr/lib`
3. Public headers will be copied to `/usr/include/novatel/edie/decoder`

Note: 
1. 'CMAKE_BUILD_TYPE' could be 'Release or Debug'

#### Windows
These instructions assume that you are using Windows 10.

1.	Install the latest version of CMAKE (https://cmake.org/install/)
2.	Clone the EDIE repository
4.	Open a PowerShell session in the root directory
5.	Create build directory and navigate to it: `mkdir build && cd build`
6.	Generate configuration for static library for VS 2017: `cmake .. -G "Visual Studio 15 2017" -A Win32`
7.	Generate configuration for shared along with static library for VS 2017: `cmake .. -G "Visual Studio 15 2017" -A Win32 -DCMAKE_LIB_SHARED=1`
    - You can replace the `-G` argument with any version VS newer than "Visual Studio 15 2017"
8.	Build & Install: `cmake --build . --config Release --target install`
    - '--config' could be 'Release or Debug' 

Build artifacts (such as public include files) will be copied to the bin directory in the root of the project.
Building EDIE in Windows will also create a solution file (<project>.sln) in the build directory, which can be opened in Visual Studio 2017. EDIE can be built by Visual Studio through this solution file. Alternatively newer version of Visual Studio can open cmake projects directly.

### Generate documentation
### Linux

1. Update the system: `apt-get update`
2. Install tzdata package: `apt-get install -y tzdata` 
3. Install doxygen and python3-pip: `apt-get install -y doxygen python3-pip`
4. Install exhale and sphinx_rtd_theme: `pip3 install exhale sphinx_rtd_theme`
5. Run Sphinx on each component e.g. `sphinx-build src/decoders/common/doc doc/decoders/common/doc/html`

### Windows

1. Install exhale and sphinx_rtd_theme: `pip3 install exhale sphinx_rtd_theme`
2. Run Sphinx on each component e.g. `sphinx-build src/decoders/common/doc doc/decoders/common/doc/html`

## FAQ

Q: Where can I find the documention for the logs.  
A: Click [here](https://docs.novatel.com/OEM7/Content/Logs/Core_Logs.htm) to view our documentation 

Q: How can the binary log data be used to extract data from a log.  
A: The database folder contains the file novatel_log_definitions.hpp that can be used to cast the binary data to a structure. 

## Roadmap
#### Short-term
* Work on more examples and integration guides
* Create a script to generate log and fieldnames so it's will be easier to use the JSON format
* Expose more Stream Interfaces though the dynamic library
* More unit testing

#### Long-term
* Rewrite Framing and Decoder code to remove the current couping from each other and clean up the codebase and logic
* Change DLL to output binary data. JSON parsing in Python slows down the decoder consideribly. 

## Authors

* [**NovAtel**](https://www.novatel.com), part of [**Hexagon**](https://hexagon.com)


## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details

