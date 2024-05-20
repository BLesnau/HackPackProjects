#include <Servo.h>

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
            speed = map( move.speed, -100, 0, zeroSpeed - maxSpeed, zeroSpeed - minSpeed );
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