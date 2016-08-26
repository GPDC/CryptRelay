# How to build, add, implement, the google test unit testing library "Google Test" aka "gtest" on windows, Visual Studio 2015 (VS 14).

// I will be telling you how I did it for my setup / project. You can do it any way you want.
// My project's name is CryptRelay, and the project folder is /CryptRelay/msvs/


Step 1: navigate to the folder /CryptRelay/ and clone into gtest here.

 Example: git clone https://github.com/google/googletest.git
 This will create a folder named googletest.


Step 2: navigate to the folder googletest. Ex:  /CryptRelay/googletest/


Step 3: checkout master branch (or checkout whatever version you want)


Step 4: Open up the gtest solution file located at /CryptRelay/googletest/msvc/gtest.sln


Step 5: Build the gtest solution as x86 Debug.


Step 6: Build the gtest solution as x86 Release.


Step 7: Open up the gmock solution file located at /CryptRelay/googlemock/msvc/2015/gmock.sln


Step 8: Build the gmock solution as x86 Debug.


Step 9. Build the gmock solution as x86 Release.



# Create a new empty project and name it CryptRelayTest

Step 1: make sure your unit_test project is using the same runtime libraries

 as the gtest library. They can be changed at:
 project properties-> configuration Properties-> C/C++-> CodeGeneration-> Runtime Library



Step 2: In project properties, set your Configuration to Debug, then continue with the steps.



Step 3: Project-> Properties-> VC++ Directories-> Include Directories

 Make new includes:
 $(ProjectDir)..\..\..\googletest\googlemock\include
 $(ProjectDir)..\..\..\googletest\googletest\include
 $(ProjectDir)..\..\..\src  // This is where your poject that you are testing's source files are.



Step 4: Project-> Properties-> VC++ Directories-> Libary Directories

 Make new includes:
 $(ProjectDir)..\..\..\googletest\googlemock\msvc\2015\Debug
 $(ProjectDir)..\..\..\googletest\googletest\msvc\gtest\Debug



Step 5: Project-> Properties-> Linker-> Input-> Additional Dependencies

 On a new line for each one, type:
 $(ProjectDir)..\..\..\googletest\googletest\msvc\gtest\Debug\gtestd.lib
 $(ProjectDir)..\..\..\googletest\googletest\msvc\gtest\Debug\gtest_maind.lib
 $(ProjectDir)..\..\..\googletest\googlemock\msvc\2015\Debug\gmock.lib
 $(ProjectDir)..\..\..\googletest\googlemock\msvc\2015\Debug\gmock_main.lib



Step 6: In project properties, set your Configuration to Release, then continue with the steps.



Step 7: Project-> Properties-> VC++ Directories-> Include Directories

 Make new includes:
 $(ProjectDir)..\..\..\googletest\googlemock\include
 $(ProjectDir)..\..\..\googletest\googletest\include
 $(ProjectDir)..\..\..\src  // This is where your poject that you are testing's source files are.



Step 8: Project-> Properties-> VC++ Directories-> Libary Directories

 Make new includes:
 $(ProjectDir)..\..\..\googletest\googlemock\msvc\2015\Release
 $(ProjectDir)..\..\..\googletest\googletest\msvc\gtest\Release



Step 9: Project-> Properties-> Linker-> Input-> Additional Dependencies

 On a new line for each one, type:
 $(ProjectDir)..\..\..\googletest\googletest\msvc\gtest\Release\gtestd.lib
 $(ProjectDir)..\..\..\googletest\googletest\msvc\gtest\Release\gtest_maind.lib
 $(ProjectDir)..\..\..\googletest\googlemock\msvc\2015\Release\gmock.lib
 $(ProjectDir)..\..\..\googletest\googlemock\msvc\2015\Release\gmock_main.lib



# Now for actually writing a sample unit test

Step 1: Include header files

 #include "gtest/gtest.h"
 #include "gmock/gmock.h"



Step 2: put this as the main function:

int32_t main(int argc, char * argv[])
{
	testing::InitGoogleTest(&argc, argv); // start google test in cmd prompt
	return RUN_ALL_TESTS(); // test all TESTs
}



Step 3: To test something, just create something like this:

class Backpack
{
	item_count;
};

// The first arg for TEST() is the category name that would categorize this test.
// Second arg is the name of the test that you are doing.
TEST(BackpackTest, BackpackStartsEmpty)
{
	Backpack Bp;   // Creating an instance of the backpack.
	EXPECT_EQ(0, Bp.item_count); // The test will expect arg1 is equal to arg2.
}

	If you build and run the program, you should see the tests being conducted in the console.
If everything was done properly, then you will notice that the test failed. That is a good thing.
It was designed to fail. Chances are it was expecting item_count to be equal to 0, just like you set it up to expect. However, if you look at the Backpack class you will notice that we never assigned item_count a value. So if you are in debug, item_count was probably something along the lines of
some large negative integer. In release mode it would have been some random number.
	Go ahead and change item_count so that item_count = 0; and then run the program.
If everything was done correctly, the test should now pass, because 0 is equal to item_count, which
is also 0.

Now that was a super simple example, and this didn't test anything in our project 'CryptRelay', but there was something that should definately be avoided in that example. Notice how, in the TEST() we created, we made a Backpack class instance. This is bad for a number of reasons, like and such bla
this guide to be continued.................



# Now let's test something from the program, CryptRelay.

Step 1: Include header files

 #include "PortKnock.h"
 #include "XBerkeleySockets.h"



Step 2: Create a TEST

TEST(PortKnockTest, PortIsNotInUse)
{
 	// Making a class instance every time is not ideal, and should be avoided.
	XBerkeleySockets XBerkeleySocketsInstance;
	PortKnock PK(&XBerkeleySocketsInstance);

	EXPECT_EQ(0, PK.isLocalPortInUse("30000", "192.168.1.115"));
}

This test is expecting a return value of 0 from the function PK.isLocalPortInUse();
Change the port and IP to whatever applies to you.



Step 3:




