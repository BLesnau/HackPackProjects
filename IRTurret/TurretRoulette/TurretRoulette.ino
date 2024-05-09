#include <Arduino.h>
#include <Servo.h>
#include <IRremote.hpp>

#define RECOIL_FIRE_AMOUNT 5 // This is how much the pitch servo moves 3 times for recoil with a 50ms delay

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

int pitchMax = 175; // this sets the maximum angle of the pitch servo to prevent it from crashing, it should remain below 180, and be greater than the pitchMin
int pitchMin = 10; // this sets the minimum angle of the pitch servo to prevent it from crashing, it should remain above 0, and be less than the pitchMax

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

void setup()
{
   Serial.begin( 9600 );

   yawServo.attach( 10 ); //attach YAW servo to pin 3
   pitchServo.attach( 11 ); //attach PITCH servo to pin 4
   rollServo.attach( 12 ); //attach ROLL servo to pin 5

   IrReceiver.begin( 9, ENABLE_LED_FEEDBACK );

   yawServo.write( 90 ); //setup YAW servo to be STOPPED (90)
   delay( 20 );
   rollServo.write( 90 ); //setup ROLL servo to be STOPPED (90)
   delay( 100 );
   pitchServo.write( 100 ); //set PITCH servo to 100 degree position
   delay( 100 );
   pitchServoVal = 100;

   randomSeed( analogRead( 0 ) );
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
            if ( pitchServoVal > 10 )
            {
               pitchServoVal = pitchServoVal - 8;
               pitchServo.write( pitchServoVal );
               delay( 50 );
            }
            break;
         }
         case down:
         {
            if ( pitchServoVal < 175 )
            {
               pitchServoVal = pitchServoVal + 8;
               pitchServo.write( pitchServoVal );
               delay( 50 );
            }
            break;
         }
         case left:
         {
            yawServo.write( 180 );
            delay( 200 );
            yawServo.write( 90 );
            delay( 5 );
            break;
         }
         case right:
         {
            yawServo.write( 0 );
            delay( 200 );
            yawServo.write( 90 );
            delay( 5 );
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
            spinAndFire();
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
   rollServo.write( 180 );//start rotating the servo
   delay( 150 );//time for approximately 60 degrees of rotation
   rollServo.write( 90 );//stop rotating the servo
   delay( 5 );
}

void fireAll()
{
   rollServo.write( 180 );//start rotating the servo
   delay( 1500 );//time for 360 degrees of rotation
   rollServo.write( 90 );//stop rotating the servo
   delay( 5 );
}

void spinAndFire()
{
   unsigned long startTime = millis();
   pitchServo.write( 90 );
   yawServoVal = 180;
   yawServo.write( yawServoVal );
   delay( 20 ); // Adjust delay for smoother movement
   while ( millis() - startTime < 10000 )
   {
      if ( millis() % 1000 == 0 && yawServoVal > 90 )
      {
         yawServoVal--;
         yawServo.write( yawServoVal );
      }
   }
   yawServo.write( 90 );

   bool shoot = random( 2 ) == 1;
   if ( shoot )
   {
      shakeHeadYes();
      delay( 1000 );
      fire();
   }
   else
   {
      shakeHeadNo();
      delay( 1000 );

      shoot = random( 1, 11 ) == 1;
      if ( shoot )
      {
         yawServo.write( 150 );
         delay( 500 );
         yawServo.write( 30 );
         delay( 450 );
         yawServo.write( 90 );
         pitchServo.write( 90 );
         fireAll();
      }
      else
      {
         spinAndFire();
      }
   }
}