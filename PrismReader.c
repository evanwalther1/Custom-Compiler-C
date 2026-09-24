//-----------------------------------------------------------
// Dr. Art Hanna
// SPL Reader (C-style, old-school version)
// ***Not updated with call back function logic***
// SPLReader.c
//-----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#define TRACEREADER

#define SOURCELINELENGTH  512
#define LINESPERPAGE       55
#define LOOKAHEAD           3
#define EOPC                0
#define EOLC             '\n'
#define TABC             '\t'

//--------------------------------------------------
// Global variables
//--------------------------------------------------
FILE *SOURCE,*LIST;
char sourceFileName[SOURCELINELENGTH];
char sourceLine[SOURCELINELENGTH+1],nextCharacters[LOOKAHEAD+1];
int  sourceLineIndex,sourceLineNumber,linesOnPage,pageNumber;
bool atEOP;

//-----------------------------------------------------------
int main()
//-----------------------------------------------------------
{
   void ListTopOfPageHeader();
   char GetNextCharacter();
   void ReadSourceLine();
   void ListInformationLine(char information[]);

   char fullFileName[SOURCELINELENGTH];
   char nextCharacter;
   int i;

   printf("Source filename? ");
   scanf("%s",sourceFileName);
   strcpy(fullFileName,sourceFileName);
   strcat(fullFileName,".spl");
   if ( (SOURCE = fopen(fullFileName,"r")) == NULL )
   {
      printf("Error opening source file \"%s \"\n",fullFileName);
      system("PAUSE");
      exit( 1 );
   }

   strcpy(fullFileName,sourceFileName);
   strcat(fullFileName,".list");
   if ( (LIST = fopen(fullFileName,"w")) == NULL )
   {
      printf("Error opening list file ""%s ""\n",fullFileName);
      system("PAUSE");
      exit( 1 );
   }
   atEOP = false;
   pageNumber = 0;
   linesOnPage = 0;
   sourceLineNumber = 0;
   ListTopOfPageHeader();
// Read first source line and "fill" nextCharacters[] 
   ReadSourceLine();
   for (i = 0; i <= LOOKAHEAD; i++)
      nextCharacters[i] = EOPC;
   for (i = 1; i <= LOOKAHEAD; i++)
      GetNextCharacter();
   do
   {
      nextCharacter = GetNextCharacter();
   } while ( nextCharacter != EOPC );
   ListInformationLine("******* SPL reader ending");
   printf("SPL reader ending\n");
   fclose(SOURCE);
   fclose(LIST);
   system("PAUSE");
   return( 0 );
}

//--------------------------------------------------
char GetNextCharacter()
//--------------------------------------------------
{
   void ReadSourceLine();
   void ListInformationLine(char information[]);

   char nextCharacter;
   int i;

   for (i = 1; i <= LOOKAHEAD; i++)
      nextCharacters[i-1] = nextCharacters[i];

   if ( atEOP )
      nextCharacter = EOPC;
   else
   { 
      if ( sourceLineIndex <= ((int) strlen(sourceLine)-1) )
      {
         nextCharacter = sourceLine[sourceLineIndex];
         sourceLineIndex += 1;
      }
      else
      {
         nextCharacter = EOLC;
         ReadSourceLine();
      }
   }

// Only non-printable characters allowed are EOPC,'\n', and '\t', others are changed to ' '
   if ( iscntrl(nextCharacter) 
    && !(    (nextCharacter == EOPC)
          || (nextCharacter == EOLC) 
          || (nextCharacter == TABC) 
        )
      )
      nextCharacter = ' ';

   nextCharacters[LOOKAHEAD] = nextCharacter;

#ifdef TRACEREADER
{
   char information[80+1];

   if      ( isprint(nextCharacters[0]) )
      sprintf(information,"%02X = %c",nextCharacters[0],nextCharacters[0]);
   else if ( nextCharacters[0] == EOPC )
      sprintf(information,"%02X = EOPC",nextCharacters[0]);
   else if ( nextCharacters[0] == EOLC )
      sprintf(information,"%02X = EOLC",nextCharacters[0]);
   else if ( nextCharacters[0] == TABC )
      sprintf(information,"%02X = TABC",nextCharacters[0]);
   else
      sprintf(information,"%02X = ???",nextCharacters[0]);
   ListInformationLine(information);
}
#endif

   return( nextCharacters[0] );
}

//-----------------------------------------------------------
char GetLookAheadCharacter(int index)
//-----------------------------------------------------------
{
// index in [ 0,LOOKAHEAD ] where index = 0 means last GetNextCharacter() returned
   return( nextCharacters[index] );
}

//--------------------------------------------------
void ReadSourceLine()
//--------------------------------------------------
{
   void ListSourceLine(char sourceLine[]);
   void ListInformationLine(char information[]);

   if ( feof(SOURCE) )
      atEOP = true;
   else
   {
      if ( fgets(sourceLine,SOURCELINELENGTH,SOURCE) == NULL )
         atEOP = true;
      else
      {
         if ( (strchr(sourceLine,'\n') == NULL) && !feof(SOURCE) )
         {
            ListInformationLine("******* Source line too long!");
         }
      // Erase *ALL* control characters at end of source line (if any)
         while ( (0 <= (int) strlen(sourceLine)-1) && 
                 iscntrl(sourceLine[(int) strlen(sourceLine)-1]) )
            sourceLine[(int) strlen(sourceLine)-1] = '\0';
         sourceLineIndex = 0;
         ListSourceLine(sourceLine);
      }
   }
}

//-----------------------------------------------------------
void ListTopOfPageHeader()
//-----------------------------------------------------------
{
/*
"Source file name" Page XXXX
Line Source Line
---- -------------------------------------------------------------------------------
*/
   const char FF = 0X0C;

   pageNumber++;
   fprintf(LIST,"%c\"%s\" Page %4d\n",FF,sourceFileName,pageNumber);
   fprintf(LIST,"Line Source Line\n");
   fprintf(LIST,"---- -------------------------------------------------------------------------------\n");
   fflush(LIST);
}

//-----------------------------------------------------------
void ListSourceLine(char sourceLine[])
//-----------------------------------------------------------
{
   void ListTopOfPageHeader();

   sourceLineNumber++;
   if ( linesOnPage >= LINESPERPAGE )
   {
      ListTopOfPageHeader();
      linesOnPage = 0;
   }
   fprintf(LIST,"%4d %s\n",sourceLineNumber,sourceLine); fflush(LIST);
   linesOnPage++;
}

//-----------------------------------------------------------
void ListInformationLine(char information[])
//-----------------------------------------------------------
{
   void ListTopOfPageHeader();

   if ( linesOnPage >= LINESPERPAGE )
   {
      ListTopOfPageHeader();
      linesOnPage = 0;
   }
   fprintf(LIST,"%s\n",information); fflush(LIST);
   linesOnPage++;
}
