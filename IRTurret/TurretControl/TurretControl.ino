#include <Arduino.h>
#include <Servo.h>
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>

#define RECOIL_FIRE_AMOUNT 8 // This is how much the pitch servo moves 3 times for recoil with a 50ms delay

#define DECODE_NEC // Defines the type of IR transmission to decode based on the remote. See IRremote library for examples on how to decode other types of remote

// Codes for buttons on remote
#define left 0x8
#define right 0x5A
#define up 0x52
#define down 0x18
#define ok 0x1C
#define cmd1 0x45
#define cmd2 0x46
#define cmd3 0x47
#define cmd4 0x44
#define cmd5 0x40
#define cmd6 0x43
#define cmd7 0x7
#define cmd8 0x15
#define cmd9 0x9
#define cmd0 0x19
#define star 0x16
#define hashtag 0xD

Servo yawServo; //names the servo responsible for YAW rotation, 360 spin around the base
Servo pitchServo; //names the servo responsible for PITCH rotation, up and down tilt
Servo rollServo; //names the servo responsible for ROLL rotation, spins the barrel to fire darts

int yawServoVal; //initialize variables to store the current value of each servo
int pitchServoVal = 100;
int rollServoVal;

int pitchMoveSpeed = 8; //this variable is the angle added to the pitch servo to control how quickly the PITCH servo moves - try values between 3 and 10
int yawMoveSpeed = 90; //this variable is the speed controller for the continuous movement of the YAW servo motor. It is added or subtracted from the yawStopSpeed, so 0 would mean full speed rotation in one direction, and 180 means full rotation in the other. Try values between 10 and 90;
int yawStopSpeed = 90; //value to stop the yaw motor - keep this at 90
int rollMoveSpeed = 90; //this variable is the speed controller for the continuous movement of the ROLL servo motor. It is added or subtracted from the rollStopSpeed, so 0 would mean full speed rotation in one direction, and 180 means full rotation in the other. Keep this at 90 for best performance / highest torque from the roll motor when firing.
int rollStopSpeed = 90; //value to stop the roll motor - keep this at 90

int yawPrecision = 150; // this variable represents the time in milliseconds that the YAW motor will remain at it's set movement speed. Try values between 50 and 500 to start (500 milliseconds = 1/2 second)
int rollPrecision = 158; // this variable represents the time in milliseconds that the ROLL motor with remain at it's set movement speed. If this ROLL motor is spinning more or less than 1/6th of a rotation when firing a single dart (one call of the fire(); command) you can try adjusting this value down or up slightly, but it should remain around the stock value (160ish) for best results.

int pitchMax = 175; // this sets the maximum angle of the pitch servo to prevent it from crashing, it should remain below 180, and be greater than the pitchMin
int pitchMin = 10; // this sets the minimum angle of the pitch servo to prevent it from crashing, it should remain above 0, and be less than the pitchMax

void setup()
{
   Serial.begin( 9600 );

   yawServo.attach( 10 ); //attach YAW servo to pin 10
   pitchServo.attach( 11 ); //attach PITCH servo to pin 11
   rollServo.attach( 12 ); //attach ROLL servo to pin 12

   IrReceiver.begin( 9, ENABLE_LED_FEEDBACK );

   homeServos();
}

void shakeHeadYes( int moves = 3 )
{
   int startAngle = pitchServoVal; // Current position of the pitch servo
   int lastAngle = pitchServoVal;
   int nodAngle = startAngle + 20; // Angle for nodding motion

   for ( int i = 0; i < moves; i++ )
   {
      // Nod up
      for ( int angle = startAngle; angle <= nodAngle; angle++ )
      {
         pitchServo.write( angle );
         delay( 7 );
      }
      delay( 50 );

      // Nod down
      for ( int angle = nodAngle; angle >= startAngle; angle-- )
      {
         pitchServo.write( angle );
         delay( 7 );
      }

      delay( 50 );
   }
}

void shakeHeadNo( int moves = 3 )
{
   int startAngle = pitchServoVal; // Current position of the pitch servo
   int lastAngle = pitchServoVal;
   int nodAngle = startAngle + 60; // Angle for nodding motion

   for ( int i = 0; i < moves; i++ )
   {
      // rotate right, stop, then rotate left, stop
      yawServo.write( 140 );
      delay( 190 ); // Adjust delay for smoother motion
      yawServo.write( yawStopSpeed );
      delay( 50 );
      yawServo.write( 40 );
      delay( 190 ); // Adjust delay for smoother motion
      yawServo.write( yawStopSpeed );
      delay( 50 ); // Pause at starting position
   }
}

