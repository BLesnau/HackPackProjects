#include <Servo.h>
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>

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

struct DanceMove
{
public:
   uint16_t duration;
   int8_t speed;
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

   DanceSpeedMove( uint16_t dur, int8_t spd )
   {
      duration = dur;
      speed = max( -100, min( spd, 100 ) );
   }

   DanceSpeedMove( uint16_t dur )
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
   uint8_t targetAngle;

   DanceAngleMove() {}

   DanceAngleMove( uint8_t targetAng, uint16_t dur )
      : targetAngle( targetAng )
   {
      duration = dur;
   }

   DanceAngleMove( uint16_t dur )
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
   uint16_t numMoves = 0;
   uint16_t currentMoveIndex = 0;
   uint16_t startMoveTime;
   uint16_t lastTime;
   uint8_t currentPosition;
   uint16_t maxSpeed;

   virtual void MoveTo( uint8_t position ) = 0;
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
   uint8_t zeroSpeed;
   uint8_t minSpeed;

   void MoveTo( uint8_t position ) override
   {
      servo.write( position );
      currentPosition = position;
   }

public:
   ServoSpeedController( uint8_t pin, uint8_t zeroSpd, uint8_t minSpd, uint8_t maxSpd )
      : zeroSpeed( zeroSpd ), minSpeed( minSpd )
   {
      maxSpeed = maxSpd;
      servo.attach( pin );
   }

   ~ServoSpeedController()
   {
      servo.detach();
   }

   void SetDanceMoves( DanceSpeedMove moveArray[], uint16_t moveCount )
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

      uint16_t currentTime = millis();
      DanceSpeedMove& move = moves[currentMoveIndex];
      uint16_t timeElapsed = currentTime - lastTime;
      uint16_t animTimeElapsed = currentTime - startMoveTime;

