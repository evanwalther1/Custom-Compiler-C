//-----------------------------------------------------------
// Dr. Art Hanna
// SPL Reader "driver" program
// SPLReader.cpp
//-----------------------------------------------------------
#include <iostream>
#include <iomanip>

#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <vector>

using namespace std;

#define TRACEREADER

#include "Prism.h"

//-----------------------------------------------------------
int main()
//-----------------------------------------------------------
{
   void Callback1(int sourceLineNumber,const char sourceLine[]);
   void Callback2(int sourceLineNumber,const char sourceLine[]);

   char sourceFileName[80+1];
   NEXTCHARACTER nextCharacter;

   READER<CALLBACKSUSED> reader(SOURCELINELENGTH,LOOKAHEAD);
//   READER<CALLBACKSUSED> reader(5,LOOKAHEAD);
   LISTER lister(LINESPERPAGE);
//   READER<CALLBACKSUSED> reader(SOURCELINELENGTH,LOOKAHEAD);

   cout << "Source filename? ";
   cin >> sourceFileName;

   try
   {
      lister.OpenFile(sourceFileName);
      reader.SetLister(&lister);
      reader.AddCallbackFunction(Callback1);
      reader.AddCallbackFunction(Callback2);
      reader.OpenFile(sourceFileName);

      do
      {
         nextCharacter = reader.GetNextCharacter();
      } while ( nextCharacter.character != READER<CALLBACKSUSED>::EOPC );
//      } while ( nextCharacter.character != READER::EOPC );
   }
   catch (PRISMEXCEPTION prismException)
   {
      cout << "SPL exception: " << prismException.GetDescription() << endl;
   }
   lister.ListInformationLine("******* PRISM reader ending");
   cout << "PRISM reader ending\n";

   system("PAUSE");
   return( 0 );
}

//-----------------------------------------------------------
void Callback1(int sourceLineNumber,const char sourceLine[])
//-----------------------------------------------------------
{
   cout << setw(4) << sourceLineNumber << " ";
}

//-----------------------------------------------------------
void Callback2(int sourceLineNumber,const char sourceLine[])
//-----------------------------------------------------------
{
   cout << sourceLine << endl;
}
