# Visual Studio
How to build / link Cryptopp library (Crypto C++) for Microsoft Visual Studio 2015. This guide will be using the cryptopp library version 5.6.3. The official website as of August 8, 2016 is: https://www.cryptopp.com/


1. Unzip the cryptopp563.zip into a folder. I named mine cryptopp, and I will hearby refer to that folder as cryptopp folder or /cryptopp/. Open up /cryptopp/vs2010.zip and read the Readme-VS2010.txt located in the vs2010.zip.

2. For the sake of this guide, we are going to do as the readme says, and extract the vs2010.zip into the /cryptopp/ folder.

3. open up the cryptest.sln

4. Build solution as debug.

5. Build solution as release.

6. in project settings, Linker -> Input -> Additional Dependencies -> 





# Special notes:
Cryptopp uses static C/C++ runtime linking. If you need to use Crypto++ with other class libraries, like ATL, MFC or QT, then you will likely encounter problems due to mixing and matching C++ runtime libraries. Please consult https://www.cryptopp.com/wiki/Visual_Studio
under the section 'Dynamic Runtime Linking' for more information about what to do in this case. Also read the Readme.txt located in the cryptopp repository or zip, wherever you got it.

*** Important Usage Notes ***

1. If a constructor for A takes a pointer to an object B (except primitive
types such as int and char), then A owns B and will delete B at A's
destruction.  If a constructor for A takes a reference to an object B,
then the caller retains ownership of B and should not destroy it until
A no longer needs it. 

2. Crypto++ is thread safe at the class level. This means you can use
Crypto++ safely in a multithreaded application, but you must provide
synchronization when multiple threads access a common Crypto++ object.