      if ( !move.started )
      {
         move.started = true;

         uint8_t speed = zeroSpeed;
         if ( move.speed > 0 )
         {
            speed = map( move.speed, 0, 100, zeroSpeed + minSpeed, zeroSpeed + maxSpeed );
         }
         else if ( move.speed < 0 )
         {
            speed = map( move.speed, -1, 0, zeroSpeed - maxSpeed, zeroSpeed - minSpeed );
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
// maxSpd: Maximum degrees/sec movement allowed
// moveArray: Array of dance moves to perform
class ServoAngleController : ServoController
{
private:
   DanceAngleMove* moves = nullptr;
   uint8_t minAngle;
   uint8_t maxAngle;

   void MoveTo( uint8_t position ) override
   {
      position = max( minAngle, min( maxAngle, position ) );
      servo.write( position );
      currentPosition = position;
   }

public:
   ServoAngleController( uint8_t pin, uint8_t minAng, uint8_t maxAng, uint16_t maxSpd )
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

         auto spd = move.speed;
         if ( move.speed < 0 )
         {
            move.speed = max( -maxSpeed, move.speed );
         }
         else
         {
            move.speed = min( maxSpeed, move.speed );
         }
      }

      auto amtToMove = (double)move.speed * secsElapsed;

      auto minMove = 1;
      if ( (amtToMove >= minMove) || (animTimeElapsed >= move.duration) )
      {
         auto newPosition = currentPosition + amtToMove;
         if ( move.speed < 0 )
         {
            newPosition = max( targetAngle, newPosition );
         }
         else
         {
            newPosition = min( targetAngle, newPosition );
         }

         if ( !move.isWaitMove )
         {
            MoveTo( newPosition );
         }

         lastTime = currentTime;
      }

      if ( animTimeElapsed >= move.duration )
      {
         currentMoveIndex++;
         startMoveTime = millis();
      }

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

   uint8_t topPitch = 110;
   uint8_t bottomPitch = 90;
   uint8_t yu = 20;
   uint8_t ru = 10;
   uint16_t du = 100;

   DanceSpeedMove rollMoves[] =
   {
      DanceSpeedMove( 108 * du ),      // 10.8
      DanceSpeedMove( 40 * du, 50 ),
      DanceSpeedMove( 40 * du, -50 ),
      DanceSpeedMove( 40 * du, 50 ),
      DanceSpeedMove( 40 * du, -50 ),
   };

   DanceSpeedMove yawMoves[] =
   {
      DanceSpeedMove( 40 * du ),          // 4
      DanceSpeedMove( 4 * du ),           // 4.4
      DanceSpeedMove( 2 * du, 4 * yu ),   // 4.8
      DanceSpeedMove( 8 * du ),           // 5.7
      DanceSpeedMove( 2 * du, -4 * yu ),  // 5.8
      DanceSpeedMove( 8 * du ),           // 6.7
      DanceSpeedMove( 2 * du, 4 * yu ),   // 6.8
      DanceSpeedMove( 8 * du ),
      DanceSpeedMove( 2 * du, -4 * yu ),  // 7.8
      DanceSpeedMove( 8 * du ),
      DanceSpeedMove( 2 * du, 4 * yu ),   // 8.8
      DanceSpeedMove( 8 * du ),
      DanceSpeedMove( 2 * du, -4 * yu ),  // 9.8
      DanceSpeedMove( 8 * du ),
      DanceSpeedMove( 2 * du, 4 * yu ),   // 10.8
   };

   DanceAngleMove pitchMoves[] =
   {
      DanceAngleMove( topPitch, 10 * du ),
      DanceAngleMove( 30 * du ),                // 4
      DanceAngleMove( bottomPitch, 4 * du ),    // 4.4
      DanceAngleMove( topPitch, du ),           // 4.5
      DanceAngleMove( 5 * du ),                 // 5
      DanceAngleMove( bottomPitch, 4 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 5 * du ),                 // 6
      DanceAngleMove( bottomPitch, 4 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 5 * du ),                 // 7
      DanceAngleMove( bottomPitch, 4 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 5 * du ),                 // 8
      DanceAngleMove( bottomPitch, 4 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 5 * du ),                 // 9
      DanceAngleMove( bottomPitch, 4 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 5 * du ),                 // 10
      DanceAngleMove( bottomPitch, 8 * du ),  // 10.8
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

   uint8_t topPitch = 120;
   uint8_t bottomPitch = 80;
   uint8_t yu = 20;
   uint8_t ru = 10;
   uint16_t du = 100;

   DanceSpeedMove rollMoves[] =
   {
   };

   DanceSpeedMove yawMoves[] =
   {
   };

   DanceAngleMove pitchMoves[] =
   {
      DanceAngleMove( topPitch, 10 * du ),
      DanceAngleMove( 30 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
      DanceAngleMove( 2 * du ),
      DanceAngleMove( bottomPitch, 2 * du ),
      DanceAngleMove( topPitch, du ),
   };

   _rollServo->SetDanceMoves( rollMoves, sizeof( rollMoves ) / sizeof( DanceSpeedMove ) );
   _yawServo->SetDanceMoves( yawMoves, sizeof( yawMoves ) / sizeof( DanceSpeedMove ) );
   _pitchServo->SetDanceMoves( pitchMoves, sizeof( pitchMoves ) / sizeof( DanceAngleMove ) );
}

void SetDanceRoutine4()
{
   _rollServo->Reset();
   _yawServo->Reset();
   _pitchServo->Reset();

   uint8_t topPitch = 110;
   uint8_t bottomPitch = 80;
   uint8_t yu = 20;
   uint8_t ru = 10;
   uint16_t du = 100;

   DanceSpeedMove rollMoves[] =
   {
   };

   DanceSpeedMove yawMoves[] =
   {
      DanceSpeedMove( 34 * du ),
      DanceSpeedMove( 6 * du, 4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( 6 * du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
      DanceSpeedMove( du, 4 * yu ),
      DanceSpeedMove( du, -4 * yu ),
   };

   DanceAngleMove pitchMoves[] =
   {
      DanceAngleMove( topPitch, 10 * du ),
      DanceAngleMove( 30 * du ),
      DanceAngleMove( bottomPitch, 10 * du ),
      DanceAngleMove( 186 * du ),
      DanceAngleMove( topPitch, 10 * du ),
   };

   _rollServo->SetDanceMoves( rollMoves, sizeof( rollMoves ) / sizeof( DanceSpeedMove ) );
   _yawServo->SetDanceMoves( yawMoves, sizeof( yawMoves ) / sizeof( DanceSpeedMove ) );
   _pitchServo->SetDanceMoves( pitchMoves, sizeof( pitchMoves ) / sizeof( DanceAngleMove ) );
}

void setup()
{
   Serial.begin( 115200 );

   _rollServo = new ServoSpeedController( ROLL_SERVO_PIN, ROLL_ZERO_SPEED, ROLL_MIN_SPEED, ROLL_MAX_SPEED );
   _yawServo = new ServoSpeedController( YAW_SERVO_PIN, YAW_ZERO_SPEED, YAW_MIN_SPEED, YAW_MAX_SPEED );
   _pitchServo = new ServoAngleController( PITCH_SERVO_PIN, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE, PITCH_MAX_SPEED );

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
         case cmd4:
         {
            if ( !_playing )
            {
               SetDanceRoutine4();
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