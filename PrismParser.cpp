//-----------------------------------------------------------
// Evan Walther
// Prism1 Parser
// PrismParser.cpp
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

//#define TRACEREADER
#define TRACESCANNER
#define TRACEPARSER

#include "Prism.h"

//-----------------------------------------------------------
typedef enum
//-----------------------------------------------------------
{
// pseudo-terminals
   IDENTIFIER,
   STRING,
   EOPTOKEN,
   UNKTOKEN,
// reserved words
   MAIN,
   DISPLAY,
   NEW_LINE,
// punctuation
   DISPLAYOPEN,
   DISPLAYCLOSE,
   DASH,
   SEMICOLON,
   OPENCURLY,
   CLOSEDCURLY
// operators
// ***NONE***
} TOKENTYPE;

//-----------------------------------------------------------
struct TOKENTABLERECORD
//-----------------------------------------------------------
{
   TOKENTYPE type;
   char description[12+1];
   bool isReservedWord;
};

//-----------------------------------------------------------
const TOKENTABLERECORD TOKENTABLE[] =
//-----------------------------------------------------------
{
   { IDENTIFIER  ,"IDENTIFIER"  ,false },
   { STRING      ,"STRING"      ,false },
   { EOPTOKEN    ,"EOPTOKEN"    ,false },
   { UNKTOKEN    ,"UNKTOKEN"    ,false },
   { MAIN        ,"MAIN"     ,   true  },
   { DISPLAY      ,"DISPLAY"    ,true  },
   { DISPLAYOPEN , "DISPLAYOPEN", false},
   { DISPLAYCLOSE, "DISPLAYCLOSE", false },
   { NEW_LINE       ,"NEW_LINE"        ,true  },
   { SEMICOLON       ,"SEMICOLON"       ,false },
   { OPENCURLY      ,"OPENCURLY"      ,false },
   { CLOSEDCURLY    , "CLOSEDCURLEY" , false },
   { DASH,          "DASH" , false }
};

//-----------------------------------------------------------
struct TOKEN
//-----------------------------------------------------------
{
   TOKENTYPE type;
   char lexeme[SOURCELINELENGTH+1];
   int sourceLineNumber;
   int sourceLineIndex;
};

//--------------------------------------------------
// Global variables
//--------------------------------------------------
READER<CALLBACKSUSED> reader(SOURCELINELENGTH,LOOKAHEAD);
LISTER lister(LINESPERPAGE);

#ifdef TRACEPARSER
int level;
#endif

//-----------------------------------------------------------
void EnterModule(const char module[])
//-----------------------------------------------------------
{
#ifdef TRACEPARSER
   char information[SOURCELINELENGTH+1];

   level++;
   sprintf(information,"   %*s>%s",level*2," ",module);
   lister.ListInformationLine(information);
#endif
}

//-----------------------------------------------------------
void ExitModule(const char module[])
//-----------------------------------------------------------
{
#ifdef TRACEPARSER
   char information[SOURCELINELENGTH+1];

   sprintf(information,"   %*s<%s",level*2," ",module);
   lister.ListInformationLine(information);
   level--;
#endif
}

//--------------------------------------------------
void ProcessCompilerError(int sourceLineNumber,int sourceLineIndex,const char errorMessage[])
//--------------------------------------------------
{
   char information[SOURCELINELENGTH+1];

// Use "panic mode" error recovery technique: report error message and terminate compilation!
   sprintf(information,"     At (%4d:%3d) %s",sourceLineNumber,sourceLineIndex,errorMessage);
   lister.ListInformationLine(information);
   lister.ListInformationLine("SPL compiler ending with compiler error!\n");
   throw( PRISMEXCEPTION("SPL compiler ending with compiler error!") );
}

//-----------------------------------------------------------
int main()
//-----------------------------------------------------------
{
   void Callback1(int sourceLineNumber,const char sourceLine[]);
   void Callback2(int sourceLineNumber,const char sourceLine[]);
   void GetNextToken(TOKEN tokens[]);
   void ParseSPLProgram(TOKEN tokens[]);

   char sourceFileName[80+1];
   TOKEN tokens[LOOKAHEAD+1];
   
   cout << "Source filename? ";
   cin >> sourceFileName;

   try
   {
      lister.OpenFile(sourceFileName);
      reader.SetLister(&lister);
      reader.AddCallbackFunction(Callback1);
      reader.AddCallbackFunction(Callback2);
      reader.OpenFile(sourceFileName);

   // Fill tokens[] for look-ahead
      for (int i = 0; i <= LOOKAHEAD; i++)
         GetNextToken(tokens);

#ifdef TRACEPARSER
      level = 0;
#endif
   
      ParseSPLProgram(tokens);
   }
   catch (PRISMEXCEPTION prismException)
   {
      cout << "PRISM exception: " << prismException.GetDescription() << endl;
   }
   lister.ListInformationLine("******* PRISM parser ending");
   cout << "PRISM parser ending\n";

   system("PAUSE");
   return( 0 );
   
}

