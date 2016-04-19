This is a rough, rambling, first draft for building miniupnp 1.9
I wrote this out while trying to get it build this myself and ran into several problems. Everything below is the method that worked for me.

how to build miniupnp for Windows / this tutorial was done with miniupnp 1.9, visual studio 2015, and Windows 7


********** Building Microsoft Visual Studio 2015 *************

1. create a txt file and rename it to miniupnpcstrings.h

2. paste this into it:
/* Project: miniupnp
* http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
* Author: Thomas Bernard
* Copyright (c) 2005-2009 Thomas Bernard
* This software is subjects to the conditions detailed
* in the LICENCE file provided within this distribution */
#ifndef __MINIUPNPCSTRINGS_H__
#define __MINIUPNPCSTRINGS_H__
	 
#define OS_STRING "Windows/7.0.0000"
#define MINIUPNPC_VERSION_STRING "1.9"

#endif


3. in the miniupnpcstrings.h file that you created, you will see the line: #define MINIUPNPC_VERSION_STRING "1.9" ... I think this is the version of the library you are using. who knows. I don't. I set it to 1.9. A forum post from long ago had it at 1.4  ... and i'm guessing OS_STRING is just your windows version probably need to change it accordingly, idk

4. open up visual studio solution in the msvc folder

5. set to release mode and build miniupnp.c PROJECT

6. set to debug mode and build miniupnp.c PROJECT


If you get some wierd errors, try re downloading and extract it again, do the whole process over
I actually had corrupted files a couple times! 


********* Including / Linking Visual Studio 2015 *************
Steps to take for including it / linking it in your program (visual studio 2015)


In your program put these 2 lines b/c they are needed by the miniupnp library to work:
	#pragma comment(lib, "Ws2_32.lib")
	#pragma comment(lib, "Iphlpapi.lib")
If you were getting linker errors the cause might have been the two missing #pragma comment

1. Set your configuration to Release and then follow the instructions:

2. in project settings, Linker -> Input -> Additional Dependencies -> miniupnpc-1.9\msvc\Release\miniupnpc.lib
	please note that it needs to be a full path to the miniupnpc.lib. Feel free to use macros. Macros not shown in example:
	c:\dev\workspace\myproject\miniupnpc-1.9\msvc\Release\miniupnpc.lib

3. in project settings, C/C++ -> Preprocessor -> Preprocessor Definitions -> STATICLIB
	please note we are just adding STATICLIB to the list of whatever is currently there. Don't delete everything in there.

4. in project settings, VC++ Directories -> Include Directories -> \miniupnpc-1.9
	please note that it needs to be a full path to the folder. Feel free to use macros. Macros not shown in example:
	C:\dev\workspace\myproject\miniupnpc-1.9

5. in project settings, VC++ Directories -> Include Directories -> \miniupnpc-1.9
	please note that it needs to be a full path to the folder. Feel free to use macros. Macros not shown in example:
	C:\dev\workspace\myproject\miniupnpc-1.9\msvc\Release

6. Set your configuration to Debug and do steps 1 through 5 all over again, but replaces any mention of "Release" with "Debug".
	Make sure you changed the additional dependencies directory paths to have debug in it too.



If you want to static link the library with your program:

1. In your program, project settings -> c/c++ -> code generation -> Runtime Library -> change it to Multi-Threaded (/MT) if you are on Release, and Multi-Threaded Debug (/MTd) if you are on debug

2. In the miniupnp library, do the exact same thing. It is important to keep the same static linking settings in your libraries and project.

If you get the linker error 4098 defaultlib 'MSVCRT' conflicts with use of other libs. If you google this is presented as a possible solution (it isn't for this case),
	but if you want to do it go to: project properties->linker->command line-> and type this:  /NODEFAULTLIB:library
	But really the problem is likely you are trying to statically link your program with some libraries. Doing step 1 set your program to statically link with its libraries.
	The problem is that your program is using a c runtime library, and the library is using a c runtime library. They are conflicting with eachother.
	In order to fix this linker error we must make sure our libraries are also statically linking with their libraries that are used.

static linking NOTES:
I had visual studio warn me that .pdb files were missing when I built my project in release. I solved this by
deleting the miniupnp library, extracting it, and building it all over again. This time it was only built as
a static library (/MT), (/MTd) instead of building it as non-static and a static.
This may not be the actualy solution / reason for the warnings, but it got it to work for me.




****** to include / link in Netbeans 8.1 ************

To build in a linux environment (not Netbeans!)
1. navigate to miniupnp-1.9 folder
2. in the command line type: make
	or gmake, depending on what you have


To include in Netbeans:

1. File->project properties-> build-> c++ compiler-> include directories-> navigate to miniupnp-1.9 folder and select the folder.

2. File-> project properties-> build-> linker-> additional library dependencies-> navigate to miniupnp-1.9 folder and select the folder.

3. File->Project properties->build->linker->libaries->Add Library File-> navigate to the miniupnp-1.9 folder, and select the libminiupnpc.a file.
