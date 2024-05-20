#include <Arduino.h>
#include "TurretControl.h"
#include "TurretRoulette.h"
#include "TurretDance.h"
#include "Utils.h"
#include "BaseProgram.h"
#include <IRremote.hpp>

#define DECODE_NEC // Defines the type of IR transmission to decode based on the remote. See IRremote library for examples on how to decode other types of remote

enum ProgramType { TurretControl, TurretRoulette, TurretDance };

struct ProgramPair {
   ProgramType type;
   BaseProgram* program;
};

ProgramPair programs[] =
{
   { TurretControl, new TurretControlProgram() },
   { TurretRoulette, new TurretRouletteProgram() },
   { TurretDance, new TurretDanceProgram() }
};

bool isSelectingProgram = false;

BaseProgram* currentProgram = nullptr;

void setup()
{
   Serial.begin( 115200 );

   IrReceiver.begin( 9, ENABLE_LED_FEEDBACK );

   currentProgram = GetProgram( TurretControl );

   SetupProgram();
}

BaseProgram* GetProgram( ProgramType type )
{
   for ( int i = 0; i < sizeof( programs ) / sizeof( programs[0] ); i++ )
   {
      if ( programs[i].type == type )
      {
         return programs[i].program;
         break;
      }
   }

   return nullptr;
}

void SetupProgram()
{
   if ( currentProgram != nullptr )
   {
      currentProgram->Setup();
   }
}

bool CanShutdownProgram()
{
   if ( currentProgram != nullptr )
   {
      return currentProgram->CanShutdown();
   }
}

void ShutdownProgram()
{
   if ( currentProgram != nullptr )
   {
      currentProgram->Shutdown();
   }
}

void ChangeProgram( ProgramType newProgramType )
{
   if ( CanShutdownProgram() )
   {
      isSelectingProgram = false;

      auto newProgram = GetProgram( newProgramType );

      if ( newProgram != nullptr )
      {
         currentProgram->Shutdown();

         currentProgram = newProgram;
         currentProgram->Setup();
      }

      isSelectingProgram = false;
   }
}

void ProgramLoop( uint16_t cmd )
{
   currentProgram->Loop( cmd );
}

void loop()
{
   if ( IrReceiver.decode() )
   {
      IrReceiver.resume();

      switch ( IrReceiver.decodedIRData.command )
      {
         case cmd0:
         {
            if ( !isSelectingProgram && CanShutdownProgram() )
            {
               isSelectingProgram = true;
            }
            break;
         }
         case cmd1:
         {
            if ( isSelectingProgram )
            {
               ChangeProgram( TurretControl );
            }
            break;
         }
         case cmd2:
         {
            if ( isSelectingProgram )
            {
               ChangeProgram( TurretRoulette );
            }
            break;
         }
         case cmd3:
         {
            if ( isSelectingProgram )
            {
               ChangeProgram( TurretDance );
            }
            break;
         }
         default:
         {
            break;
         }
      }

      ProgramLoop( IrReceiver.decodedIRData.command );
   }
   else
   {
      ProgramLoop( -1 );
   }

   delay( 5 );
}