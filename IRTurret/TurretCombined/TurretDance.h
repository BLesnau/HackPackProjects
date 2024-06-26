#include <Arduino.h>
#include <Servo.h>
#include "PinDefinitionsAndMore.h"
#include "Utils.h"
#include "BaseProgram.h"
#include "DanceMove.h"
#include "ServoController.h"

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

class TurretDanceProgram : public BaseProgram
{
public:
   void Setup() override
   {
      _rollServo = new ServoSpeedController( ROLL_SERVO_PIN, ROLL_ZERO_SPEED, ROLL_MIN_SPEED, ROLL_MAX_SPEED );
      _yawServo = new ServoSpeedController( YAW_SERVO_PIN, YAW_ZERO_SPEED, YAW_MIN_SPEED, YAW_MAX_SPEED );
      _pitchServo = new ServoAngleController( PITCH_SERVO_PIN, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE, PITCH_MAX_SPEED );

      // SetDanceRoutine1();
      // _playing = true;
   }

   void Loop( uint16_t cmd ) override
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

   bool CanShutdown() override
   {
      return !_playing;
   }

   void Shutdown() override
   {
      delete _rollServo;
      delete _yawServo;
      delete _pitchServo;
   }

private:
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
         DanceSpeedMove( 40 * du ),
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
      };

      _rollServo->SetDanceMoves( rollMoves, sizeof( rollMoves ) / sizeof( DanceSpeedMove ) );
      _yawServo->SetDanceMoves( yawMoves, sizeof( yawMoves ) / sizeof( DanceSpeedMove ) );
      _pitchServo->SetDanceMoves( pitchMoves, sizeof( pitchMoves ) / sizeof( DanceAngleMove ) );
   }
};