void loop()
{
   if ( IrReceiver.decode() )
   {
      IrReceiver.resume();

      switch ( IrReceiver.decodedIRData.command )
      {
         case up:
         {
            upMove( 1 );
            break;
         }
         case down:
         {
            downMove( 1 );
            break;
         }
         case left:
         {
            leftMove( 1 );
            break;
         }
         case right:
         {
            rightMove( 1 );
            break;
         }
         case ok:
         {
            fire();
            break;
         }
         case star:
         {
            fireAll();
            delay( 50 );
            break;
         }
         case hashtag:
         {
            shakeHeadYes( 3 );
            shakeHeadNo( 3 );
            break;
         }
      }
   }
   delay( 5 );
}

void leftMove( int moves )
{
   for ( int i = 0; i < moves; i++ )
   {
      yawServo.write( yawStopSpeed + yawMoveSpeed ); // adding the servo speed = 180 (full counterclockwise rotation speed)
      delay( yawPrecision ); // stay rotating for a certain number of milliseconds
      yawServo.write( yawStopSpeed ); // stop rotating
      delay( 5 ); //delay for smoothness
   }
}

void rightMove( int moves )
{
   for ( int i = 0; i < moves; i++ )
   {
      yawServo.write( yawStopSpeed - yawMoveSpeed ); //subtracting the servo speed = 0 (full clockwise rotation speed)
      delay( yawPrecision );
      yawServo.write( yawStopSpeed );
      delay( 5 );
   }
}

void upMove( int moves )
{
   for ( int i = 0; i < moves; i++ )
   {
      if ( pitchServoVal > pitchMin ) //make sure the servo is within rotation limits (greater than 10 degrees by default)
      {
         pitchServoVal = pitchServoVal - pitchMoveSpeed; //decrement the current angle and update
         pitchServo.write( pitchServoVal );
         delay( 50 );
      }
   }
}

void downMove( int moves )
{
   for ( int i = 0; i < moves; i++ )
   {
      if ( pitchServoVal < pitchMax ) //make sure the servo is within rotation limits (less than 175 degrees by default)
      {
         pitchServoVal = pitchServoVal + pitchMoveSpeed;//increment the current angle and update
         pitchServo.write( pitchServoVal );
         delay( 50 );
      }
   }
}

void doRecoil()
{
   if ( RECOIL_FIRE_AMOUNT != 0 )
   {
      auto origPitchVal = pitchServoVal;

      for ( auto i = 0; i < 3; i++ )
      {
         pitchServoVal += RECOIL_FIRE_AMOUNT;
         pitchServoVal = min( pitchServoVal, pitchMax );
         pitchServoVal = max( pitchServoVal, pitchMin );
         pitchServo.write( pitchServoVal );
         delay( 50 );
      }

      for ( auto i = 0; i < 3; i++ )
      {
         pitchServoVal -= RECOIL_FIRE_AMOUNT;
         pitchServoVal = min( pitchServoVal, pitchMax );
         pitchServoVal = max( pitchServoVal, pitchMin );
         pitchServo.write( pitchServoVal );
         delay( 50 );
      }

      pitchServo.write( origPitchVal );
   }
}

void fire()
{
   rollServo.write( rollStopSpeed + rollMoveSpeed );//start rotating the servo
   delay( rollPrecision );//time for approximately 60 degrees of rotation
   rollServo.write( rollStopSpeed );//stop rotating the servo

   doRecoil();

   delay( 5 ); //delay for smoothness
}

void fireAll()
{
   rollServo.write( rollStopSpeed + rollMoveSpeed );//start rotating the servo
   delay( rollPrecision * 6 ); //time for 360 degrees of rotation
   rollServo.write( rollStopSpeed );//stop rotating the servo

   doRecoil();

   delay( 5 ); // delay for smoothness
}

void homeServos()
{
   yawServo.write( yawStopSpeed ); //setup YAW servo to be STOPPED (90)
   delay( 20 );
   rollServo.write( rollStopSpeed ); //setup ROLL servo to be STOPPED (90)
   delay( 100 );
   pitchServo.write( 100 ); //set PITCH servo to 100 degree position
   delay( 100 );
   pitchServoVal = 100; // store the pitch servo value
}