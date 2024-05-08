#include <Servo.h>
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>

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

// Map a ration to a range. For example, .5 should fall in the middle of a range if the input can be 0-1
double mapDouble( double x, double in_min, double in_max, double out_min, double out_max )
{
   return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

struct DanceMove
{
public:
   unsigned long duration;
   double speed;
   bool started = false;
};

// Dance move for rotating roll and yaw servos
// You can assign a duration and speed
// Duration is milliseconds
// Speed is from 0-1
struct DanceSpeedMove : DanceMove
{
public:
   DanceSpeedMove() {}

   DanceSpeedMove( unsigned long dur, double spd = 0.0 )
   {
      duration = dur;
      speed = max( -1, min( spd, 1 ) );
   }
};

// Dance move for rotating pitch servo
// You can assign a target angle and duration
// Target angle (degrees) is the angle you want to end up at
// Duration is milliseconds
struct DanceAngleMove : DanceMove
{
public:
   int targetAngle;

   DanceAngleMove() {}

   DanceAngleMove( int targetAng, unsigned long dur )
      : targetAngle( targetAng )
   {
      duration = dur;
   }
};

class ServoController
{
public:
   virtual void Reset() = 0;
   virtual bool Update() = 0;

protected:
   Servo servo;
   int numMoves = 0;
   int currentMoveIndex = 0;
   unsigned long startMoveTime;
   unsigned long lastTime;
   double currentPosition;
   int maxSpeed;

   virtual void MoveTo( double position ) = 0;
};

// Controller to define properties for a servo that lets you set the speed
// and rotates 360 degrees. This is for the roll and yaw servos.
// pin: Arduino pin for servo
// zeroSpd: Speed that is used to keep servo stationary
// minSpd: Minimum speed needed to get servo moving. You may need to experient for your own values
// maxSpd: Maximum speed needed to get servo moving. You may need to experient for your own values
// moveArray: Array of dance moves to perform
class ServoSpeedController : ServoController
{
private:
   DanceSpeedMove* moves = nullptr;
   int zeroSpeed;
   int minSpeed;

   void MoveTo( double position ) override
   {
      servo.write( position );
      currentPosition = position;
   }

public:
   ServoSpeedController( int pin, int zeroSpd, int minSpd, int maxSpd )
      : zeroSpeed( zeroSpd ), minSpeed( minSpd )
   {
      maxSpeed = maxSpd;
      servo.attach( pin );
   }

   void SetDanceMoves( DanceSpeedMove moveArray[], int moveCount )
   {
      Reset();
      moves = new DanceSpeedMove[moveCount];
      memcpy( moves, moveArray, moveCount * sizeof( DanceSpeedMove ) );
      numMoves = moveCount;
   }

   void Reset() override
   {
      MoveTo( zeroSpeed );
      lastTime = millis();
      startMoveTime = lastTime;
      currentMoveIndex = 0;

      if ( moves != nullptr )
      {
         delete[] moves;
         moves = nullptr;
      }
      numMoves = 0;
   }

   bool Update() override
   {
      if ( currentMoveIndex >= numMoves )
      {
         return true;
      }

      unsigned long currentTime = millis();
      DanceSpeedMove& move = moves[currentMoveIndex];
      unsigned long timeElapsed = currentTime - lastTime;
      unsigned long animTimeElapsed = currentTime - startMoveTime;

      if ( !move.started )
      {
         move.started = true;

         double speed = zeroSpeed;
         if ( move.speed > 0 )
         {
            speed = mapDouble( move.speed, 0, 1, zeroSpeed + minSpeed, zeroSpeed + maxSpeed );
         }
         else if ( move.speed < 0 )
         {
            speed = mapDouble( move.speed, -1, 0, zeroSpeed - maxSpeed, zeroSpeed - minSpeed );
         }

         MoveTo( speed );
      }

      if ( animTimeElapsed >= move.duration )
      {
         currentMoveIndex++;
         startMoveTime = millis();
         MoveTo( zeroSpeed );
      }

      lastTime = currentTime;

      return false;
   }
};

// Controller to define properties for a servo that lets you set an angle.
// This is for the pitch servo.
// pin: Arduino pin for servo
// minAng: Minimum angle allowed. Prevents rotating too much in one direction.
// maxAng: Maximum angle allowed. Prevents rotating too much in one direction.
// maxSpde: Maximum degrees/sec movement allowed
// moveArray: Array of dance moves to perform
class ServoAngleController : ServoController
{
private:
   DanceAngleMove* moves = nullptr;
   int minAngle;
   int maxAngle;

   void MoveTo( double position ) override
   {
      position = max( minAngle, min( maxAngle, position ) );
      servo.write( position );
      currentPosition = position;
   }

public:
   ServoAngleController( int pin, int minAng, int maxAng, int maxSpd )
      : minAngle( minAng ), maxAngle( maxAng )
   {
      maxSpeed = maxSpd;
      servo.attach( pin );

      MoveTo( minAngle + (double( maxAngle - minAngle ) / 2.0) );
   }

   void SetDanceMoves( DanceAngleMove moveArray[], int moveCount )
   {
      Reset();
      moves = new DanceAngleMove[moveCount];
      memcpy( moves, moveArray, moveCount * sizeof( DanceAngleMove ) );
      numMoves = moveCount;
   }

   void Reset() override
   {
      lastTime = millis();
      startMoveTime = lastTime;
      currentMoveIndex = 0;

      if ( moves != nullptr )
      {
         delete[] moves;
         moves = nullptr;
      }
      numMoves = 0;
   }

   bool Update() override
   {
      if ( currentMoveIndex >= numMoves )
      {
         return true;
      }

      unsigned long currentTime = millis();
      DanceAngleMove& move = moves[currentMoveIndex];
      unsigned long timeElapsed = currentTime - lastTime;
      double secsElapsed = (double)timeElapsed / 1000.0;
      auto targetAngle = max( minAngle, min( maxAngle, move.targetAngle ) );
      unsigned long animTimeElapsed = currentTime - startMoveTime;

      if ( !move.started )
      {
         move.started = true;
         move.speed = ((double)targetAngle - currentPosition) / ((double)move.duration / 1000.0);
         move.speed = min( maxSpeed, move.speed );
      }

      auto amtToMove = move.speed * secsElapsed;
      auto newPosition = currentPosition + amtToMove;

      if ( animTimeElapsed >= move.duration )
      {
         currentMoveIndex++;
         startMoveTime = millis();
         MoveTo( targetAngle );
      }
      else
      {
         MoveTo( newPosition );
      }

      lastTime = currentTime;

      return false;
   }
};

ServoSpeedController* _rollServo;
ServoSpeedController* _yawServo;
ServoAngleController* _pitchServo;

bool _playing = false;

void SetResetRoutine()
{
   _rollServo->Reset();
   _yawServo->Reset();
   _pitchServo->Reset();

   DanceSpeedMove rollMoves[] =
   {
   };

   DanceSpeedMove yawMoves[] =
   {
   };

   DanceAngleMove pitchMoves[] =
   {
       DanceAngleMove( 93, 1000 ),
   };

   _rollServo->SetDanceMoves( rollMoves, sizeof( rollMoves ) / sizeof( DanceSpeedMove ) );
   _yawServo->SetDanceMoves( yawMoves, sizeof( yawMoves ) / sizeof( DanceSpeedMove ) );
   _pitchServo->SetDanceMoves( pitchMoves, sizeof( pitchMoves ) / sizeof( DanceAngleMove ) );
}

void SetDanceRoutine1()
{
   _rollServo->Reset();
   _yawServo->Reset();
   _pitchServo->Reset();

   DanceSpeedMove rollMoves[] =
   {
     DanceSpeedMove( 5000, .4 ),
     DanceSpeedMove( 5000, -.4 ),
     DanceSpeedMove( 5000, .4 ),
     DanceSpeedMove( 5000, -.4 ),
   };

   DanceSpeedMove yawMoves[] =
   {
     DanceSpeedMove( 500, .2 ),
     DanceSpeedMove( 500, -.2 ),
     DanceSpeedMove( 500, .2 ),
     DanceSpeedMove( 500, -.2 ),
   };

   DanceAngleMove pitchMoves[] =
   {
       DanceAngleMove( 150, 1000 ),
       DanceAngleMove( 35, 2000 ),
       DanceAngleMove( 150, 1000 ),
       DanceAngleMove( 150, 1000 ),
       DanceAngleMove( 35, 2000 ),
       DanceAngleMove( 150, 1000 ),
   };

   _rollServo->SetDanceMoves( rollMoves, sizeof( rollMoves ) / sizeof( DanceSpeedMove ) );
   _yawServo->SetDanceMoves( yawMoves, sizeof( yawMoves ) / sizeof( DanceSpeedMove ) );
   _pitchServo->SetDanceMoves( pitchMoves, sizeof( pitchMoves ) / sizeof( DanceAngleMove ) );
}

void SetDanceRoutine2()
{
   _rollServo->Reset();
   _yawServo->Reset();
   _pitchServo->Reset();

   DanceSpeedMove rollMoves[] =
   {
      DanceSpeedMove( 5000, .4 ),
      DanceSpeedMove( 5000, -.4 ),
   };

   DanceSpeedMove yawMoves[] =
   {
      DanceSpeedMove( 500, .2 ),
      DanceSpeedMove( 500, -.2 ),
   };

   DanceAngleMove pitchMoves[] =
   {
       DanceAngleMove( 150, 1000 ),
       DanceAngleMove( 35, 2000 ),
       DanceAngleMove( 150, 1000 ),
   };

   _rollServo->SetDanceMoves( rollMoves, sizeof( rollMoves ) / sizeof( DanceSpeedMove ) );
   _yawServo->SetDanceMoves( yawMoves, sizeof( yawMoves ) / sizeof( DanceSpeedMove ) );
   _pitchServo->SetDanceMoves( pitchMoves, sizeof( pitchMoves ) / sizeof( DanceAngleMove ) );
}

void setup()
{
   Serial.begin( 115200 );

   _rollServo = new ServoSpeedController( 12, 90, 45, 90 );
   _yawServo = new ServoSpeedController( 10, 90, 45, 90 );
   _pitchServo = new ServoAngleController( 11, 35, 170, 100 );

   IrReceiver.begin( 9, ENABLE_LED_FEEDBACK );
}

void loop()
{
   if ( _playing )
   {
      auto donePlaying = _rollServo->Update();
      donePlaying &= _yawServo->Update();
      donePlaying &= _pitchServo->Update();

      _playing = !donePlaying;
   }

   if ( IrReceiver.decode() )
   {
      IrReceiver.resume();

      switch ( IrReceiver.decodedIRData.command )
      {
         case cmd1:
         {
            if ( !_playing )
            {
               SetDanceRoutine1();
               _playing = true;
            }
            break;
         }
         case cmd2:
         {
            if ( !_playing )
            {
               SetDanceRoutine2();
               _playing = true;
            }
            break;
         }
         case ok:
         {
            _playing = true;
            SetResetRoutine();
         }
      }
   }

   delay( 10 );
}