//-----------------------------------------------------------
void ParseSPLProgram(TOKEN tokens[])
//-----------------------------------------------------------
{
   void GetNextToken(TOKEN tokens[]);
   void ParsePROGRAMDefinition(TOKEN tokens[]);

   EnterModule("SPLProgram");

   if ( tokens[0].type == MAIN ){

	   GetNextToken(tokens);
   	   if(tokens[0].type != OPENCURLY){
   		 ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
   		                           "Expecting '{'");
   	   } else {
      ParsePROGRAMDefinition(tokens);
   	   }

   } else
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Expecting MAIN");

   if ( tokens[0].type != EOPTOKEN )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Expecting end-of-program");

   ExitModule("SPLProgram");
}

//-----------------------------------------------------------
void ParsePROGRAMDefinition(TOKEN tokens[])
//-----------------------------------------------------------
{
   void GetNextToken(TOKEN tokens[]);
   void ParseStatement(TOKEN tokens[]);

   EnterModule("PROGRAMDefinition");

   GetNextToken(tokens);

   while ( tokens[0].type != CLOSEDCURLY )
      ParseStatement(tokens);

   GetNextToken(tokens);

   ExitModule("PROGRAMDefinition");
}

//-----------------------------------------------------------
void ParseStatement(TOKEN tokens[])
//-----------------------------------------------------------
{
   void GetNextToken(TOKEN tokens[]);
   void ParseDISPLAYStatement(TOKEN tokens[]);

   EnterModule("Statement");

   switch ( tokens[0].type )
   {
      case DISPLAY:
         ParseDISPLAYStatement(tokens);
         break;
      default:
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                              "Expecting beginning-of-statement");
         break;
   }

   ExitModule("Statement");
}

//-----------------------------------------------------------
void ParseDISPLAYStatement(TOKEN tokens[])
//-----------------------------------------------------------
{
   void GetNextToken(TOKEN tokens[]);

   EnterModule("PRINTStatement");



	GetNextToken(tokens);
	 if(tokens[0].type != DISPLAYOPEN){
	  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex, "Expecting '<' ");
	 } else {
	 GetNextToken(tokens);
	 }
	 // Expect first STRING or NEW_LINE
	 switch (tokens[0].type)
	 {
		case STRING:
			GetNextToken(tokens);
			break;
		case NEW_LINE:
		   GetNextToken(tokens);
		   break;
		default:
		   ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex,
								"Expecting string or NEW_LINE");
	 }

	 // Handle any number of "- STRING" or "- NEW_LINE"
	 while (tokens[0].type == DASH)
	 {
		 GetNextToken(tokens); // consume '-'

		 if (tokens[0].type == STRING || tokens[0].type == NEW_LINE)
		 {
			 GetNextToken(tokens); // consume the STRING or NEW_LINE
		 }
		 else
		 {
			 ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex,
								  "Expecting string or NEW_LINE after '-'");
		 }
	 }


   if(tokens[0].type != DISPLAYCLOSE){
	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
	                              "Expecting '>'");
   } else {
	   GetNextToken(tokens);
   }

   if ( tokens[0].type != SEMICOLON )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Expecting ';'");

   GetNextToken(tokens);

   ExitModule("PRINTStatement");
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

