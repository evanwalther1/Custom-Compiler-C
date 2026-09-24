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
   INTEGER,
   EOPTOKEN,
   UNKTOKEN,
// reserved words
   MAIN,
   DISPLAY,
   CONSOLEGET,
   FOR,
   ASSERT,
   NEW_LINE,
   DO,
   WHILE,
   OR,
   NOR,
   XOR,
   AND,
   NAND,
   NOT,
   ABS,
   TRUE,
   FALSE,
   INT,
   BOOL,
   PROCEDURE,
  IN,
  OUT,
  IO,
  REF,
  CALL,
  RETURN,
  FUNCTION,
// punctuation
   DISPLAYOPEN,
   DISPLAYCLOSE,
   DASH,
   SEMICOLON,
   OPENCURLY,
   CLOSEDCURLY,
   COLON,
   QUEMARK,
   QUEMARKELSE,
   OPARENTHESIS,
   ARROW,
      CPARENTHESIS,
// operators
	  LT,
	 LTEQ,
	 EQ,
	 GT,
	 GTEQ,
	 NOTEQ, // <> and !=
	 PLUS,
	 MINUS,
	 MULTIPLY,
	 DIVIDE,
	 MODULUS,
	 POWER  // ^ and **
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
   { INTEGER     ,"INTEGER"     ,false },
   { STRING      ,"STRING"      ,false },
   { EOPTOKEN    ,"EOPTOKEN"    ,false },
   { UNKTOKEN    ,"UNKTOKEN"    ,false },
   { MAIN        ,"MAIN"     ,   true  },
   { DISPLAY      ,"DISPLAY"    ,true  },
   { FOR , "FOR",                true   },
   { ARROW,  "ARROW"     ,        false  },
   { ASSERT      ,"ASSERT"      ,true  },
   { DO          ,"DO"          ,true  },
   { WHILE       ,"WHILE"       ,true  },
   { COLON		,"COLON"       ,false },
   { QUEMARK      ,"QUEMARK"     ,false },
   { QUEMARKELSE  ,"QUEMARKELSE" ,false },
   { DISPLAYOPEN , "DISPLAYOPEN", false},
   { DISPLAYCLOSE, "DISPLAYCLOSE", false },
   { NEW_LINE       ,"NEW_LINE"        ,true  },
   { SEMICOLON       ,"SEMICOLON"       ,false },
   { OPENCURLY      ,"OPENCURLY"      ,false },
   { CLOSEDCURLY    , "CLOSEDCURLEY" , false },
   { DASH,          "DASH" , false },
   { OR          ,"OR"          ,true  },
   { INT, "INT", true},
   { BOOL        ,"BOOL"        ,true  },
      { CONSOLEGET       ,"CONSOLEGET"       ,true  },
      { NOR         ,"NOR"         ,true  },
      { XOR         ,"XOR"         ,true  },
      { AND         ,"AND"         ,true  },
      { NAND        ,"NAND"        ,true  },
      { NOT         ,"NOT"         ,true  },
      { ABS         ,"ABS"         ,true  },
      { TRUE        ,"TRUE"        ,true  },
      { FALSE       ,"FALSE"       ,true  },
      { OPARENTHESIS,"OPARENTHESIS",false },
      { CPARENTHESIS,"CPARENTHESIS",false },
      { LT          ,"LT"          ,false },
      { LTEQ        ,"LTEQ"        ,false },
      { EQ          ,"EQ"          ,false },
      { GT          ,"GT"          ,false },
      { GTEQ        ,"GTEQ"        ,false },
	  { PROCEDURE   ,"PROCEDURE"   ,true  },
		{ IN          ,"IN"          ,true  },
		{ OUT         ,"OUT"         ,true  },
		{ IO          ,"IO"          ,true  },
		{ REF         ,"REF"         ,true  },
		{ CALL        ,"CALL"        ,true  },
		{ RETURN      ,"RETURN"      ,true  },
		{ FUNCTION    ,"FUNCTION"    ,true  },
      { NOTEQ       ,"NOTEQ"       ,false },
      { PLUS        ,"PLUS"        ,false },
      { MINUS       ,"MINUS"       ,false },
      { MULTIPLY    ,"MULTIPLY"    ,false },
      { DIVIDE      ,"DIVIDE"      ,false },
      { MODULUS     ,"MODULUS"     ,false },
      { POWER       ,"POWER"       ,false }
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
CODE code;
IDENTIFIERTABLE identifierTable(&lister,MAXIMUMIDENTIFIERS);

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
   lister.ListInformationLine("Prism compiler ending with compiler error!\n");
   throw( PRISMEXCEPTION("Prism compiler ending with compiler error!") );
}

//-----------------------------------------------------------
int main()
//-----------------------------------------------------------
{
   void Callback1(int sourceLineNumber,const char sourceLine[]);
   void Callback2(int sourceLineNumber,const char sourceLine[]);
   void GetNextToken(TOKEN tokens[]);
   void ParsePRISMProgram(TOKEN tokens[]);

   char sourceFileName[80+1];
   TOKEN tokens[LOOKAHEAD+1];

   cout << "Source filename? ";
   cin >> sourceFileName;

   try
   {
	   lister.OpenFile(sourceFileName);
	   code.OpenFile(sourceFileName);
// CODEGENERATION
      code.EmitBeginningCode(sourceFileName);
// ENDCODEGENERATION

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

      ParsePRISMProgram(tokens);

// CODEGENERATION
      code.EmitEndingCode();
// ENDCODEGENERATION

   }
   catch (PRISMEXCEPTION prismException)
      {
         cout << "PRISM exception: " << prismException.GetDescription() << endl;
      }
      lister.ListInformationLine("******* PRISM compiler ending");
      cout << "PRISM compiler ending\n";

      system("PAUSE");
      return( 0 );

   }

//-----------------------------------------------------------
void ParsePRISMProgram(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseDataDefinitions(TOKEN tokens[],IDENTIFIERSCOPE identifierScope);
   void ParsePROCEDUREDefinition(TOKEN tokens[]);
   void GetNextToken(TOKEN tokens[]);
   void ParseFUNCTIONDefinition(TOKEN tokens[]);
   void ParseMAINDefinition(TOKEN tokens[]);

   EnterModule("PrismProgram");

   ParseDataDefinitions(tokens,GLOBALSCOPE);

#ifdef TRACECOMPILER
   identifierTable.DisplayTableContents("Contents of identifier table after compilation of global data definitions");
#endif

   while ( (tokens[0].type == PROCEDURE) || (tokens[0].type ==  FUNCTION) )
      {
         switch ( tokens[0].type )
         {
            case PROCEDURE:
               ParsePROCEDUREDefinition(tokens);
               break;
            case FUNCTION:
               ParseFUNCTIONDefinition(tokens);
               break;
         }
      }


   if ( tokens[0].type == MAIN ){

	   GetNextToken(tokens);
   	   if(tokens[0].type != OPENCURLY){
   		 ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
   		                           "Expecting '{'");
   	   } else {
      ParseMAINDefinition(tokens);
   	   }

   } else
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Expecting MAIN");

   if ( tokens[0].type != EOPTOKEN )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Expecting end-of-program");

   ExitModule("PrismProgram");
}

//-----------------------------------------------------------
void ParseDataDefinitions(TOKEN tokens[], IDENTIFIERSCOPE identifierScope)
//-----------------------------------------------------------
{
    void GetNextToken(TOKEN tokens[]);

    EnterModule("DataDefinitions");



    while ((tokens[0].type == INT) || (tokens[0].type == BOOL))
    {
        DATATYPE datatype;


        if (tokens[0].type == INT)
            datatype = INTTYPE;
        else
            datatype = BOOLTYPE;


        GetNextToken(tokens);


        if (tokens[0].type != IDENTIFIER)
            ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Expecting identifier");

        // Process identifiers separated by '-'
        do
        {
            char identifier[MAXIMUMLENGTHIDENTIFIER + 1];
            char reference[MAXIMUMLENGTHIDENTIFIER + 1];
            bool isInTable;
            int index;

            // Expect identifier
            if (tokens[0].type != IDENTIFIER)
                ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Expecting identifier");

            strcpy(identifier, tokens[0].lexeme);
            GetNextToken(tokens); // move past identifier

            // Check for duplicates
            index = identifierTable.GetIndex(identifier, isInTable);
            if (isInTable && identifierTable.IsInCurrentScope(index))
                ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Multiply-defined identifier");

            // Add variable to table + generate code
            switch (identifierScope)
            {
                case GLOBALSCOPE:
                    // CODEGENERATION
                    code.AddRWToStaticData(1, identifier, reference);
                    // ENDCODEGENERATION
                    identifierTable.AddToTable(identifier, GLOBAL_VARIABLE, datatype, reference);
                    break;

                case PROGRAMMODULESCOPE:
                    // CODEGENERATION
                    code.AddRWToStaticData(1, identifier, reference);
                    // ENDCODEGENERATION
                    identifierTable.AddToTable(identifier, PROGRAMMODULE_VARIABLE, datatype, reference);
                    break;
                case SUBPROGRAMMODULESCOPE:
               // CODEGENERATION
                                    sprintf(reference,"FB:0D%d",code.GetFBOffset());
                                    code.IncrementFBOffset(1);
               // ENDCODEGENERATION
                                    identifierTable.AddToTable(identifier,SUBPROGRAMMODULE_VARIABLE,datatype,reference);
                                    break;
            }


            if (tokens[0].type == DASH)
                GetNextToken(tokens);

        } while (tokens[0].type == IDENTIFIER);


        if (tokens[0].type != SEMICOLON)
            ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Expecting ';'");

        GetNextToken(tokens);
    }

    ExitModule("DataDefinitions");
}


