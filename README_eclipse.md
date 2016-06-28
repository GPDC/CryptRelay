# This guide was created for use w/ Debian Linux & Eclipse 3.8.1
Want to use Eclipse to build / make changes to CryptRelay? This is the README for you.

# If you are new to programming on linux you might need to download and install a couple packages that contain some tools / libraries:
sudo apt-get install build-essential
Want to know what separate packages make up the build-essential package? Type:
gedit /usr/share/doc/build-essential/essential-packages-list
Also make sure you have dbg installed or else you can't do a lot of debugging. To do that type: apt-get install dbg
When you download eclipse using apt-get install, make sure to install the one called: eclipse-cdt

# get the files from github
First: git clone https://github.com/GPDC/CryptRelay.git master
Second: git pull https://github.com/GPDC/CryptRelay.git master

# Make an Eclipse project
Now open up the Eclipse IDE.
Create a new project by doing this:
File->new->C++ project->
Poject Name: CryptRelay
Project type: Empty project
Toolchain: Linux GCC

Next page:
Debug and Release are check-marked.

Click finish.

Doing this created a project folder in your workspace.
Copy all CryptRelay source files you have downloaded from the remote git repo and paste them into your new CryptRelay project folder. Do not replace your existing, local .project and .cproject files with the ones that have been pulled from the repo (this guide doesn't cover dealing with existing eclipse project files yet).

# SUPER IMPORTANT
Now back in Eclipse right click on your Project folder, and click refresh.
If you don't refresh Eclipse will NEVER know about it. Ever. A lot of common problems with eclipse can be resolved by refreshing this. Remember this.

# Now for proper setup in Eclipse:
Make sure you do all steps for both Release and Debug configurations.

1. right click the project folder->properties->c/c++build/Tool Chain Editor/
and click on the Current toolchain drop down menu, and select cross gcc.

2. right click the project folder->properties->c/c++build/settings/Cross G++ Compiler/miscellaneous/other flags/ now add on to the end of whatever is currently there (don't delete anything!) this line: -std=c++14

3. Window->preferences->C/C++->build->settings->discovery tab-> CDT Cross GCC Built-in Compiler Settings-> and add this: -std=c++14
for example:       ${COMMAND} ${FLAGS} -E -P -v -dD -std=c++14 "${INPUTS}"
Please note that doing this changes the setting for your entire workspace, not just this project.

# Now for including libraries:
Look at README_miniupnp to see how to include the miniupnpc library. After having included the miniupnpc library, come back here and continue following instructions.

Lets include the pthread library:
1. project properties->c/c++build->settings-> Cross G++ Linker->Libraries-> 
	in the Libraries (-l) section put this:
	pthread
1.1 the library search path -L shouldn't be necessary to input as it already checks /usr/lib/   for libraries


# Important Ending:
Make sure you made all the changes for both Debug and Release configuration.
Try cleaning the project, building it, and then running it. If you get an error:

Now you will probably get an error something like this:
	 Path for project must have only one segment

// This part needs to be re-written
To fix this we must go to   run->run configurations-> c/c++ Application-> your project name here-> look over to the right and click on the Main tab ? go to the second line below where it says Project:-> click browse button, and select your project folder. Hit apply.
Continued next page...
Make sure everything was done for release AND also for debug configurations


# SUPER IMPORTANT
finally we do a clean and build for the project, and then the index. To do the index:
project->c/c++ Index-> rebuild
If you are getting a bunch of unresolved errors relating to the miniupnpc library, and it says all of them are Semantic Errors, then this should be the fix.



always, always remember to hit refresh on your project if you encounter some problems with the IDE.


# Now we must do even more things just to be able to Run the program!
You can either run the program by opening your main.cpp file and then and only then can you select Run (ctrl-f11).
OR you can set it up so that isn't needed to be done every time by going to here:
Run->run configurations->C/C++ Application-> your Project or application name should be here in the drop down menu from C/C++ Application, but if it isn't then double click C/C++ Application to create a new one.

# If your project was already in that drop down menu, then do these steps:
Look over to the right and click on the Main tab, now look down to the second line where it says Project: , now click browse, and select your poject folder. Hit apply.


# ELSE do these steps (after having created a new thing by double clicking)
Look over to the right and click on the Main tab.
Look down at the line with C/C++ Application:
Type in: Debug/CryptRelay
uhhhh not sure what to do past this because it looks like I should be making one for Release too... ,but when I run the program in release mode it works just fine too so idk

Look down at the line with Project:
click browse and select your Project folder.


# Want to run the program with command line arguments?
Run->run configuration-> c/c++ Application-> your project should be in the dropdown here.
Now look over to the right and click on the Arguments tab.
Type in your command line arguments here.
