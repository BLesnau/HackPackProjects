#include <Arduino.h>
#include "TurretControl.h"
#include "TurretRoulette.h"
#include "TurretDance.h"
#include "Utils.h"
#include <IRremote.hpp>

#define DECODE_NEC // Defines the type of IR transmission to decode based on the remote. See IRremote library for examples on how to decode other types of remote

enum ProgramType { TurretControl, TurretRoulette, TurretDance };

ProgramType currentProgram = ProgramType::TurretControl;
bool isSelectingProgram = false;

void setup()
{
   Serial.begin( 115200 );

   IrReceiver.begin( 9, ENABLE_LED_FEEDBACK );

   ControlSetup();
}

bool CanChangeProgram()
{
   if ( currentProgram == ProgramType::TurretControl )
   {
      return ControlCanChangeProgram();
   }
   if ( currentProgram == ProgramType::TurretRoulette )
   {
      return RouletteCanChangeProgram();
   }
   if ( currentProgram == ProgramType::TurretDance )
   {
      return DanceCanChangeProgram();
   }

   return true;
}

void ChangeProgram( ProgramType newProgram )
{
   if ( CanChangeProgram() )
   {
      if ( currentProgram == ProgramType::TurretControl )
      {
         ControlShutdown();
      }
      if ( currentProgram == ProgramType::TurretRoulette )
      {
         RouletteShutdown();
      }
      if ( currentProgram == ProgramType::TurretDance )
      {
         DanceShutdown();
      }

      if ( newProgram == ProgramType::TurretControl )
      {
         ControlSetup();
      }
      if ( newProgram == ProgramType::TurretRoulette )
      {
         RouletteSetup();
      }
      if ( newProgram == ProgramType::TurretDance )
      {
         DanceSetup();
      }

      currentProgram = newProgram;
      isSelectingProgram = false;
   }
}

void ProgramLoop( uint16_t cmd )
{
   if ( currentProgram == ProgramType::TurretControl )
   {
      return ControlLoop( cmd );
   }
   if ( currentProgram == ProgramType::TurretRoulette )
   {
      return RouletteLoop( cmd );
   }
   if ( currentProgram == ProgramType::TurretDance )
   {
      return DanceLoop( cmd );
   }
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
            if ( !isSelectingProgram && CanChangeProgram() )
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