//-----------------------------------------------------------
void ParsePROCEDUREDefinition(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseFormalParameter(TOKEN tokens[],IDENTIFIERTYPE &identifierType,int &n);
   void ParseStatement(TOKEN tokens[]);
   void GetNextToken(TOKEN tokens[]);

   bool isInTable;
   char line[SOURCELINELENGTH+1];
   int index;
   char reference[SOURCELINELENGTH+1];

// n = # formal parameters, m = # words of "save-register" space and locally-defined variables/constants
   int n,m;
   char label[SOURCELINELENGTH+1],operand[SOURCELINELENGTH+1],comment[SOURCELINELENGTH+1];

   EnterModule("PROCEDUREDefinition");

   GetNextToken(tokens);

   if ( tokens[0].type != IDENTIFIER )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting identifier");

   index = identifierTable.GetIndex(tokens[0].lexeme,isInTable);
   if ( isInTable && identifierTable.IsInCurrentScope(index) )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Multiply-defined identifier");

   identifierTable.AddToTable(tokens[0].lexeme,PROCEDURE_SUBPROGRAMMODULE,NOTYPE,tokens[0].lexeme);

// CODEGENERATION
   code.EnterModuleBody(PROCEDURE_SUBPROGRAMMODULE,index);
   code.ResetFrameData();
   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** PROCEDURE module (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
   code.EmitFormattedLine(tokens[0].lexeme,"EQU","*");
// ENDCODEGENERATION

   identifierTable.EnterNestedStaticScope();

   GetNextToken(tokens);
   n = 0;
   if ( tokens[0].type == OPARENTHESIS )
   {
      do
      {
         IDENTIFIERTYPE identifierType;

         GetNextToken(tokens);
         ParseFormalParameter(tokens,identifierType,n);
      } while ( tokens[0].type == SEMICOLON );

      if ( tokens[0].type != CPARENTHESIS )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
      GetNextToken(tokens);
   }

   if(tokens[0].type != OPENCURLY)
	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
	                           "Expecting '{'");
   GetNextToken(tokens);

#ifdef TRACECOMPILER
   identifierTable.DisplayTableContents("Contents of identifier table after compilation of PROCEDURE module header");
#endif

// CODEGENERATION
   code.IncrementFBOffset(2); // makes room in frame for caller's saved FB register and the CALL return address
// ENDCODEGENERATION

   ParseDataDefinitions(tokens,SUBPROGRAMMODULESCOPE);

#ifdef TRACECOMPILER
   identifierTable.DisplayTableContents("Contents of identifier table after compilation of PROCEDURE local data definitions");
#endif

// CODEGENERATION
   m = code.GetFBOffset()-(n+2);
   code.EmitFormattedLine("","PUSHSP","","set PROCEDURE module FB = SP-on-entry + 2(n+2)");
   sprintf(operand,"#0D%d",2*(n+2));
   sprintf(comment,"n = %d",n);
   code.EmitFormattedLine("","PUSH",operand,comment);
   code.EmitFormattedLine("","ADDI");
   code.EmitFormattedLine("","POPFB");
   code.EmitFormattedLine("","PUSHSP","","PROCEDURE module SP = SP-on-entry - 2m");
   sprintf(operand,"#0D%d",2*m);
   sprintf(comment,"m = %d",m);
   code.EmitFormattedLine("","PUSH",operand,comment);
   code.EmitFormattedLine("","SUBI");
   code.EmitFormattedLine("","POPSP");
   code.EmitUnformattedLine("; statements to initialize frame data (if necessary)");
   code.EmitFrameData();
   sprintf(label,"MODULEBODY%04d",code.LabelSuffix());
   code.EmitFormattedLine("","CALL",label);
   code.EmitFormattedLine("","PUSHFB","","restore caller's SP-on-entry = FB - 2(n+2)");
   sprintf(operand,"#0D%d",2*(n+2));
   code.EmitFormattedLine("","PUSH",operand);
   code.EmitFormattedLine("","SUBI");
   code.EmitFormattedLine("","POPSP");
   code.EmitFormattedLine("","RETURN","","return to caller");
   code.EmitFormattedLine(label,"EQU","*");
   code.EmitUnformattedLine("; statements in body of PROCEDURE module (may include RETURN)");
// ENDCODEGENERATION

   while ( tokens[0].type != CLOSEDCURLY )
      ParseStatement(tokens);

// CODEGENERATION
   code.EmitFormattedLine("","RETURN");
   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** END (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
   code.ExitModuleBody();
// ENDCODEGENERATION

   identifierTable.ExitNestedStaticScope();

#ifdef TRACECOMPILER
   identifierTable.DisplayTableContents("Contents of identifier table at end of compilation of PROCEDURE module definition");
#endif

   GetNextToken(tokens);

   ExitModule("PROCEDUREDefinition");
}

//-----------------------------------------------------------
void ParseFUNCTIONDefinition(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseFormalParameter(TOKEN tokens[],IDENTIFIERTYPE &identifierType,int &n);
   void ParseStatement(TOKEN tokens[]);
   void GetNextToken(TOKEN tokens[]);

   bool isInTable;
   DATATYPE datatype;
   char identifier[SOURCELINELENGTH+1];
   char line[SOURCELINELENGTH+1];
   int index;
   char reference[SOURCELINELENGTH+1];

// n = # formal parameters, m = # words of return-value, "save-register" space, and locally-defined variables/constants
   int n,m;
   char label[SOURCELINELENGTH+1],operand[SOURCELINELENGTH+1],comment[SOURCELINELENGTH+1];

   EnterModule("FUNCTIONDefinition");

   GetNextToken(tokens);

   if ( tokens[0].type != IDENTIFIER )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting identifier");

   strcpy(identifier,tokens[0].lexeme);
   index = identifierTable.GetIndex(identifier,isInTable);
   if ( isInTable && identifierTable.IsInCurrentScope(index) )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Multiply-defined identifier");
   GetNextToken(tokens);

   if ( tokens[0].type != COLON )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ':'");
   GetNextToken(tokens);

   switch ( tokens[0].type )
   {
      case INT:
         datatype = INTTYPE;
         break;
      case BOOL:
         datatype = BOOLTYPE;
         break;
      default:
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting INT or BOOL");
   }
   GetNextToken(tokens);

   identifierTable.AddToTable(identifier,FUNCTION_SUBPROGRAMMODULE,datatype,identifier);
   index = identifierTable.GetIndex(identifier,isInTable);

// CODEGENERATION
   code.EnterModuleBody(FUNCTION_SUBPROGRAMMODULE,index);
   code.ResetFrameData();

// Reserve frame-space for FUNCTION return value
   code.IncrementFBOffset(1);

   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** FUNCTION module (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
   code.EmitFormattedLine(identifier,"EQU","*");
// ENDCODEGENERATION

   identifierTable.EnterNestedStaticScope();

   n = 0;
   if ( tokens[0].type != OPARENTHESIS )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '('");
// Use token look-ahead to make parsing decision
   if ( tokens[1].type != CPARENTHESIS )
   {
      do
      {
         IDENTIFIERTYPE identifierType;

         GetNextToken(tokens);
         ParseFormalParameter(tokens,identifierType,n);

// STATICSEMANTICS
         if ( identifierType != IN_PARAMETER )
            ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"FUNCTION parameter must be IN");
// ENDSTATICSEMANTICS

      } while ( tokens[0].type == SEMICOLON );
   }
   else
      GetNextToken(tokens);
   if ( tokens[0].type != CPARENTHESIS )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
   GetNextToken(tokens);


   if(tokens[0].type != OPENCURLY)
	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
	                           "Expecting '{'");
   GetNextToken(tokens);

#ifdef TRACECOMPILER
   identifierTable.DisplayTableContents("Contents of identifier table after compilation of FUNCTION module header");
#endif

// CODEGENERATION
   code.IncrementFBOffset(2); // makes room in frame for caller's saved FB register and the CALL return address
// ENDCODEGENERATION

   ParseDataDefinitions(tokens,SUBPROGRAMMODULESCOPE);

#ifdef TRACECOMPILER
   identifierTable.DisplayTableContents("Contents of identifier table after compilation of FUNCTION local data definitions");
#endif

// CODEGENERATION
   m = code.GetFBOffset()-(n+3);
   code.EmitFormattedLine("","PUSHSP","","set FUNCTION module FB = SP-on-entry + 2(n+3)");
   sprintf(operand,"#0D%d",2*(n+3));
   sprintf(comment,"n = %d",n);
   code.EmitFormattedLine("","PUSH",operand,comment);
   code.EmitFormattedLine("","ADDI");
   code.EmitFormattedLine("","POPFB");
   code.EmitFormattedLine("","PUSHSP","","FUNCTION module SP = SP-on-entry - 2m");
   sprintf(operand,"#0D%d",2*m);
   sprintf(comment,"m = %d",m);
   code.EmitFormattedLine("","PUSH",operand,comment);
   code.EmitFormattedLine("","SUBI");
   code.EmitFormattedLine("","POPSP");
   code.EmitUnformattedLine("; statements to initialize frame data (if necessary)");
   code.EmitFrameData();
   sprintf(label,"MODULEBODY%04d",code.LabelSuffix());
   code.EmitFormattedLine("","CALL",label);
   code.EmitFormattedLine("","PUSHFB","","restore caller's SP-on-entry = FB - 2(n+3)");
   sprintf(operand,"#0D%d",2*(n+3));
   code.EmitFormattedLine("","PUSH",operand);
   code.EmitFormattedLine("","SUBI");
   code.EmitFormattedLine("","POPSP");
   code.EmitFormattedLine("","RETURN","","return to caller");
   code.EmitFormattedLine(label,"EQU","*");
   code.EmitUnformattedLine("; statements in body of FUNCTION module (*MUST* execute RETURN)");
// ENDCODEGENERATION

    while ( tokens[0].type != CLOSEDCURLY )
      ParseStatement(tokens);

// CODEGENERATION
   sprintf(operand,"#0D%d",tokens[0].sourceLineNumber);
   code.EmitFormattedLine("","PUSH",operand);
   code.EmitFormattedLine("","PUSH","#0D3");
   code.EmitFormattedLine("","JMP","HANDLERUNTIMEERROR");
   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** END (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
   code.ExitModuleBody();
// ENDCODEGENERATION

   identifierTable.ExitNestedStaticScope();

#ifdef TRACECOMPILER
   identifierTable.DisplayTableContents("Contents of identifier table at end of compilation of FUNCTION module definition");
#endif

   GetNextToken(tokens);

   ExitModule("FUNCTIONDefinition");
}


//-----------------------------------------------------------
void ParseFormalParameter(TOKEN tokens[],IDENTIFIERTYPE &identifierType,int &n)
//-----------------------------------------------------------
{
   void GetNextToken(TOKEN tokens[]);

   char identifier[MAXIMUMLENGTHIDENTIFIER+1],reference[MAXIMUMLENGTHIDENTIFIER+1];
   bool isInTable;
   int index;
   DATATYPE datatype;

   EnterModule("FormalParameter");

// CODEGENERATION
   switch ( tokens[0].type )
   {
      case IN:
         identifierType = IN_PARAMETER;
         sprintf(reference,"FB:0D%d",code.GetFBOffset());
         code.IncrementFBOffset(1);
         n += 1;
         GetNextToken(tokens);
         break;
      case OUT:
         identifierType = OUT_PARAMETER;
         code.IncrementFBOffset(1);
         sprintf(reference,"FB:0D%d",code.GetFBOffset());
         code.IncrementFBOffset(1);
         n += 2;
         GetNextToken(tokens);
         break;
      case IO:
         identifierType = IO_PARAMETER;
         code.IncrementFBOffset(1);
         sprintf(reference,"FB:0D%d",code.GetFBOffset());
         code.IncrementFBOffset(1);
         n += 2;
         GetNextToken(tokens);
         break;
      case REF:
         identifierType = REF_PARAMETER;
         sprintf(reference,"@FB:0D%d",code.GetFBOffset());
         code.IncrementFBOffset(1);
         n += 1;
         GetNextToken(tokens);
         break;
      default:
         identifierType = IN_PARAMETER;
         sprintf(reference,"FB:0D%d",code.GetFBOffset());
         code.IncrementFBOffset(1);
         n += 1;
         break;
   }
// ENDCODEGENERATION

   if ( tokens[0].type != IDENTIFIER )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting identifier");
   strcpy(identifier,tokens[0].lexeme);
   GetNextToken(tokens);

   if ( tokens[0].type != COLON )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ':'");
   GetNextToken(tokens);

   switch ( tokens[0].type )
   {
      case INT:
         datatype = INTTYPE;
         break;
      case BOOL:
         datatype = BOOLTYPE;
         break;
      default:
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting INT or BOOL");
   }
   GetNextToken(tokens);

   index = identifierTable.GetIndex(identifier,isInTable);
   if ( isInTable && identifierTable.IsInCurrentScope(index) )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Multiply-defined identifier");

   identifierTable.AddToTable(identifier,identifierType,datatype,reference);

   ExitModule("FormalParameter");
}


//-----------------------------------------------------------
void ParseMAINDefinition(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseStatement(TOKEN tokens[]);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   char label[SOURCELINELENGTH+1];
   char reference[SOURCELINELENGTH+1];

   EnterModule("MAINDefinition");

// CODEGENERATION
   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** MAIN module (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
   code.EmitFormattedLine("PROGRAMMAIN","EQU"  ,"*");

   code.EmitFormattedLine("","PUSH" ,"#RUNTIMESTACK","set SP");
   code.EmitFormattedLine("","POPSP");
   code.EmitFormattedLine("","PUSHA","STATICDATA","set SB");
   code.EmitFormattedLine("","POPSB");
   code.EmitFormattedLine("","PUSH","#HEAPBASE","initialize heap");
   code.EmitFormattedLine("","PUSH","#HEAPSIZE");
   code.EmitFormattedLine("","SVC","#SVC_INITIALIZE_HEAP");
   sprintf(label,"PROGRAMBODY%04d",code.LabelSuffix());
   code.EmitFormattedLine("","CALL",label);
   code.AddDSToStaticData("Normal program termination","",reference);
   code.EmitFormattedLine("","PUSHA",reference);
   code.EmitFormattedLine("","SVC","#SVC_WRITE_STRING");
   code.EmitFormattedLine("","SVC","#SVC_WRITE_ENDL");
   code.EmitFormattedLine("","PUSH","#0D0","terminate with status = 0");
   code.EmitFormattedLine("","SVC" ,"#SVC_TERMINATE");
   code.EmitFormattedLine(label,"EQU","*");
// ENDCODEGENERATION

   GetNextToken(tokens);

   identifierTable.EnterNestedStaticScope();
   ParseDataDefinitions(tokens,PROGRAMMODULESCOPE);

   while ( tokens[0].type != CLOSEDCURLY )
        ParseStatement(tokens);

// CODEGENERATION
   code.EmitFormattedLine("","RETURN");
   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** END (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
// ENDCODEGENERATION

#ifdef TRACECOMPILER
   identifierTable.DisplayTableContents("Contents of identifier table at end of compilation of PROGRAM module definition");
#endif

   identifierTable.ExitNestedStaticScope();

   GetNextToken(tokens);

   ExitModule("MAINDefinition");
}

//-----------------------------------------------------------
void ParseStatement(TOKEN tokens[])
//-----------------------------------------------------------
{
   void GetNextToken(TOKEN tokens[]);
   void ParseDISPLAYStatement(TOKEN tokens[]);
   void ParseGetStatement(TOKEN tokens[]);
   void ParseAssignmentStatement(TOKEN tokens[]);
   void ParseIFStatement(TOKEN tokens[]);
   void ParseDOWHILEStatement(TOKEN tokens[]);
   void ParseCALLStatement(TOKEN tokens[]);
   void ParseRETURNStatement(TOKEN tokens[]);
   void ParseAssertion(TOKEN tokens[]);
   void ParseFORStatement(TOKEN tokens[]);

   EnterModule("Statement");

   while ( tokens[0].type == ASSERT )
        ParseAssertion(tokens);

   switch ( tokens[0].type )
   {
   case DO:
		 ParseDOWHILEStatement(tokens);
		 break;
   case FOR:
            ParseFORStatement(tokens);
            break;
   case DISPLAYOPEN:
	    ParseIFStatement(tokens);
	    break;
      case DISPLAY:
         ParseDISPLAYStatement(tokens);
         break;
      case CONSOLEGET:
               ParseGetStatement(tokens);
               break;
	  case IDENTIFIER:
	   ParseAssignmentStatement(tokens);
	   break;
	  case CALL:
	          ParseCALLStatement(tokens);
	          break;
	   case RETURN:
		  ParseRETURNStatement(tokens);
		  break;
      default:
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                              "Expecting beginning-of-statement");
         break;
   }
   while ( tokens[0].type == ASSERT )
      ParseAssertion(tokens);
   ExitModule("Statement");
}



//-----------------------------------------------------------
void ParseCALLStatement(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseVariable(TOKEN tokens[],bool asLValue,DATATYPE &datatype);
   void ParseExpression(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   bool isInTable;
   int index,parameters;

   EnterModule("CALLStatement");

   sprintf(line,"; **** CALL statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

   if ( tokens[0].type != IDENTIFIER )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting identifier");

// STATICSEMANTICS
   index = identifierTable.GetIndex(tokens[0].lexeme,isInTable);
   if ( !isInTable )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Undefined identifier");
   if ( identifierTable.GetType(index) != PROCEDURE_SUBPROGRAMMODULE )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting PROCEDURE identifier");
// ENDSTATICSEMANTICS

   GetNextToken(tokens);
   parameters = 0;
   if ( tokens[0].type == OPARENTHESIS )
   {
      DATATYPE expressionDatatype,variableDatatype;

      do
      {
         GetNextToken(tokens);
         parameters++;

// CODEGENERATION
// STATICSEMANTICS
         switch ( identifierTable.GetType(index+parameters) )
         {
            case IN_PARAMETER:
               ParseExpression(tokens,expressionDatatype);
               if ( expressionDatatype != identifierTable.GetDatatype(index+parameters) )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                     "Actual parameter data type does not match formal parameter data type");
               break;
            case OUT_PARAMETER:
               ParseVariable(tokens,true,variableDatatype);
               if ( variableDatatype != identifierTable.GetDatatype(index+parameters) )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                     "Actual parameter data type does not match formal parameter data type");
               code.EmitFormattedLine("","PUSH","#0X0000");
               break;
            case IO_PARAMETER:
               ParseVariable(tokens,true,variableDatatype);
               if ( variableDatatype != identifierTable.GetDatatype(index+parameters) )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                     "Actual parameter data type does not match formal parameter data type");
               code.EmitFormattedLine("","PUSH","@SP:0D0");
               break;
            case REF_PARAMETER:
               ParseVariable(tokens,true,variableDatatype);
               if ( variableDatatype != identifierTable.GetDatatype(index+parameters) )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                     "Actual parameter data type does not match formal parameter data type");
               break;
         }
// ENDSTATICSEMANTICS
// ENDCODEGENERATION
      } while ( tokens[0].type == DASH );

      if ( tokens[0].type != CPARENTHESIS )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting )");

      GetNextToken(tokens);
   }

