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