//-----------------------------------------------------------
void GetNextToken(TOKEN tokens[])
//-----------------------------------------------------------
{
   const char *TokenDescription(TOKENTYPE type);

   int i;
   TOKENTYPE type;
   char lexeme[SOURCELINELENGTH+1];
   int sourceLineNumber;
   int sourceLineIndex;
   char information[SOURCELINELENGTH+1];

//===========================================================
// Move look-ahead "window" to make room for next token-and-lexeme
//===========================================================
   for (int i = 1; i <= LOOKAHEAD; i++)
      tokens[i-1] = tokens[i];

   char nextCharacter = reader.GetLookAheadCharacter(0).character;

//===========================================================
// "Eat" white space and comments
//===========================================================
   do
   {
//    "Eat" any white-space (blanks and EOLCs and TABCs) 
      while ( (nextCharacter == ' ')
           || (nextCharacter == READER<CALLBACKSUSED>::EOLC)
           || (nextCharacter == READER<CALLBACKSUSED>::TABC) )
         nextCharacter = reader.GetNextCharacter().character;


      //check for block comments first
      if ( (nextCharacter == '$') && (reader.GetLookAheadCharacter(1).character == '~') )
            {
            #ifdef TRACESCANNER
               sprintf(information,"At (%4d:%3d) begin prism block comment (no nesting allowed)",
                  reader.GetLookAheadCharacter(0).sourceLineNumber,
                  reader.GetLookAheadCharacter(0).sourceLineIndex);
               lister.ListInformationLine(information);
            #endif

               // Eat "$~"
               nextCharacter = reader.GetNextCharacter().character; // eat '$'
               nextCharacter = reader.GetNextCharacter().character; // eat '~'

               // Loop until "~$"
               do {
                  nextCharacter = reader.GetNextCharacter().character;
               } while (!(nextCharacter == '~' && reader.GetLookAheadCharacter(1).character == '$')
                        && (nextCharacter != READER<CALLBACKSUSED>::EOPC));

               // At this point, nextCharacter == '~' and lookahead is '$'
               nextCharacter = reader.GetNextCharacter().character; // eat '$'

            #ifdef TRACESCANNER
               sprintf(information,"At (%4d:%3d)   end prism block comment (no nesting allowed)",
                  reader.GetLookAheadCharacter(0).sourceLineNumber,
                  reader.GetLookAheadCharacter(0).sourceLineIndex);
               lister.ListInformationLine(information);
            #endif
            }
//    Next eat line comments
      if ( nextCharacter == '$' )
      {

#ifdef TRACESCANNER
   sprintf(information,"At (%4d:%3d) begin line comment",
      reader.GetLookAheadCharacter(0).sourceLineNumber,
      reader.GetLookAheadCharacter(0).sourceLineIndex);
   lister.ListInformationLine(information);
#endif

         do
            nextCharacter = reader.GetNextCharacter().character;
         while ( (nextCharacter != READER<CALLBACKSUSED>::EOLC) 
              && (nextCharacter != READER<CALLBACKSUSED>::EOPC) );
      } 

      // Eat block comments (no nesting allowed)

   } while ( (nextCharacter == ' ')
          || (nextCharacter == READER<CALLBACKSUSED>::EOLC)
          || (nextCharacter == READER<CALLBACKSUSED>::TABC)
          || (nextCharacter == '$')
          || ((nextCharacter == '~') && (reader.GetLookAheadCharacter(1).character == '$')) );

//===========================================================
// Scan token
//===========================================================
   sourceLineNumber = reader.GetLookAheadCharacter(0).sourceLineNumber;
   sourceLineIndex = reader.GetLookAheadCharacter(0).sourceLineIndex;

// reserved words (and <identifier>

   //
   if ( isalpha(nextCharacter) )
   {

      char UCLexeme[SOURCELINELENGTH+1];
      i = 0;
      lexeme[i++] = nextCharacter;
      nextCharacter = reader.GetNextCharacter().character;
      //While there are more character, add them to the lexeme and then terminate it
      while ( isalpha(nextCharacter) || isdigit(nextCharacter) || (nextCharacter == '_') )
      {
         lexeme[i++] = nextCharacter;
         nextCharacter = reader.GetNextCharacter().character;
      }
      lexeme[i] = '\0';
      for (i = 0; i <= (int) strlen(lexeme); i++)
         UCLexeme[i] = toupper(lexeme[i]);

      bool isFound = false;

      i = 0;
      while ( !isFound && (i <= (sizeof(TOKENTABLE)/sizeof(TOKENTABLERECORD))-1) )
      {
    	  //checks to see if the word we just created is in the token table
         if ( TOKENTABLE[i].isReservedWord && (strcmp(UCLexeme,TOKENTABLE[i].description) == 0) )
            isFound = true;
         else
            i++;
      }
      if ( isFound )
         type = TOKENTABLE[i].type;
      else
         type = IDENTIFIER;
   }
   else
   {
      switch ( nextCharacter )
      {
// <string> literal *Note* no escape character sequences supported but " embedded as ""
      case '"':
    	  i = 0;
		 nextCharacter = reader.GetNextCharacter().character;
		 while ( (nextCharacter != '"')
			  && (nextCharacter != READER<CALLBACKSUSED>::EOLC)
			  && (nextCharacter != READER<CALLBACKSUSED>::EOPC) )
		 {
			if ( nextCharacter == '\\' )
			{
			   lexeme[i++] = nextCharacter;
			   nextCharacter = reader.GetNextCharacter().character;
			   if ( (nextCharacter ==  'n') ||
					(nextCharacter ==  't') ||
					(nextCharacter ==  'b') ||
					(nextCharacter ==  'r') ||
					(nextCharacter == '\\') ||
					(nextCharacter ==  '"') )
			   {
				  lexeme[i++] = nextCharacter;
			   }
			   else
				  ProcessCompilerError(sourceLineNumber,sourceLineIndex,
									   "Illegal escape character sequence in string literal");
			}
			else
			{
			   lexeme[i++] = nextCharacter;
			}
			nextCharacter = reader.GetNextCharacter().character;
		 }
		 if ( nextCharacter != '"' )
			ProcessCompilerError(sourceLineNumber,sourceLineIndex,
								 "Un-terminated string literal");
		 lexeme[i] = '\0';
		 type = STRING;
		 reader.GetNextCharacter();
		 break;

//===========================================================
         case READER<CALLBACKSUSED>::EOPC: 
            {
               static int count = 0;
   
               if ( ++count > (LOOKAHEAD+1) )
                  ProcessCompilerError(sourceLineNumber,sourceLineIndex,
                                       "Unexpected end-of-program");
               else
               {
                  type = EOPTOKEN;
                  reader.GetNextCharacter();
                  lexeme[0] = '\0';
               }
            }
            break;
         case '<':
        	 type = DISPLAYOPEN;
			 lexeme[0] = nextCharacter;
        	 lexeme[1] = '\0';
			 reader.GetNextCharacter();
			 break;
         case '>':
        	 type = DISPLAYCLOSE;
        	 lexeme[0] = nextCharacter;
			 lexeme[1] = '\0';
			 reader.GetNextCharacter();
			 break;
         case ';':
            type = SEMICOLON;
            lexeme[0] = nextCharacter; lexeme[1] = '\0';
            reader.GetNextCharacter();
            break;
         case '{':
            type = OPENCURLY;
            lexeme[0] = nextCharacter; lexeme[1] = '\0';
            reader.GetNextCharacter();
            break;
         case '}':
        	 type = CLOSEDCURLY;
        	 lexeme[0] = nextCharacter;
        	 lexeme[1] = '\0';
        	 reader.GetNextCharacter();
        	 break;
         case '-':
        	 type = DASH;
        	 lexeme[0] = nextCharacter;
        	 lexeme[1] = '\0';
        	 reader.GetNextCharacter();
        	 break;
         default:  
            type = UNKTOKEN;
            lexeme[0] = nextCharacter; lexeme[1] = '\0';
            reader.GetNextCharacter();
            break;
      }
   }

   tokens[LOOKAHEAD].type = type;
   strcpy(tokens[LOOKAHEAD].lexeme,lexeme);
   tokens[LOOKAHEAD].sourceLineNumber = sourceLineNumber;
   tokens[LOOKAHEAD].sourceLineIndex = sourceLineIndex;

#ifdef TRACESCANNER
   sprintf(information,"At (%4d:%3d) token = %12s lexeme = |%s|",
      tokens[LOOKAHEAD].sourceLineNumber,
      tokens[LOOKAHEAD].sourceLineIndex,
      TokenDescription(type),lexeme);
   lister.ListInformationLine(information);
#endif

}

//-----------------------------------------------------------
const char *TokenDescription(TOKENTYPE type)
//-----------------------------------------------------------
{
   int i;
   bool isFound;
   
   isFound = false;
   i = 0;
   while ( !isFound && (i <= (sizeof(TOKENTABLE)/sizeof(TOKENTABLERECORD))-1) )
   {
      if ( TOKENTABLE[i].type == type )
         isFound = true;
      else
         i++;
   }
   return ( isFound ? TOKENTABLE[i].description : "???????" );
}