// STATICSEMANTICS
   if ( identifierTable.GetCountOfFormalParameters(index) != parameters )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
         "Number of actual parameters does not match number of formal parameters");
// ENDSTATICSEMANTICS

// CODEGENERATION
   code.EmitFormattedLine("","PUSHFB");
   code.EmitFormattedLine("","CALL",identifierTable.GetReference(index));
   code.EmitFormattedLine("","POPFB");
   for (parameters = identifierTable.GetCountOfFormalParameters(index); parameters >= 1; parameters--)
   {
      switch ( identifierTable.GetType(index+parameters) )
      {
         case IN_PARAMETER:
            code.EmitFormattedLine("","DISCARD","#0D1");
            break;
         case OUT_PARAMETER:
            code.EmitFormattedLine("","POP","@SP:0D1");
            code.EmitFormattedLine("","DISCARD","#0D1");
            break;
         case IO_PARAMETER:
            code.EmitFormattedLine("","POP","@SP:0D1");
            code.EmitFormattedLine("","DISCARD","#0D1");
            break;
         case REF_PARAMETER:
            code.EmitFormattedLine("","DISCARD","#0D1");
            break;
      }
   }
// ENDCODEGENERATION

   if ( tokens[0].type != SEMICOLON )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ';'");

   GetNextToken(tokens);

   ExitModule("CALLStatement");
}

