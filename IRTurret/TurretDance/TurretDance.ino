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

// Map a ration to a range. For example, .5 shoudl fall in the middle of a range if the input can be 0-1
double mapDouble( double x, double in_min, double in_max, double out_min, double out_max )
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Dance move for rotating roll and yaw servos
// You can assign a duration and speed
// Duration is milliseconds
// Speed is from 0-1
struct DanceSpeedMove
{
  unsigned long duration;
  double speed;
  bool started;

  DanceSpeedMove( unsigned long dur, double spd = 0.0 )
    : duration( dur ), speed( spd ), started( false )
  {
    speed = max( -1, min( speed, 1 ) );
  }
};

// Dance move for rotating pitch servo
// You can assign a target angle and speed
// Target angle (degrees) is the angle you want to end up at
// Speed degrees/sec
struct DanceAngleMove
{
  int targetAngle;
  double speed;
  bool started;

  DanceAngleMove( int targetAng, int spd )
    : targetAngle( targetAng ), speed( spd )
  {

  }
};

 // Controller to define properties for a servo that lets you set the speed
 // and rotates 360 degrees. This is for the roll and yaw servos.
 // pin: Arduino pin for servo
 // zeroSpd: Speed that is used to keep servo stationary
 // minSpdRange: Minimum speed needed to get servo moving. You may need to experient for your own values
 // maxSpdRange: Maximum speed needed to get servo moving. You may need to experient for your own values
 // moveArray: Array of dance moves to perform
 // moveCount: Number of dance moves in moveArray
class ServoSpeedController
{
public:
  Servo servo;
  DanceSpeedMove* moves;
  int numMoves;
  int currentMoveIndex = 0;
  unsigned long startMoveTime;
  unsigned long lastTime;
  double currentPosition;
  int zeroSpeed;
  int minSpeedRange;
  int maxSpeedRange;

  ServoSpeedController( int pin, int zeroSpd, int minSpdRange, int maxSpdRange, DanceSpeedMove moveArray[], int moveCount )
    : zeroSpeed( zeroSpd ), minSpeedRange( minSpdRange ), maxSpeedRange( maxSpdRange ), moves( moveArray ), numMoves( moveCount )
  {
    servo.attach( pin );
    moveTo( zeroSpd );
    lastTime = millis();
    startMoveTime = lastTime;
  }

  void moveTo( double position )
  {
    servo.write( position );
    currentPosition = position;
  }

  void update()
  {
    if ( currentMoveIndex >= numMoves )
    {
      return;
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
        speed = mapDouble( move.speed, 0, 1, zeroSpeed + minSpeedRange, zeroSpeed + maxSpeedRange );
      }
      else if ( move.speed < 0 )
      {
        speed = mapDouble( move.speed, -1, 0, zeroSpeed - maxSpeedRange, zeroSpeed - minSpeedRange );
      }

      moveTo( speed );
    }

    if ( animTimeElapsed >= move.duration )
    {
      currentMoveIndex++;
      startMoveTime = millis();
      moveTo( zeroSpeed );
    }

    lastTime = currentTime;
  }
};

// Controller to define properties for a servo that lets you set an angle.
// This is for the pitch servo.
// pin: Arduino pin for servo
// minAng: Minimum angle allowed. Prevents rotating too much in one direction.
// maxAng: Maximum angle allowed. Prevents rotating too much in one direction.
// maxSpde: Maximum degrees/sec movement allowed
// moveArray: Array of dance moves to perform
// moveCount: Number of dance moves in moveArray
class ServoAngleController
{
public:
  Servo servo;
  DanceAngleMove* moves;
  int numMoves;
  int currentMoveIndex = 0;
  unsigned long lastTime;
  double currentPosition;
  int maxSpeed;
  int minAngle;
  int maxAngle;

  ServoAngleController( int pin, int minAng, int maxAng, int maxSpd, DanceAngleMove moveArray[], int moveCount )
    : minAngle( minAng ), maxAngle( maxAng ), maxSpeed( maxSpd ), moves( moveArray ), numMoves( moveCount )
  {
    servo.attach( pin );
    lastTime = millis();
    moveTo( minAngle + (double( maxAngle - minAngle ) / 2.0) );
  }

  void moveTo( double position )
  {
    position = max( minAngle, min( maxAngle, position ) );
    servo.write( position );
    currentPosition = position;
  }

  void update()
  {
    if ( currentMoveIndex >= numMoves )
    {
      return;
    }

    unsigned long currentTime = millis();
    DanceAngleMove& move = moves[currentMoveIndex];
    unsigned long timeElapsed = currentTime - lastTime;
    double secsElapsed = (double)timeElapsed / 1000.0;
    double degreesToMove = move.speed * secsElapsed;
    degreesToMove = min( maxSpeed, degreesToMove );

    auto targetAngle = max( minAngle, min( maxAngle, move.targetAngle ) );
    auto direction = (targetAngle - currentPosition) > 0 ? 1 : -1;
    auto newPosition = currentPosition + (degreesToMove * direction);

    if ( (direction > 0 && newPosition > targetAngle) ||
      (direction < 0 && newPosition < targetAngle) )
    {
      newPosition = targetAngle;
      currentMoveIndex++;
    }

    moveTo( newPosition );

    lastTime = currentTime;
  }
};

ServoSpeedController* _rollServo;
ServoSpeedController* _yawServo;
ServoAngleController* _pitchServo;
bool playing = false;

void setup()
{
  Serial.begin( 115200 );

  DanceSpeedMove rollMoves[] =
  {
    DanceSpeedMove( 5000, .2 ),
    DanceSpeedMove( 5000, -.2 ),
    DanceSpeedMove( 5000, .2 ),
    DanceSpeedMove( 5000, -.2 ),
  };

  DanceSpeedMove yawMoves[] =
  {
    DanceSpeedMove( 5000, .2 ),
    DanceSpeedMove( 5000, -.2 ),
    DanceSpeedMove( 5000, .2 ),
    DanceSpeedMove( 5000, -.2 ),
  };

  DanceAngleMove angleMoves[] =
  {
      DanceAngleMove( 150, 50 ),
      DanceAngleMove( 35, 50 ),
      DanceAngleMove( 150, 50 ),
      DanceAngleMove( 35, 50 ),
      DanceAngleMove( 150, 50 ),
      DanceAngleMove( 95, 50 ),
  };

  _rollServo = new ServoSpeedController( 12, 90, 45, 90, rollMoves, sizeof( rollMoves ) / sizeof( DanceSpeedMove ) );
  _yawServo = new ServoSpeedController( 10, 90, 45, 90, yawMoves, sizeof( yawMoves ) / sizeof( DanceSpeedMove ) );
  _pitchServo = new ServoAngleController( 11, 35, 170, 5, angleMoves, sizeof( angleMoves ) / sizeof( DanceAngleMove ) );

  IrReceiver.begin( 9, ENABLE_LED_FEEDBACK );
}

void loop()
{
  if ( IrReceiver.decode() )
  {
    IrReceiver.resume();

    switch ( IrReceiver.decodedIRData.command )
    {
      case cmd1:
      {
        playing = true;
        break;
      }
    }
  }

  if ( playing )
  {
    _rollServo->update();
    _yawServo->update();
    _pitchServo->update();
  }

  delay( 10 );
}