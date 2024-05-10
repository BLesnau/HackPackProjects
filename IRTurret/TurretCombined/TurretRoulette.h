#include <Arduino.h>
#include "PinDefinitionsAndMore.h"
#include "Utils.h"
#include <Servo.h>

#define RECOIL_FIRE_AMOUNT2 8 // This is how much the pitch servo moves 3 times for recoil with a 50ms delay

Servo yawServo2; //names the servo responsible for YAW rotation, 360 spin around the base
Servo pitchServo2; //names the servo responsible for PITCH rotation, up and down tilt
Servo rollServo2; //names the servo responsible for ROLL rotation, spins the barrel to fire2 darts

int yawServo2Val; //initialize variables to store the current value of each servo
int pitchServo2Val = 100;
int rollServo2Val;

int pitch2MoveSpeed = 8; //this variable is the angle added to the pitch servo to control how quickly the PITCH servo moves - try values between 3 and 10
int yaw2MoveSpeed = 90; //this variable is the speed controller for the continuous movement of the YAW servo motor. It is added or subtracted from the yaw2StopSpeed, so 0 would mean full speed rotation in one direction, and 180 means full rotation in the other. Try values between 10 and 90;
int yaw2StopSpeed = 90; //value to stop the yaw motor - keep this at 90
int roll2MoveSpeed = 90; //this variable is the speed controller for the continuous movement of the ROLL servo motor. It is added or subtracted from the roll2StopSpeed, so 0 would mean full speed rotation in one direction, and 180 means full rotation in the other. Keep this at 90 for best performance / highest torque from the roll motor when firing.
int roll2StopSpeed = 90; //value to stop the roll motor - keep this at 90

int pitch2Max = 175; // this sets the maximum angle of the pitch servo to prevent it from crashing, it should remain below 180, and be greater than the pitch2Min
int pitch2Min = 10; // this sets the minimum angle of the pitch servo to prevent it from crashing, it should remain above 0, and be less than the pitch2Max

bool isPlaying2 = false;

void shakeHeadYes2( int moves = 3 )
{
   int startAngle = pitchServo2Val; // Current position of the pitch servo
   int lastAngle = pitchServo2Val;
   int nodAngle = startAngle + 20; // Angle for nodding motion

   for ( int i = 0; i < moves; i++ )
   {
      // Nod up
      for ( int angle = startAngle; angle <= nodAngle; angle++ )
      {
         pitchServo2.write( angle );
         delay( 7 );
      }
      delay( 50 );

      // Nod down
      for ( int angle = nodAngle; angle >= startAngle; angle-- )
      {
         pitchServo2.write( angle );
         delay( 7 );
      }

      delay( 50 );
   }
}

void shakeHeadNo2( int moves = 3 )
{
   int startAngle = pitchServo2Val; // Current position of the pitch servo
   int lastAngle = pitchServo2Val;
   int nodAngle = startAngle + 60; // Angle for nodding motion

   for ( int i = 0; i < moves; i++ )
   {
      // rotate right, stop, then rotate left, stop
      yawServo2.write( 140 );
      delay( 190 ); // Adjust delay for smoother motion
      yawServo2.write( yaw2StopSpeed );
      delay( 50 );
      yawServo2.write( 40 );
      delay( 190 ); // Adjust delay for smoother motion
      yawServo2.write( yaw2StopSpeed );
      delay( 50 ); // Pause at starting position
   }
}

void doRecoil2()
{
   if ( RECOIL_FIRE_AMOUNT2 != 0 )
   {
      auto origPitchVal = pitchServo2Val;

      for ( auto i = 0; i < 3; i++ )
      {
         pitchServo2Val += RECOIL_FIRE_AMOUNT2;
         pitchServo2Val = min( pitchServo2Val, pitch2Max );
         pitchServo2Val = max( pitchServo2Val, pitch2Min );
         pitchServo2.write( pitchServo2Val );
         delay( 50 );
      }

      for ( auto i = 0; i < 3; i++ )
      {
         pitchServo2Val -= RECOIL_FIRE_AMOUNT2;
         pitchServo2Val = min( pitchServo2Val, pitch2Max );
         pitchServo2Val = max( pitchServo2Val, pitch2Min );
         pitchServo2.write( pitchServo2Val );
         delay( 50 );
      }

      pitchServo2.write( origPitchVal );
   }
}