//-----------------------------------------------------------
void ParseRETURNStatement(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseExpression(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];

   EnterModule("RETURNStatement");

   sprintf(line,"; **** RETURN statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

// STATICSEMANTICS
   if      ( code.IsInModuleBody(PROCEDURE_SUBPROGRAMMODULE) )
// CODEGENERATION
      code.EmitFormattedLine("","RETURN");
// ENDCODEGENERATION
   else if ( code.IsInModuleBody( FUNCTION_SUBPROGRAMMODULE) )
   {
      DATATYPE datatype;

      if ( tokens[0].type != OPARENTHESIS )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '('");
      GetNextToken(tokens);

      ParseExpression(tokens,datatype);

      if ( datatype != identifierTable.GetDatatype(code.GetModuleIdentifierIndex()) )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
            "RETURN expression data type must match FUNCTION data type");

// CODEGENERATION
      code.EmitFormattedLine("","POP","FB:0D0","pop RETURN expression into function return value");
      code.EmitFormattedLine("","RETURN");
// ENDCODEGENERATION

      if ( tokens[0].type != CPARENTHESIS )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
      GetNextToken(tokens);
   }
   else
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
         "RETURN only allowed in PROCEDURE or FUNCTION module body");
// ENDSTATICSEMANTICS

   if ( tokens[0].type != SEMICOLON )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting .");

   GetNextToken(tokens);

   ExitModule("RETURNStatement");
}


