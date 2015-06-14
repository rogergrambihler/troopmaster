https://dcravey.wordpress.com/2011/03/21/using-sqlite-in-a-visual-c-application/

Using SQLite in a Native Visual C++ Application
Posted on March 21, 2011 by dcravey
As promised, here is the Native Visual C++ followup to my previous blog post.  I hope you find this helpful!

Create a new C++ Win32 Console application.
Download the native SQLite DLL from: http://sqlite.org/sqlite-dll-win32-x86-3070400.zip
Unzip the DLL and DEF files and place the contents in your project’s source folder (an easy way to find this is to right click on the tab and click the “Open Containing Folder” menu item.
Open a “Visual Studio Command Prompt (2010)” and navigate to your source folder.
Create an import library using the following command line: LIB /DEF:sqlite3.def
Add the library (i.e. sqlite3.lib) to your Project Properties -> Configuration Properties -> Linker -> Additional Dependencies.
Download http://sqlite.org/sqlite-amalgamation-3070400.zip
Unzip the sqlite3.h header file and place into your source directory.
Include the the sqlite3.h header file in your source code.
You will need to include the sqlite3.dll in the same directory as your program (or in a System Folder).
Alternatively you can add the sqlite3.c file in your project,  and the SQLite database engine will be included in your exe.  After you add sqlite3.c to your project (right click the project and choose to add an existing file), you will need to right click on the sqlite3.c file in the Solution Explorer and change: Properties->Configuration Properties->C/C++->Precompiled Headers->Precompiled Header = Not Using Precompiled Headers