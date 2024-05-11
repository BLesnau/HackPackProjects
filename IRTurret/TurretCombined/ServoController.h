#include <Servo.h>

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

   // Map a ration to a range. For example, .5 should fall in the middle of a range if the input can be 0-1
   double mapDouble( double x, double in_min, double in_max, double out_min, double out_max )
   {
      return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
   }
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