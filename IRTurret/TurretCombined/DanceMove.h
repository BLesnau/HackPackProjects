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