//-----------------------------------------------------------
void ParseAssertion(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseExpression(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   DATATYPE datatype;

   EnterModule("Assertion");

   sprintf(line,"; **** %4d: { assertion }",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

   ParseExpression(tokens,datatype);

// STATICSEMANTICS
   if ( datatype != BOOLTYPE )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean expression");
// ENDSTATICSEMANTICS

// CODEGENERATION
/*
      SETT
      JMPT      E????
      PUSH      #0D(sourceLineNumber)
      PUSH      #0D1
      JMP       HANDLERUNTIMEERROR
E???? EQU       *
      DISCARD   #0D1
*/
   char Elabel[SOURCELINELENGTH+1],operand[SOURCELINELENGTH+1];

   code.EmitFormattedLine("","SETT");
   sprintf(Elabel,"E%04d",code.LabelSuffix());
   code.EmitFormattedLine("","JMPT",Elabel);
   sprintf(operand,"#0D%d",tokens[0].sourceLineNumber);
   code.EmitFormattedLine("","PUSH",operand);
   code.EmitFormattedLine("","PUSH","#0D1");
   code.EmitFormattedLine("","JMP","HANDLERUNTIMEERROR");
   code.EmitFormattedLine(Elabel,"EQU","*");
   code.EmitFormattedLine("","DISCARD","#0D1");
// ENDCODEGENERATION

   if ( tokens[0].type != SEMICOLON )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting }");

   GetNextToken(tokens);

   ExitModule("Assertion");
}


//-----------------------------------------------------------
void ParseIFStatement(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseExpression(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   char Ilabel[SOURCELINELENGTH+1],Elabel[SOURCELINELENGTH+1];
   DATATYPE datatype;

   EnterModule("IFStatement");

   sprintf(line,"; **** IF statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);


   if ( tokens[0].type != DISPLAYOPEN )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '('");
   GetNextToken(tokens);
   ParseExpression(tokens,datatype);
   if ( tokens[0].type != DISPLAYCLOSE )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
   GetNextToken(tokens);

   if(tokens[0].type != QUEMARK)
	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '?'");
   GetNextToken(tokens);

   if ( datatype != BOOLTYPE )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean expression");

// CODEGENERATION
/*
   Plan for the generalized IF statement with n ELIFs and 1 ELSE (*Note* n
      can be 0 and the ELSE may be missing and the plan still "works.")

   ...expression...           ; boolean expression on top-of-stack
      SETT
      DISCARD   #0D1
      JMPNT     I???1
   ...statements...
      JMP       E????
I???1 EQU       *             ; 1st ELIF clause
   ...expression...
      SETT
      DISCARD   #0D1
      JMPNT     I???2
   ...statements...
      JMP       E????
      .
      .
I???n EQU       *             ; nth ELIF clause
   ...expression...
      SETT
      DISCARD   #0D1
      JMPNT     I????
   ...statements...
      JMP       E????
I???? EQU       *             ; ELSE clause
   ...statements...
E???? EQU       *
*/
   sprintf(Elabel,"E%04d",code.LabelSuffix());
   code.EmitFormattedLine("","SETT");
   code.EmitFormattedLine("","DISCARD","#0D1");
   sprintf(Ilabel,"I%04d",code.LabelSuffix());
   code.EmitFormattedLine("","JMPNT",Ilabel);
// ENDCODEGENERATION

   while ( (tokens[0].type != QUEMARKELSE) &&
           (tokens[0].type != COLON) &&
           (tokens[0].type !=  SEMICOLON))
      ParseStatement(tokens);

// CODEGENERATION
   code.EmitFormattedLine("","JMP",Elabel);
   code.EmitFormattedLine(Ilabel,"EQU","*");
// ENDCODEGENERATION

   while ( tokens[0].type == QUEMARKELSE )
   {
      GetNextToken(tokens);
      if ( tokens[0].type != DISPLAYOPEN )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '('");
      GetNextToken(tokens);
      ParseExpression(tokens,datatype);
      if ( tokens[0].type != DISPLAYCLOSE )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
      GetNextToken(tokens);

      if ( datatype != BOOLTYPE )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean expression");

// CODEGENERATION
      code.EmitFormattedLine("","SETT");
      code.EmitFormattedLine("","DISCARD","#0D1");
      sprintf(Ilabel,"I%04d",code.LabelSuffix());
      code.EmitFormattedLine("","JMPNT",Ilabel);
// ENDCODEGENERATION

      while ( (tokens[0].type != QUEMARKELSE) &&
              (tokens[0].type != COLON) &&
              (tokens[0].type !=  SEMICOLON) )
         ParseStatement(tokens);

// CODEGENERATION
      code.EmitFormattedLine("","JMP",Elabel);
      code.EmitFormattedLine(Ilabel,"EQU","*");
// ENDCODEGENERATION

   }
   if ( tokens[0].type == COLON)
   {
      GetNextToken(tokens);
      while ( tokens[0].type != SEMICOLON )
         ParseStatement(tokens);
   }

   GetNextToken(tokens);

// CODEGENERATION
      code.EmitFormattedLine(Elabel,"EQU","*");
// ENDCODEGENERATION

   ExitModule("IFStatement");
}

//-----------------------------------------------------------
void ParseDOWHILEStatement(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseExpression(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   char Dlabel[SOURCELINELENGTH+1],Elabel[SOURCELINELENGTH+1];
   DATATYPE datatype;

   EnterModule("DOWHILEStatement");

   sprintf(line,"; **** DO-WHILE statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

// CODEGENERATION
/*
D???? EQU       *
   ...statements...
   ...expression...
      SETT
      DISCARD   #0D1
      JMPNT     E????
   ...statements...
      JMP       D????
E???? EQU       *
*/

   sprintf(Dlabel,"D%04d",code.LabelSuffix());
   sprintf(Elabel,"E%04d",code.LabelSuffix());
   code.EmitFormattedLine(Dlabel,"EQU","*");
// ENDCODEGENERATION

   if(tokens[0].type != OPENCURLY)
	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '{'");
   GetNextToken(tokens);
   while (tokens[0].type != CLOSEDCURLY )
      ParseStatement(tokens);
   GetNextToken(tokens);
   if ( tokens[0].type != WHILE )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting 'WHILE'");
   GetNextToken(tokens);
   if ( tokens[0].type != DISPLAYOPEN )
	  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '<'");
   GetNextToken(tokens);
   ParseExpression(tokens,datatype);
   if ( tokens[0].type != DISPLAYCLOSE )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '>'");
   GetNextToken(tokens);

   if ( datatype != BOOLTYPE )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean expression");

// CODEGENERATION
   code.EmitFormattedLine("","SETT");
   code.EmitFormattedLine("","DISCARD","#0D1");
   code.EmitFormattedLine("","JMPNT",Elabel);
// ENDCODEGENERATION

   if(tokens[0].type != SEMICOLON)
	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ';'");

   GetNextToken(tokens);

// CODEGENERATION
   code.EmitFormattedLine("","JMP",Dlabel);
   code.EmitFormattedLine(Elabel,"EQU","*");
// ENDCODEGENERATION

   ExitModule("DOWHILEStatement");
}

//-----------------------------------------------------------
void ParseGetStatement(TOKEN tokens[])
//-----------------------------------------------------------
{
    void ParseVariable(TOKEN tokens[], bool asLValue, DATATYPE &datatype);
    void GetNextToken(TOKEN tokens[]);

    char reference[SOURCELINELENGTH+1];
    char line[SOURCELINELENGTH+1];
    DATATYPE datatype;

    EnterModule("INPUTStatement");

    // Emit comment for codegen
    sprintf(line, "; **** INPUT statement (%4d)", tokens[0].sourceLineNumber);
    code.EmitUnformattedLine(line);

    GetNextToken(tokens); // consume CONSOLEGET

    // Expect opening '<'
    if (tokens[0].type != DISPLAYOPEN)
        ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Expecting '<'");
    GetNextToken(tokens); // consume '<'

    // Loop through <InputItem> { ; <InputItem> }*
    while (true)
    {
        // Parse <InputItem> ::= <variable> [ <string> ]
        ParseVariable(tokens, true, datatype);

        // Optional string prompt
        if (tokens[0].type == STRING)
        {
            code.AddDSToStaticData(tokens[0].lexeme, "", reference);
            code.EmitFormattedLine("", "PUSHA", reference);
            code.EmitFormattedLine("", "SVC", "#SVC_WRITE_STRING");
            GetNextToken(tokens);
        }

        // Emit input read based on variable type
        switch (datatype)
        {
            case INTTYPE:
                code.EmitFormattedLine("", "SVC", "#SVC_READ_INTEGER");
                break;
            case BOOLTYPE:
                code.EmitFormattedLine("", "SVC", "#SVC_READ_BOOLEAN");
                break;
            default:
                ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Invalid input type");
        }

        code.EmitFormattedLine("", "POP", "@SP:0D1");
        code.EmitFormattedLine("", "DISCARD", "#0D1");

        // Check next token: either ';' (more input items) or '>' (end of input list)
        if (tokens[0].type == SEMICOLON)
        {
            GetNextToken(tokens); // consume ';'
            continue;
        }
        else if (tokens[0].type == DISPLAYCLOSE)
        {
            break; // done with input list
        }
        else
        {
            ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex,
                                 "Expecting ';' or '>'");
        }
    }

    GetNextToken(tokens); // consume '>'

    // Expect terminating ';'
    if (tokens[0].type != SEMICOLON)
        ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Expecting ';'");
    GetNextToken(tokens); // consume ';'

    ExitModule("INPUTStatement");
}
//-----------------------------------------------------------
void ParseDISPLAYStatement(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseExpression(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   DATATYPE datatype;

   EnterModule("DISPLAYStatement");

   // CODEGENERATION
   sprintf(line, "; **** DISPLAY statement (%4d)", tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   // ENDCODEGENERATION

   GetNextToken(tokens); // consume DISPLAY
   if (tokens[0].type != DISPLAYOPEN)
      ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Expecting '<'");
   GetNextToken(tokens); // consume '<'

   // Loop through <item - item - item>
   while (tokens[0].type != DISPLAYCLOSE)
   {
      switch (tokens[0].type)
      {
         case STRING:
         {
            char reference[SOURCELINELENGTH+1];
            code.AddDSToStaticData(tokens[0].lexeme, "", reference);
            code.EmitFormattedLine("", "PUSHA", reference);
            code.EmitFormattedLine("", "SVC", "#SVC_WRITE_STRING");
            GetNextToken(tokens);
            break;
         }

         case NEW_LINE:
            code.EmitFormattedLine("", "SVC", "#SVC_WRITE_ENDL");
            GetNextToken(tokens);
            break;

         default:
            // Expression (like (a + b))
            ParseExpression(tokens, datatype);
            switch (datatype)
            {
               case INTTYPE:
                  code.EmitFormattedLine("", "SVC", "#SVC_WRITE_INTEGER");
                  break;
               case BOOLTYPE:
                  code.EmitFormattedLine("", "SVC", "#SVC_WRITE_BOOLEAN");
                  break;
               default:
                  ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex,
                                       "Unsupported datatype in DISPLAY");
                  break;
            }
            break;
      }

      // If there’s a dash, consume it and continue
      if (tokens[0].type == DASH)
         GetNextToken(tokens);
      else
         break;
   }

   if (tokens[0].type != DISPLAYCLOSE)
      ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Expecting '>'");
   GetNextToken(tokens); // consume '>'

   if (tokens[0].type != SEMICOLON)
      ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Expecting ';'");
   GetNextToken(tokens);

   ExitModule("DISPLAYStatement");
}


//-----------------------------------------------------------
void ParseAssignmentStatement(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseVariable(TOKEN tokens[], bool asLValue, DATATYPE &datatype);
   void ParseExpression(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   DATATYPE datatypeLHS, datatypeRHS;
   int n;

   EnterModule("AssignmentStatement");

   sprintf(line, "; **** assignment statement (%4d)", tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   ParseVariable(tokens, true, datatypeLHS);
   n = 1;

   while (tokens[0].type == DASH)
   {
      DATATYPE datatype;
      GetNextToken(tokens);
      ParseVariable(tokens, true, datatype);
      n++;

      if (datatype != datatypeLHS)
         ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex,
                              "Mixed-mode variables not allowed");
   }

   if (tokens[0].type != EQ)
      ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Expecting '='");
   GetNextToken(tokens);

   ParseExpression(tokens, datatypeRHS);

   if (datatypeLHS != datatypeRHS)
      ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Data type mismatch");

   // CODEGENERATION
   for (int i = 1; i <= n; i++)
   {
      code.EmitFormattedLine("", "MAKEDUP");
      code.EmitFormattedLine("", "POP", "@SP:0D2");
      code.EmitFormattedLine("", "SWAP");
      code.EmitFormattedLine("", "DISCARD", "#0D1");
   }

   // Only discard the last value if we don't need it afterward
      code.EmitFormattedLine("", "DISCARD", "#0D1");
   // ENDCODEGENERATION

   if (tokens[0].type != SEMICOLON)
      ProcessCompilerError(tokens[0].sourceLineNumber, tokens[0].sourceLineIndex, "Expecting ';'");
   GetNextToken(tokens);

   ExitModule("AssignmentStatement");
}

//-----------------------------------------------------------
void ParseFORStatement(TOKEN tokens[])
//-----------------------------------------------------------
{
   void ParseVariable(TOKEN tokens[],bool asLValue,DATATYPE &datatype);
   void ParseExpression(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   char Dlabel[SOURCELINELENGTH+1],Llabel[SOURCELINELENGTH+1],
        Clabel[SOURCELINELENGTH+1],Elabel[SOURCELINELENGTH+1];
   char operand[SOURCELINELENGTH+1];
   DATATYPE datatype;

   EnterModule("FORStatement");

   sprintf(line,"; **** FOR statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

   if(tokens[0].type != OPARENTHESIS)
	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '('");
   GetNextToken(tokens);

   ParseVariable(tokens,true,datatype);
     if ( datatype != INTTYPE )
        ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer variable");


/*
; v := e1
   ...v...                    ; &v = run-time stack (bottom to top)
   ...e1...                   ; &v,e1
      POP       @SP:0D1       ; &v := e1
   ...e2...                   ; &v,e2
   ...e3...                   ; &v,e2,e3
      SETNZPI
; if ( e3 = 0 ) then
      JMPNZ     D????
      PUSH      #0D(current line number)
      PUSH      #0D2
      JMP       HANDLERUNTIMEERROR
D???? SETNZPI
; else if ( e3 > 0 ) then
      JMPN      L????
      SWAP                    ; &v,e3,e2
      MAKEDUP                 ; &v,e3,e2,e2
      PUSH      @SP:0D3       ; &v,e3,e2,e2,v
      SWAP                    ; &v,e3,e2,v,e2
;    if ( v <= e2 ) continue else end
      CMPI                    ; &v,e3,e2 (set LEG)
      JMPLE     C????
      JMP       E????
; else ( e3 < 0 )
L???? SWAP                    ; &v,e3,e2
      MAKEDUP                 ; &v,e3,e2,e2
      PUSH      @SP:0D3       ; &v,e3,e2,e2,v
      SWAP                    ; &v,e3,e2,v,e2
;    if ( v >= e2 ) continue else end
      CMPI                    ; &v,e3,e2 (set LEG)
      JMPGE     C????
      JMP       E????
; endif
C???? EQU       *
   ...statements...
      SWAP                    ; &v,e2,e3
      MAKEDUP                 ; &v,e2,e3,e3
; v := e3+v
      PUSH      @SP:0D3       ; &v,e2,e3,e3,v
      ADDI                    ; &v,e2,e3,(e3+v)
      POP       @SP:0D3       ; &v,e2,e3
      JMP       D????
E???? DISCARD   #0D3          ; now run-time stack is empty
*/

     if(tokens[0].type != SEMICOLON)
   	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ';'");
     GetNextToken(tokens);

   ParseExpression(tokens,datatype);
     if ( datatype != INTTYPE )
        ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer variable");

// CODEGENERATION
   code.EmitFormattedLine("","POP","@SP:0D1");
// ENDCODEGENERATION

   if ( tokens[0].type != ARROW )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ->");
   GetNextToken(tokens);

   ParseExpression(tokens,datatype);
   if ( datatype != INTTYPE )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer data type");
   if(tokens[0].type != SEMICOLON)
	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ';'");
   GetNextToken(tokens);


            ParseExpression(tokens,datatype);
            if ( datatype != INTTYPE )
               ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer data type");

   if(tokens[0].type != CPARENTHESIS)
   	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
      GetNextToken(tokens);
   if(tokens[0].type != OPENCURLY)
        	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
   GetNextToken(tokens);

// CODEGENERATION
   sprintf(Dlabel,"D%04d",code.LabelSuffix());
   sprintf(Llabel,"L%04d",code.LabelSuffix());
   sprintf(Clabel,"C%04d",code.LabelSuffix());
   sprintf(Elabel,"E%04d",code.LabelSuffix());

   code.EmitFormattedLine("","SETNZPI");
   code.EmitFormattedLine("","JMPNZ",Dlabel);
   sprintf(operand,"#0D%d",tokens[0].sourceLineNumber);
   code.EmitFormattedLine("","PUSH",operand);
   code.EmitFormattedLine("","PUSH","#0D2");
   code.EmitFormattedLine("","JMP","HANDLERUNTIMEERROR");

   code.EmitFormattedLine(Dlabel,"SETNZPI");
   code.EmitFormattedLine("","JMPN",Llabel);
   code.EmitFormattedLine("","SWAP");
   code.EmitFormattedLine("","MAKEDUP");
   code.EmitFormattedLine("","PUSH","@SP:0D3");
   code.EmitFormattedLine("","SWAP");
   code.EmitFormattedLine("","CMPI");
   code.EmitFormattedLine("","JMPLE",Clabel);
   code.EmitFormattedLine("","JMP",Elabel);
   code.EmitFormattedLine(Llabel,"SWAP");
   code.EmitFormattedLine("","MAKEDUP");
   code.EmitFormattedLine("","PUSH","@SP:0D3");
   code.EmitFormattedLine("","SWAP");
   code.EmitFormattedLine("","CMPI");
   code.EmitFormattedLine("","JMPGE",Clabel);
   code.EmitFormattedLine("","JMP",Elabel);
   code.EmitFormattedLine(Clabel,"EQU","*");
// ENDCODEGENERATION

   while ( tokens[0].type != CLOSEDCURLY )
      ParseStatement(tokens);

   GetNextToken(tokens);
   if(tokens[0].type != SEMICOLON)
	   ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ';'");
   GetNextToken(tokens);

// CODEGENERATION
   code.EmitFormattedLine("","SWAP");
   code.EmitFormattedLine("","MAKEDUP");
   code.EmitFormattedLine("","PUSH","@SP:0D3");
   code.EmitFormattedLine("","ADDI");
   code.EmitFormattedLine("","POP","@SP:0D3");
   code.EmitFormattedLine("","JMP",Dlabel);
   code.EmitFormattedLine(Elabel,"DISCARD","#0D3");
// ENDCODEGENERATION

   ExitModule("FORStatement");
}

//-----------------------------------------------------------
void ParseExpression(TOKEN tokens[],DATATYPE &datatype)
//-----------------------------------------------------------
{
// CODEGENERATION
/*
   An expression is composed of a collection of one or more operands (SPL calls them
      primaries) and operators (and perhaps sets of parentheses to modify the default
      order-of-evaluation established by precedence and associativity rules).
      Expression evaluation computes a single value as the expression's result.
      The result has a specific data type. By design, the expression result is
      "left" at the top of the run-time stack for subsequent use.

   SPL expressions must be single-mode with operators working on operands of
      the appropriate type (for example, boolean AND boolean) and not mixing
      modes. Static semantic analysis guarantees that operators are
      operating on operands of appropriate data type.
*/
// ENDCODEGENERATION

   void ParseConjunction(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS,datatypeRHS;

   EnterModule("Expression");

   ParseConjunction(tokens,datatypeLHS);

   if ( (tokens[0].type ==  OR) ||
        (tokens[0].type == NOR) ||
        (tokens[0].type == XOR) )
   {
      while ( (tokens[0].type ==  OR) ||
              (tokens[0].type == NOR) ||
              (tokens[0].type == XOR) )
      {
         TOKENTYPE operation = tokens[0].type;

         GetNextToken(tokens);
         ParseConjunction(tokens,datatypeRHS);

// CODEGENERATION
         switch ( operation )
         {
            case OR:

// STATICSEMANTICS
               if ( !((datatypeLHS == BOOLTYPE) && (datatypeRHS == BOOLTYPE)) )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean operands");
// ENDSTATICSEMANTICS

               code.EmitFormattedLine("","OR");
               datatype = BOOLTYPE;
               break;
            case NOR:

// STATICSEMANTICS
               if ( !((datatypeLHS == BOOLTYPE) && (datatypeRHS == BOOLTYPE)) )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean operands");
// ENDSTATICSEMANTICS

               code.EmitFormattedLine("","NOR");
               datatype = BOOLTYPE;
               break;
            case XOR:

// STATICSEMANTICS
               if ( !((datatypeLHS == BOOLTYPE) && (datatypeRHS == BOOLTYPE)) )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean operands");
// ENDSTATICSEMANTICS

               code.EmitFormattedLine("","XOR");
               datatype = BOOLTYPE;
               break;
         }
      }
// CODEGENERATION

   }
   else
      datatype = datatypeLHS;

   ExitModule("Expression");
}
//-----------------------------------------------------------
void ParseConjunction(TOKEN tokens[],DATATYPE &datatype)
//-----------------------------------------------------------
{
   void ParseNegation(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS,datatypeRHS;

   EnterModule("Conjunction");

   ParseNegation(tokens,datatypeLHS);

   if ( (tokens[0].type ==  AND) ||
        (tokens[0].type == NAND) )
   {
      while ( (tokens[0].type ==  AND) ||
              (tokens[0].type == NAND) )
      {
         TOKENTYPE operation = tokens[0].type;

         GetNextToken(tokens);
         ParseNegation(tokens,datatypeRHS);

         switch ( operation )
         {
            case AND:
               if ( !((datatypeLHS == BOOLTYPE) && (datatypeRHS == BOOLTYPE)) )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean operands");
               code.EmitFormattedLine("","AND");
               datatype = BOOLTYPE;
               break;
            case NAND:
               if ( !((datatypeLHS == BOOLTYPE) && (datatypeRHS == BOOLTYPE)) )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean operands");
               code.EmitFormattedLine("","NAND");
               datatype = BOOLTYPE;
               break;
         }
      }
   }
   else
      datatype = datatypeLHS;

   ExitModule("Conjunction");
}

//-----------------------------------------------------------
void ParseNegation(TOKEN tokens[],DATATYPE &datatype)
//-----------------------------------------------------------
{
   void ParseComparison(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeRHS;

   EnterModule("Negation");

   if ( tokens[0].type == NOT )
   {
      GetNextToken(tokens);
      ParseComparison(tokens,datatypeRHS);

      if ( !(datatypeRHS == BOOLTYPE) )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean operand");
      code.EmitFormattedLine("","NOT");
      datatype = BOOLTYPE;
   }
   else
      ParseComparison(tokens,datatype);

   ExitModule("Negation");
}

//-----------------------------------------------------------
void ParseComparison(TOKEN tokens[],DATATYPE &datatype)
//-----------------------------------------------------------
{
   void ParseComparator(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS,datatypeRHS;

   EnterModule("Comparison");

   ParseComparator(tokens,datatypeLHS);
   if ( (tokens[0].type ==    LT) ||
        (tokens[0].type ==  LTEQ) ||
        (tokens[0].type ==    EQ) ||
        (tokens[0].type ==    GT) ||
        (tokens[0].type ==  GTEQ) ||
        (tokens[0].type == NOTEQ)
      )
   {
      TOKENTYPE operation = tokens[0].type;

      GetNextToken(tokens);
      ParseComparator(tokens,datatypeRHS);

      if ( (datatypeLHS != INTTYPE) || (datatypeRHS != INTTYPE) )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer operands");
/*
      CMPI
      JMPXX     T????         ; XX = L,E,G,LE,NE,GE (as required)
      PUSH      #0X0000       ; push FALSE
      JMP       E????         ;    or
T???? PUSH      #0XFFFF       ; push TRUE (as required)
E???? EQU       *
*/
      char Tlabel[SOURCELINELENGTH+1],Elabel[SOURCELINELENGTH+1];

      code.EmitFormattedLine("","CMPI");
      sprintf(Tlabel,"T%04d",code.LabelSuffix());
      sprintf(Elabel,"E%04d",code.LabelSuffix());
      switch ( operation )
      {
         case LT:
            code.EmitFormattedLine("","JMPL",Tlabel);
            break;
         case LTEQ:
            code.EmitFormattedLine("","JMPLE",Tlabel);
            break;
         case EQ:
            code.EmitFormattedLine("","JMPE",Tlabel);
            break;
         case GT:
            code.EmitFormattedLine("","JMPG",Tlabel);
            break;
         case GTEQ:
            code.EmitFormattedLine("","JMPGE",Tlabel);
            break;
         case NOTEQ:
            code.EmitFormattedLine("","JMPNE",Tlabel);
            break;
      }
      datatype = BOOLTYPE;
      code.EmitFormattedLine("","PUSH","#0X0000");
      code.EmitFormattedLine("","JMP",Elabel);
      code.EmitFormattedLine(Tlabel,"PUSH","#0XFFFF");
      code.EmitFormattedLine(Elabel,"EQU","*");
   }
   else
      datatype = datatypeLHS;

   ExitModule("Comparison");
}

//-----------------------------------------------------------
void ParseComparator(TOKEN tokens[],DATATYPE &datatype)
//-----------------------------------------------------------
{
   void ParseTerm(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS,datatypeRHS;

   EnterModule("Comparator");

   ParseTerm(tokens,datatypeLHS);

   if ( (tokens[0].type ==  PLUS) ||
        (tokens[0].type == MINUS) )
   {
      while ( (tokens[0].type ==  PLUS) ||
              (tokens[0].type == MINUS) )
      {
         TOKENTYPE operation = tokens[0].type;

         GetNextToken(tokens);
         ParseTerm(tokens,datatypeRHS);

         if ( (datatypeLHS != INTTYPE) || (datatypeRHS != INTTYPE) )
            ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer operands");

         switch ( operation )
         {
            case PLUS:
               code.EmitFormattedLine("","ADDI");
               break;
            case MINUS:
               code.EmitFormattedLine("","SUBI");
               break;
         }
         datatype = INTTYPE;
      }
   }
   else
      datatype = datatypeLHS;

   ExitModule("Comparator");
}

//-----------------------------------------------------------
void ParseTerm(TOKEN tokens[],DATATYPE &datatype)
//-----------------------------------------------------------
{
   void ParseFactor(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS,datatypeRHS;

   EnterModule("Term");

   ParseFactor(tokens,datatypeLHS);
   if ( (tokens[0].type == MULTIPLY) ||
        (tokens[0].type ==   DIVIDE) ||
        (tokens[0].type ==  MODULUS) )
   {
      while ( (tokens[0].type == MULTIPLY) ||
              (tokens[0].type ==   DIVIDE) ||
              (tokens[0].type ==  MODULUS) )
      {
         TOKENTYPE operation = tokens[0].type;

         GetNextToken(tokens);
         ParseFactor(tokens,datatypeRHS);

         if ( (datatypeLHS != INTTYPE) || (datatypeRHS != INTTYPE) )
            ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer operands");

         switch ( operation )
         {
            case MULTIPLY:
               code.EmitFormattedLine("","MULI");
               break;
            case DIVIDE:
               code.EmitFormattedLine("","DIVI");
               break;
            case MODULUS:
               code.EmitFormattedLine("","REMI");
               break;
         }
         datatype = INTTYPE;
      }
   }
   else
      datatype = datatypeLHS;

   ExitModule("Term");
}

//-----------------------------------------------------------
void ParseFactor(TOKEN tokens[],DATATYPE &datatype)
//-----------------------------------------------------------
{
   void ParseSecondary(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   EnterModule("Factor");

   if ( (tokens[0].type ==   ABS) ||
        (tokens[0].type ==  PLUS) ||
        (tokens[0].type == MINUS)
      )
   {
      DATATYPE datatypeRHS;
      TOKENTYPE operation = tokens[0].type;

      GetNextToken(tokens);
      ParseSecondary(tokens,datatypeRHS);

      if ( datatypeRHS != INTTYPE )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer operand");

      switch ( operation )
      {
         case ABS:
/*
      SETNZPI
      JMPNN     E????
      NEGI                    ; NEGI or NEGF (as required)
E???? EQU       *
*/
            {
               char Elabel[SOURCELINELENGTH+1];

               sprintf(Elabel,"E%04d",code.LabelSuffix());
               code.EmitFormattedLine("","SETNZPI");
               code.EmitFormattedLine("","JMPNN",Elabel);
               code.EmitFormattedLine("","NEGI");
               code.EmitFormattedLine(Elabel,"EQU","*");
            }
            break;
         case PLUS:
         // Do nothing (identity operator)
            break;
         case MINUS:
            code.EmitFormattedLine("","NEGI");
            break;
      }
      datatype = INTTYPE;
   }
   else
      ParseSecondary(tokens,datatype);

   ExitModule("Factor");
}

//-----------------------------------------------------------
void ParseSecondary(TOKEN tokens[],DATATYPE &datatype)
//-----------------------------------------------------------
{
   void ParsePrimary(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS,datatypeRHS;

   EnterModule("Secondary");

   ParsePrimary(tokens,datatypeLHS);

   if ( tokens[0].type == POWER )
   {
      GetNextToken(tokens);

      ParsePrimary(tokens,datatypeRHS);

      if ( (datatypeLHS != INTTYPE) || (datatypeRHS != INTTYPE) )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer operands");

      code.EmitFormattedLine("","POWI");
      datatype = INTTYPE;
   }
   else
      datatype = datatypeLHS;

   ExitModule("Secondary");
}

//-----------------------------------------------------------
void ParsePrimary(TOKEN tokens[],DATATYPE &datatype)
//-----------------------------------------------------------
{
   void ParseVariable(TOKEN tokens[],bool asLValue,DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   EnterModule("Primary");

   switch ( tokens[0].type )
   {
      case INTEGER:
         {
            char operand[SOURCELINELENGTH+1];

            sprintf(operand,"#0D%s",tokens[0].lexeme);
            code.EmitFormattedLine("","PUSH",operand);
            datatype = INTTYPE;
            GetNextToken(tokens);
         }
         break;
   //************* Thanks to Cayden Garcia (FA2023)
   // ***BEWARE*** when you choose a different lexeme for either boolean value!
   //*************
      case TRUE:
         code.EmitFormattedLine("","PUSH","#0XFFFF"); // or "#true"
         datatype = BOOLTYPE;
         GetNextToken(tokens);
         break;
      case FALSE:
         code.EmitFormattedLine("","PUSH","#0X0000"); // or "false"
         datatype = BOOLTYPE;
         GetNextToken(tokens);
         break;
      case OPARENTHESIS:
         GetNextToken(tokens);
         ParseExpression(tokens,datatype);
         if ( tokens[0].type != CPARENTHESIS )
            ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
         GetNextToken(tokens);
         break;
      case IDENTIFIER:
         {
            bool isInTable;
            int index;

            index = identifierTable.GetIndex(tokens[0].lexeme,isInTable);
            if ( !isInTable )
               ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Undefined identifier");
//==========================
// variable reference
//==========================
            if ( identifierTable.GetType(index) != FUNCTION_SUBPROGRAMMODULE )
               ParseVariable(tokens,false,datatype);
//==========================
// FUNCTION_SUBPROGRAMMODULE reference
//==========================
            else
            {
               char operand[MAXIMUMLENGTHIDENTIFIER+1];
               int parameters;

               GetNextToken(tokens);
               if ( tokens[0].type != OPARENTHESIS )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '('");

// CODEGENERATION
               code.EmitFormattedLine("","PUSH","#0X0000","reserve space for function return value");
// ENDCODEGENERATION

               datatype = identifierTable.GetDatatype(index);
               parameters = 0;
               if ( tokens[1].type == CPARENTHESIS )
               {
                  GetNextToken(tokens);
               }
               else
               {
                  do
                  {
                     DATATYPE expressionDatatype;

                     GetNextToken(tokens);
                     ParseExpression(tokens,expressionDatatype);
                     parameters++;

// STATICSEMANTICS
                     if ( expressionDatatype != identifierTable.GetDatatype(index+parameters) )
                        ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Actual parameter data type does not match formal parameter data type");
// ENDSTATICSEMANTICS

                  } while ( tokens[0].type == SEMICOLON );
               }

// STATICSEMANTICS
               if ( identifierTable.GetCountOfFormalParameters(index) != parameters )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                     "Number of actual parameters does not match number of formal parameters");
// ENDSTATICSEMANTICS

               if ( tokens[0].type != CPARENTHESIS )
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
               GetNextToken(tokens);

// CODEGENERATION
               code.EmitFormattedLine("","PUSHFB");
               code.EmitFormattedLine("","CALL",identifierTable.GetReference(index));
               code.EmitFormattedLine("","POPFB");
               sprintf(operand,"#0D%d",parameters);
               code.EmitFormattedLine("","DISCARD",operand);
// ENDCODEGENERATION
            }
         }
         break;
      default:
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                              "Expecting integer, true, false, '(', variable, FUNCTION identifier");
         break;
   }

   ExitModule("Primary");
}

//-----------------------------------------------------------
void ParseVariable(TOKEN tokens[],bool asLValue,DATATYPE &datatype)
//-----------------------------------------------------------
{
/*
Syntax "locations"                 l- or r-value
---------------------------------  -------------
<expression>                       r-value
<prefix>                           l-value
<INPUTStatement>                   l-value
LHS of <assignmentStatement>       l-value
<FORStatement>                     l-value
OUT <formalParameter>              l-value
IO <formalParameter>               l-value
REF <formalParameter>              l-value

r-value ( read-only): value is pushed on run-time stack
l-value (read/write): address of value is pushed on run-time stack
*/
   void GetNextToken(TOKEN tokens[]);

   bool isInTable;
   int index;
   IDENTIFIERTYPE identifierType;

   EnterModule("Variable");

   if ( tokens[0].type != IDENTIFIER )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting identifier");

// STATICSEMANTICS
   index = identifierTable.GetIndex(tokens[0].lexeme,isInTable);
   if ( !isInTable )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Undefined identifier");

   identifierType = identifierTable.GetType(index);
   datatype = identifierTable.GetDatatype(index);

   if ( !((identifierType ==           GLOBAL_VARIABLE) ||
          (identifierType ==           GLOBAL_CONSTANT) ||
          (identifierType ==    PROGRAMMODULE_VARIABLE) ||
          (identifierType ==    PROGRAMMODULE_CONSTANT) ||
          (identifierType == SUBPROGRAMMODULE_VARIABLE) ||
          (identifierType == SUBPROGRAMMODULE_CONSTANT) ||
          (identifierType ==              IN_PARAMETER) ||
          (identifierType ==             OUT_PARAMETER) ||
          (identifierType ==              IO_PARAMETER) ||
          (identifierType ==             REF_PARAMETER)) )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting variable or constant identifier");

   if ( asLValue && ((identifierType ==           GLOBAL_CONSTANT) ||
                     (identifierType ==    PROGRAMMODULE_CONSTANT) ||
                     (identifierType == SUBPROGRAMMODULE_CONSTANT)) )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Constant may not be l-value");

   if ( asLValue && (identifierType == GLOBAL_VARIABLE) && code.IsInModuleBody(FUNCTION_SUBPROGRAMMODULE) )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"FUNCTION may not modify global variable");
// ENDSTATICSEMANTICS

// CODEGENERATION
   if ( asLValue )
      code.EmitFormattedLine("","PUSHA",identifierTable.GetReference(index));
   else
      code.EmitFormattedLine("","PUSH",identifierTable.GetReference(index));
// ENDCODEGENERATION

   GetNextToken(tokens);

   ExitModule("Variable");
}

//-----------------------------------------------------------
void Callback1(int sourceLineNumber,const char sourceLine[])
//-----------------------------------------------------------
{
   cout << setw(4) << sourceLineNumber << " " << sourceLine << endl;
}

//-----------------------------------------------------------
void Callback2(int sourceLineNumber,const char sourceLine[])
//-----------------------------------------------------------
{
   char line[SOURCELINELENGTH+1];

// CODEGENERATION
   sprintf(line,"; %4d %s",sourceLineNumber,sourceLine);
   code.EmitUnformattedLine(line);
// ENDCODEGENERATION
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
               }

              while (!(nextCharacter == '~' && reader.GetLookAheadCharacter(1).character == '$')
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
   else if ( isdigit(nextCharacter) )
   {
      i = 0;
      lexeme[i++] = nextCharacter;
      nextCharacter = reader.GetNextCharacter().character;
      while ( isdigit(nextCharacter) )
      {
         lexeme[i++] = nextCharacter;
         nextCharacter = reader.GetNextCharacter().character;
      }
      lexeme[i] = '\0';
      type = INTEGER;
   } else {
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
         case '(':
                     type = OPARENTHESIS;
                     lexeme[0] = nextCharacter; lexeme[1] = '\0';
                     reader.GetNextCharacter();
                     break;
                  case ')':
                     type = CPARENTHESIS;
                     lexeme[0] = nextCharacter; lexeme[1] = '\0';
                     reader.GetNextCharacter();
                     break;
		  case '=':
					 type = EQ;
					 lexeme[0] = nextCharacter; lexeme[1] = '\0';
					 reader.GetNextCharacter();
					 break;
		  case '!':
		              lexeme[0] = nextCharacter;
		              if ( reader.GetLookAheadCharacter(1).character == '=' )
		              {
		                 nextCharacter = reader.GetNextCharacter().character;
		                 lexeme[1] = nextCharacter; lexeme[2] = '\0';
		                 reader.GetNextCharacter();
		                 type = NOTEQ;
		              }
		              else
		              {
		                 type = UNKTOKEN;
		                 lexeme[1] = '\0';
		                 reader.GetNextCharacter();
		              }
		              break;
	   case '+':
		  type = PLUS;
		  lexeme[0] = nextCharacter; lexeme[1] = '\0';
		  reader.GetNextCharacter();
		  break;
	   case '-':
		  type = MINUS;
		  lexeme[0] = nextCharacter;
		  nextCharacter = reader.GetNextCharacter().character;
		  if(nextCharacter == '?'){
			  type = MINUS;
			  lexeme[1] = nextCharacter;
			  lexeme[2] = '\0';
			  reader.GetNextCharacter();
		  } else if(nextCharacter == '>'){
			  type = ARROW;
			  lexeme[1] = nextCharacter;
			  lexeme[2] = '\0';
			  reader.GetNextCharacter();
		  }
		  else {
			  type = DASH;
			  lexeme[1] = '\0';
			  break;
	     }

		  break;
	// use character look-ahead to "find" other '*'
	   case '*':
		  lexeme[0] = nextCharacter;
		  if ( reader.GetLookAheadCharacter(1).character == '*' )
		  {
			 nextCharacter = reader.GetNextCharacter().character;
			 lexeme[1] = nextCharacter; lexeme[2] = '\0';
			 type = POWER;
		  }
		  else
		  {
			 type = MULTIPLY;
			 lexeme[0] = nextCharacter; lexeme[1] = '\0';
		  }
		  reader.GetNextCharacter();
		  break;
	   case '/':
		  type = DIVIDE;
		  lexeme[0] = nextCharacter; lexeme[1] = '\0';
		  reader.GetNextCharacter();
		  break;
	   case '%':
		  type = MODULUS;
		  lexeme[0] = nextCharacter; lexeme[1] = '\0';
		  reader.GetNextCharacter();
		  break;
	   case '^':
		  type = POWER;
		  lexeme[0] = nextCharacter; lexeme[1] = '\0';
		  reader.GetNextCharacter();
		  break;
         case '<':
			lexeme[0] = nextCharacter;
			nextCharacter = reader.GetNextCharacter().character;
			if ( nextCharacter == '=' )
			{
			   type = LTEQ;
			   lexeme[1] = nextCharacter; lexeme[2] = '\0';
			   reader.GetNextCharacter();
			}
			else if (nextCharacter == '?')
			{
			   type = LT;
			   lexeme[1] = nextCharacter;
			   lexeme[2] = '\0';
			   reader.GetNextCharacter();
			} else {
				type = DISPLAYOPEN;
				 lexeme[1] = '\0';
			}
			break;

         case '>':
        	 type = DISPLAYCLOSE;
        	 lexeme[0] = nextCharacter;
        	 nextCharacter = reader.GetNextCharacter().character;
			if ( nextCharacter == '=' )
			{
			   type = GTEQ;
			   lexeme[1] = nextCharacter; lexeme[2] = '\0';
			   reader.GetNextCharacter();
			}
			else if (nextCharacter == '?')
			{
			   type = GT;
			   lexeme[1] = nextCharacter;
			   lexeme[2] = '\0';
			   reader.GetNextCharacter();
			} else {
				type = DISPLAYCLOSE;
				 lexeme[1] = '\0';
			}

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
         case ':':
        	 type = COLON;
        	 lexeme[0] = nextCharacter;
        	 lexeme[1] = '\0';
        	 reader.GetNextCharacter();
        	 break;
         case '?':
        	 lexeme[0] = nextCharacter;
        	 nextCharacter = reader.GetNextCharacter().character;
        	 if ( nextCharacter == ':' )
			 {
				 type = QUEMARKELSE;
				 lexeme[1] = nextCharacter;
				 lexeme[2] = '\0';
				 reader.GetNextCharacter();
			 } else {
				 type = QUEMARK;
				 lexeme[1] = '\0';
				 reader.GetNextCharacter();
			 }
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
