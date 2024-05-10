#include <Arduino.h>
#include <Servo.h>
#include "PinDefinitionsAndMore.h"
#include "Utils.h"

#define ROLL_SERVO_PIN  12    // Pin for roll servo
#define ROLL_ZERO_SPEED 90    // Speed to keep roll servo stationary
#define ROLL_MIN_SPEED  45    // Minimum speed away from zero speed needed to get roll servo moving
#define ROLL_MAX_SPEED  90    // Maximum speed away from zero speed allowed for roll servo

#define YAW_SERVO_PIN   10    // Pin for yaw servo
#define YAW_ZERO_SPEED  90    // Speed to keep yaw servo stationary
#define YAW_MIN_SPEED   45    // Minimum speed away from zero speed needed to get yaw servo moving
#define YAW_MAX_SPEED   90    // Maximum speed away from zero speed allowed for yaw servo

#define PITCH_SERVO_PIN 11    // Pin for pitch servo
#define PITCH_MIN_ANGLE 35    // Lowest angle (degrees) allowed for pitch servo
#define PITCH_MAX_ANGLE 170   // Highest angle (degrees) allowed for pitch servo
#define PITCH_MAX_SPEED 300   // Highest speed (degrees/sec) allowed for pitch servo

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
   bool isWaitMove = false;
};

// Dance move for rotating roll and yaw servos
// You can assign a duration and speed
// Duration is milliseconds
// Speed is from 0-1
// You can also use the other constructor to define a duration to wait for
struct DanceSpeedMove : DanceMove
{
public:
   DanceSpeedMove() {}

   DanceSpeedMove( unsigned long dur, double spd )
   {
      duration = dur;
      speed = max( -1, min( spd, 1 ) );
   }

   DanceSpeedMove( unsigned long dur )
   {
      duration = dur;
      isWaitMove = true;
   }
};

// Dance move for rotating pitch servo
// You can assign a target angle and duration
// Target angle (degrees) is the angle you want to end up at
// Duration is milliseconds
// You can also use the other constructor to define a duration to wait for
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

   DanceAngleMove( unsigned long dur )
   {
      duration = dur;
      isWaitMove = true;
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
// minSpd: Minimum speed away from zeroSpd needed to get servo moving. You may need to experient for your own values
// maxSpd: Maximum speed away from zeroSpd needed to get servo moving. You may need to experient for your own values
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

   ~ServoSpeedController()
   {
      servo.detach();
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

         if ( !move.isWaitMove )
         {
            MoveTo( speed );
         }
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

   ~ServoAngleController()
   {
      servo.detach();
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
      if ( move.speed < 0 )
      {
         newPosition = max( targetAngle, newPosition );
      }
      else
      {
         newPosition = min( targetAngle, newPosition );
      }

      if ( animTimeElapsed >= move.duration )
      {
         currentMoveIndex++;
         startMoveTime = millis();
      }

      if ( !move.isWaitMove )
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

void SetDanceRoutine1()
{
   _rollServo->Reset();
   _yawServo->Reset();
   _pitchServo->Reset();

   DanceSpeedMove rollMoves[] =
   {
      DanceSpeedMove( 3000 ),
      DanceSpeedMove( 2000, .4 ),
      DanceSpeedMove( 2000, -.4 ),
      DanceSpeedMove( 2000, .4 ),
      DanceSpeedMove( 2000, -.4 ),
   };

   DanceSpeedMove yawMoves[] =
   {
      DanceSpeedMove( 3000 ),
      DanceSpeedMove( 500, .2 ),
      DanceSpeedMove( 500, -.2 ),
      DanceSpeedMove( 500, .2 ),
      DanceSpeedMove( 500, -.2 ),
   };

   DanceAngleMove pitchMoves[] =
   {
      DanceAngleMove( 93, 1000 ),
      DanceAngleMove( 2000 ),
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
      DanceSpeedMove( 3000 ),
      DanceSpeedMove( 2000, .4 ),
      DanceSpeedMove( 2000, -.4 ),
   };

   DanceSpeedMove yawMoves[] =
   {
      DanceSpeedMove( 3000 ),
      DanceSpeedMove( 500, .2 ),
      DanceSpeedMove( 500, -.2 ),
   };

   DanceAngleMove pitchMoves[] =
   {
      DanceAngleMove( 93, 1000 ),
      DanceAngleMove( 2000 ),
      DanceAngleMove( 150, 1000 ),
      DanceAngleMove( 35, 2000 ),
      DanceAngleMove( 150, 1000 ),
   };

   _rollServo->SetDanceMoves( rollMoves, sizeof( rollMoves ) / sizeof( DanceSpeedMove ) );
   _yawServo->SetDanceMoves( yawMoves, sizeof( yawMoves ) / sizeof( DanceSpeedMove ) );
   _pitchServo->SetDanceMoves( pitchMoves, sizeof( pitchMoves ) / sizeof( DanceAngleMove ) );
}

bool DanceCanChangeProgram()
{
   return !_playing;
}

void DanceSetup()
{
   _rollServo = new ServoSpeedController( ROLL_SERVO_PIN, ROLL_ZERO_SPEED, ROLL_MIN_SPEED, ROLL_MAX_SPEED );
   _yawServo = new ServoSpeedController( YAW_SERVO_PIN, YAW_ZERO_SPEED, YAW_MIN_SPEED, YAW_MAX_SPEED );
   _pitchServo = new ServoAngleController( PITCH_SERVO_PIN, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE, PITCH_MAX_SPEED );
}

void DanceShutdown()
{
   delete _rollServo;
   delete _yawServo;
   delete _pitchServo;
}

void DanceLoop( uint16_t cmd )
{
   if ( _playing )
   {
      auto donePlaying = _rollServo->Update();
      donePlaying &= _yawServo->Update();
      donePlaying &= _pitchServo->Update();

      _playing = !donePlaying;
   }

   if ( cmd != -1 )
   {
      switch ( cmd )
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
            _playing = false;
            _rollServo->Reset();
            _yawServo->Reset();
            _pitchServo->Reset();
         }
      }
   }

   delay( 10 );
}