void fire2()
{
   rollServo2.write( 180 );//start rotating the servo
   delay( 150 );//time for approximately 60 degrees of rotation
   rollServo2.write( 90 );//stop rotating the servo

   doRecoil2();

   delay( 5 );
}

void fireAll2()
{
   rollServo2.write( 180 );//start rotating the servo
   delay( 1500 );//time for 360 degrees of rotation
   rollServo2.write( 90 );//stop rotating the servo

   doRecoil2();

   delay( 5 );
}

void spinAndFire2()
{
   unsigned long startTime = millis();
   pitchServo2.write( 90 );
   yawServo2Val = 180;
   yawServo2.write( yawServo2Val );
   delay( 20 ); // Adjust delay for smoother movement
   while ( millis() - startTime < 10000 )
   {
      if ( millis() % 1000 == 0 && yawServo2Val > 90 )
      {
         yawServo2Val--;
         yawServo2.write( yawServo2Val );
      }
   }
   yawServo2.write( 90 );

   bool shoot = random( 2 ) == 1;
   if ( shoot )
   {
      shakeHeadYes2();
      delay( 1000 );
      fire2();
   }
   else
   {
      shakeHeadNo2();
      delay( 1000 );

      shoot = random( 1, 11 ) == 1;
      if ( shoot )
      {
         yawServo2.write( 150 );
         delay( 500 );
         yawServo2.write( 30 );
         delay( 450 );
         yawServo2.write( 90 );
         pitchServo2.write( 90 );
         fireAll2();
      }
      else
      {
         spinAndFire2();
      }
   }
}

bool RouletteCanChangeProgram()
{
   return !isPlaying2;
}

void RouletteSetup()
{
   yawServo2.attach( 10 ); //attach YAW servo to pin 3
   pitchServo2.attach( 11 ); //attach PITCH servo to pin 4
   rollServo2.attach( 12 ); //attach ROLL servo to pin 5

   yawServo2.write( 90 ); //setup YAW servo to be STOPPED (90)
   delay( 20 );
   rollServo2.write( 90 ); //setup ROLL servo to be STOPPED (90)
   delay( 100 );
   pitchServo2.write( 100 ); //set PITCH servo to 100 degree position
   delay( 100 );
   pitchServo2Val = 100;

   randomSeed( analogRead( 0 ) );
}

void RouletteShutdown()
{
   yawServo2.detach();
   pitchServo2.detach();
   rollServo2.detach();
}

void RouletteLoop( uint16_t cmd )
{
   if ( cmd != -1 )
   {
      switch ( cmd )
      {
         case up:
         {
            if ( pitchServo2Val > 10 )
            {
               pitchServo2Val = pitchServo2Val - 8;
               pitchServo2.write( pitchServo2Val );
               delay( 50 );
            }
            break;
         }
         case down:
         {
            if ( pitchServo2Val < 175 )
            {
               pitchServo2Val = pitchServo2Val + 8;
               pitchServo2.write( pitchServo2Val );
               delay( 50 );
            }
            break;
         }
         case left:
         {
            yawServo2.write( 180 );
            delay( 200 );
            yawServo2.write( 90 );
            delay( 5 );
            break;
         }
         case right:
         {
            yawServo2.write( 0 );
            delay( 200 );
            yawServo2.write( 90 );
            delay( 5 );
            break;
         }
         case ok:
         {
            if ( !isPlaying2 )
            {
               isPlaying2 = true;
               fire2();
               isPlaying2 = false;
            }
            break;
         }
         case star:
         {
            if ( !isPlaying2 )
            {
               isPlaying2 = true;
               fireAll2();
               delay( 50 );
               isPlaying2 = false;
            }
            break;
         }
         case hashtag:
         {
            if ( !isPlaying2 )
            {
               isPlaying2 = true;
               spinAndFire2();
               isPlaying2 = false;
            }
            break;
         }
         default:
         {
            break;
         }
      }
   }
   delay( 5 );
}