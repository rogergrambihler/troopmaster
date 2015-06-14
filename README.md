# troopmaster

Troopmaster is used by Scout Troops for keeping track of Scouts advancement, outings, personal information, etc. Troopmaster provides a way for exporting some of the data but doesn't provide a way to export Outing information. Outing information is also lost when a Scout is archived

Goal of the Troopmaster project is to be able to read the Troopmaster data and provide utilities.

The code to read the Troopmaster data is in the code\TMAPI folder and is modeled after the SqlLite api. Not all Troopmaster tables or record columns are currently read.

There is a sample utility code\TMReader that converts the Troopmaster data into a SqlLite database.

Code is C++ and currently compiled for Windows using the free Visual Studio Community but code is organized so that it can be made portable to other platforms. To compile download Visual Studio Community and then open the .csproj in the code\TMReader folder.
