# How to build, add, implement, the google test unit testing library "Google Test" aka "gtest" on windows, Visual Studio 2015 (VS 14).

// I will be telling you how I did it for my setup / project. You can do it any way you want.
// My project's name is CryptRelay, and the project folder is /CryptRelay/msvs/

Step 1: navigate to the folder /CryptRelay/ and clone into gtest here.
 Example: git clone https://github.com/google/googletest.git
 This will create a folder named googletest.

Step 2: navigate to the folder googletest. Ex:  /CryptRelay/googletest/

Step 3: checkout the latest release tag. As of the current time of writing this, it is v1.7.0
 git checkout release-1.7.0

Step 4: Open up the gtest solution file located at /CryptRelay/googletest/msvc/gtest.sln

Step 5: Build the gtest solution as x86 Debug.

Step 6: Build the gtest solution as x86 Release.

# Create a new empty project and name it CryptRelayTest

Step 1:


// STEPS BELOW HERE ARE WRONG. A SEPARATE PROJECT IS SUPPOSED TO BE CREATED TO TEST THINGS IN.

Step 7: Now open up the CryptRelay project solution located at /CryptRelay/msvs/CryptRelay.sln

Step 8: Project-> Properties-> VC++ Directories-> Include Directories.
 Make sure your configuration is set to Debug.
 Here we make a new include: $(ProjectDir)..\googletest\include
 Please note that the $(ProjectDir) is here: /CryptRelay/msvs/
 so that means the .. will makei t go up to /CryptRelay/ first, then to /CryptRelay/googletest/
 So if your project layout is not setup like that, adjust accordingly.

Step 9: Project-> Properties-> Linker-> Input-> Additional Dependencies
 Make sure your configuration is set to Debug.
 Here, on new lines, add these:
 $(ProjectDir)..\googletest\msvc\gtest\Debug\gtestd.lib
 $(ProjectDir)..\googletest\msvc\gtest\Debug\gtest_maind.lib

Step 10: Project-> Properties-> VC++ Directories-> Include Directories.

Step 11: do Step 8 and 9, but this time with your configuration set to Release
 Don't forget to add file paths that point to the release library, instead of debug.

Step 12: In your project, add the include in your file:
 #include <gtest/gtest.h>

# Now time to create an example test case:
