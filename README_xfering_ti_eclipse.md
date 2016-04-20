File->new->C++ project->
Poject Name: CryptRelay
Project type: Empty project
Toolchain: Linux GCC
Debug and Release are check-marked.

Click finish.

Doing this created a project folder in your workspace.
Copy all CryptRelay source files you have downloaded from the remote git repo and paste them into your new CryptRelay project folder.

Now back in Eclipse right click on your Project folder, and click refresh.
If you don't refresh Eclipse will NEVER know about it. Ever.

Warning: not having source files, or the eclipse project files inside the main CryptRelay Project directory can cause unknown problems. If you change the positioning and then revert it back to how it was originally, the program will have unknown build errors.
So just dump everything into the main CryptRelay project folder.

Now for proper setup in Eclipse:

Extract the miniupnpc-1.9.tar.gz into your /usr/lib
Use this if you don't know how: sudo tar xvzf miniupnpc-1.9.tar.gz -C /usr/lib/

then when you are in that folder inside a terminal, type sudo make

1. right click the project folder->properties->c/c++build/settings/GCC C++ compiler/miscellaneous/other flags/ now add on to the end of whatever is currently there (don't delete!) this line: -std=c++14

2. Window->preferences->C/C++->build->settings->discovery tab-> CDT GCC Built-in Compiler Settings-> and add this: -std=c++14
for example:       ${COMMAND} ${FLAGS} -E -P -v -dD -std=c++14 "${INPUTS}"
Please note that doing this changes the setting for your entire workspace, not just this project.

Now we must include libraries:
1. project properties-> c/c++build-> settings-> gcc c++ compiler-> includes->include paths (-l)-> click the add new button and navigate to and select the miniupnpc-1.9 folder.

2. project properties->c/c++build->settings-> GCC C++ Linker->Libraries-> 
	in the Libraries (-l) section put this:
	miniupnpc		//PLEASE NOTE: this does not say libminiupnpc
				//Nor does it say libminiupnpc.a nor libminiupnpc.so
				//Nor miniupnpc.a or .so
				//In order to link to a library, you must not mention the
				// 'lib' prefix on the file.
				// so libminiupnpc should be mentioned as miniupnpc
				// And don't mention any extension such as .a or .so


2.1 in the Libraries search path (-L) section navigate to your miniupnpc-1.9 folder and select it.

Now we must link to the pthreads library:
1. project properties->c/c++build->settings-> GCC C++ Linker->Libraries-> 
	in the Libraries (-l) section put this:
	pthread
1.1 the library search path -L shouldn't be necessary to input as it already checks /usr/lib/   for libraries


Now you will probably get an error something like this:
	 Path for project must have only one segment

To fix this we must go to   run->run configurations-> c/c++ Application-> your project name here-> look over to the right and click on the Main tab → go to the second line below where it says Project:-> click browse button, and select your project folder. Hit apply.

Make sure verything was done for release and for debug configurations


finally we do a clean and build for the index and the project. To do the index,
project->c/c++ Index-> rebuild
