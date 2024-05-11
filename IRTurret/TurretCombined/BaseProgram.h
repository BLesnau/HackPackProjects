#pragma once

class BaseProgram
{
public:
   virtual void Setup() = 0;
   virtual void Loop( uint16_t cmd ) = 0;
   virtual bool CanShutdown() = 0;
   virtual void Shutdown() = 0;
};