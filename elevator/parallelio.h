#pragma once
#include "Bounce2.h"

//--------------------------------------------------------------------------------------
// Shift register based inputs & outputs
// Extend bounce library to support parallel to serial shift register

class ParallelInputs {
  public:
    uint8_t pin_data, pin_clock, pin_latch;
    uint8_t values;

  public:
    ParallelInputs(uint8_t pin_data, uint8_t pin_clock, uint8_t pin_latch) {
      this->pin_data = pin_data;
      this->pin_clock = pin_clock;
      this->pin_latch = pin_latch;

      pinMode(pin_data, INPUT);
      pinMode(pin_clock, OUTPUT);
      pinMode(pin_latch, OUTPUT);
      
      this->values = 0;
    }

    void update() {
      digitalWrite(this->pin_latch, LOW);
      delay(10);
      digitalWrite(this->pin_latch, HIGH);
      delay(1);
      this->values = shiftIn(pin_data, pin_clock, MSBFIRST);
    }

    bool read(uint8_t pin) {
      return (this->values & (1 << pin));
    }
};

class ParallelOutputs {
  public:
    uint8_t pin_data, pin_clock, pin_latch;
    uint8_t values;

  public:
    ParallelOutputs(uint8_t pin_data, uint8_t pin_clock, uint8_t pin_latch) {
      this->pin_data = pin_data;
      this->pin_clock = pin_clock;
      this->pin_latch = pin_latch;

      pinMode(pin_data, OUTPUT);
      pinMode(pin_clock, OUTPUT);
      pinMode(pin_latch, OUTPUT);

      this->values = 0;
    }

    void update() {
      digitalWrite(this->pin_latch, LOW);
      shiftOut(this->pin_data, this->pin_clock, MSBFIRST, this->values & 0xFF);
      digitalWrite(this->pin_latch, HIGH);
    }

    void write(uint8_t pin, uint8_t val) {
      bitWrite(this->values, pin, val != 0);
    }
};

const uint8_t SEG_A =  0b01000000;
const uint8_t SEG_B =  0b10000000;
const uint8_t SEG_C =  0b00000001;
const uint8_t SEG_D =  0b00000010;
const uint8_t SEG_E =  0b00000100;
const uint8_t SEG_F =  0b00010000;
const uint8_t SEG_G =  0b00001000;
const uint8_t SEG_DP = 0b00100000;

class SSeg {
  public:
    uint8_t pin_data, pin_clock, pin_latch;
    uint8_t values[2];

  public:
    SSeg(uint8_t pin_data, uint8_t pin_clock, uint8_t pin_latch) {
      this->pin_data = pin_data;
      this->pin_clock = pin_clock;
      this->pin_latch = pin_latch;

      pinMode(pin_data, OUTPUT);
      pinMode(pin_clock, OUTPUT);
      pinMode(pin_latch, OUTPUT);

      this->values[0] = 0;
      this->values[1] = 0;
    }

    void update() {
      digitalWrite(this->pin_latch, LOW);
      shiftOut(this->pin_data, this->pin_clock, MSBFIRST, this->values[0]);
      shiftOut(this->pin_data, this->pin_clock, MSBFIRST, this->values[1]);
      digitalWrite(this->pin_latch, HIGH);
    }

    static uint8_t digit(uint8_t number) {
      switch (number) {
        case 0: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F;
        case 1: return SEG_B | SEG_C;
        case 2: return SEG_A | SEG_B | SEG_G | SEG_E | SEG_D;
        case 3: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_G;
        case 4: return SEG_F | SEG_G | SEG_B | SEG_C;
        case 5: return SEG_A | SEG_F | SEG_G | SEG_C | SEG_D;
        case 6: return SEG_A | SEG_F | SEG_G | SEG_E | SEG_D | SEG_C;
        case 7: return SEG_A | SEG_B | SEG_C;
        case 8: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
        case 9: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G;
        default: return SEG_DP;
      }
    }
};


class ParallelBounce : public Bounce {
    ParallelInputs& pi;

  public:
    ParallelBounce(ParallelInputs& pi, uint8_t pin) : pi(pi)
    {
      this->attach(pin);
    }

    virtual bool readCurrentState() {
      return this->pi.read(this->pin);
    }

    virtual void setPinMode(int pin, int mode) {
      // no-op
      return;
    }
};

class OutputPin
{
  public:
    uint8_t pin;
    OutputPin(uint8_t pin) {
      this->pin = pin;
      setPinMode(pin, OUTPUT);
    }

    virtual void write(int8_t value) {
      digitalWrite(pin, value);
    }

  protected:
    virtual void setPinMode(int pin, int mode) {
      pinMode(pin, mode);
    }

    
};

class ParallelOutputPin : public OutputPin {
  ParallelOutputs& po;
  
  public:
    ParallelOutputPin(ParallelOutputs& po, uint8_t pin) : po(po), OutputPin(pin)
    {
    }

    virtual void setPinMode(int pin, int mode) 
    {
      // no-op
      return;
    }

    virtual void write(int8_t value) {
      po.write(pin, value);
    }
};

//--------------------------------------------------------------------------------------

class FancyButton
{ 
  public:
    ParallelBounce input;
    ParallelOutputPin out;

    bool held_called;
    bool on;
    bool on_on_press;
   
    FancyButton(ParallelBounce input, ParallelOutputPin out) : input(input), out(out)
    {
      on_on_press = true;
      held_called = true;
      on = false;
    }

    void update(void (*pressed)() = NULL, void (*held)() = NULL) {
      input.update();
      if (input.rose() && !held_called && pressed) {
        pressed();
      }
      if (input.read() == LOW && !held_called && input.duration() >= 1000) {
         held_called = true;
         if (held) {
          held();
         }
      }
      if (input.rose()) {
        held_called = false;
      }

      bool is_on = on || (on_on_press && input.read() == LOW);
      out.write(is_on);
    }
};
