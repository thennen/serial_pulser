#include <Messenger.h>

double serialin;
float duration;
uint16_t prescaler;
uint32_t pulse_length;
uint16_t cycles;
bool direction;

// Instantiate Messenger object with the message function and the default separator (the space character)
Messenger message = Messenger();

// Fire a one-shot pulse with the specified width. 
// Order of operations in calculating m must avoid overflow of the unint16_t.
// TCNT2 starts one count lower than the match value becuase the chip will block any compare on the cycle after setting a TCNT. 
void OSP_SET_AND_FIRE(uint16_t cycles)
{
  uint16_t m = 0xffff - (cycles - 1);
  Serial.print("m = ");
  Serial.print(m);
  Serial.print(", ");
  OCR1B = m;
  TCNT1 = m-2;
}

int set_prescaler(int value)
{
  TCCR1B =  0;              // Halt counter by setting clock select bits to 0 (No clock source)
  TCNT1 = 0;                // Start counting at bottom. 
  OCR1A = 0;                // Set TOP to 0. Counter just keeps reseting back to 0.
                            // We break out of this by manually setting the TCNT higher than 0, in which case it will count all the way up to MAX and then overflow back to 0 and get locked up again.
  TCCR1A = _BV(COM1B0) | _BV(COM1B1) | _BV(WGM10) | _BV(WGM11); // Set on Match, clear on BOTTOM. Mode 15 Fast PWM.
  TCCR1B = _BV(WGM12)| _BV(WGM13);                  // Select Fast PWM mode 15
  // Select prescaler
  switch(value)
  {
    case 1:
      TCCR1B |= _BV(CS10);
      return 1;
    case 8:
      TCCR1B |= _BV(CS11);
      return 8;
    case 64:
      TCCR1B |= _BV(CS10) | _BV(CS11);
      return 64;
    case 256:
      TCCR1B |= _BV(CS12);
      return 256;
    case 1024:
      TCCR1B |= _BV(CS10) | _BV(CS12);
      return 1024;
    default:
      return 0;
  }
}

void messageCompleted() 
{
  
  serialin = message.readDouble();
  
  if(serialin > 0) direction = 1;
  else direction=0;

  // pulse length in clock cycles
  pulse_length = uint32_t(abs(serialin * 1000000000) / 62.5);
  
  if(pulse_length == 0) Serial.print("Does not compute");
  else if (abs(serialin) > 4.194304)
  {
    Serial.print("Cannot pulse longer than 4.194304 seconds");
  }
  else
  {
    // Find necessary prescaler
    if(pulse_length >= 16777216 - 1)
    {
      prescaler = set_prescaler(1024);
    }
    else if(pulse_length >= 4194304 - 1)
    {
      prescaler = set_prescaler(256);
    }
    else if(pulse_length >= 524288 - 1)
    {
      prescaler = set_prescaler(64);
    }
    else if(pulse_length >= 65536 - 1)
    {
      prescaler = set_prescaler(8);
    }
    else
    {
      prescaler = set_prescaler(1);
    }
    cycles = pulse_length / prescaler;
    duration = 62.5 * cycles * prescaler;
    Serial.print("cycles =");
    Serial.print(cycles);
    Serial.print(", ");
    Serial.print("prescaler =");
    Serial.print(prescaler);
    Serial.print(", ");

    //delay(100);
    OSP_SET_AND_FIRE(cycles);
    Serial.print("Pulsing for ");
    Serial.print(duration);
    Serial.print(" ns in direction ");
    Serial.print(direction);
  }
  Serial.println(); // Terminate message
}

void setup()
{
  TCCR1B =  0;              // Halt counter by setting clock select bits to 0 (No clock source)
  TCNT1 = 0;                // Start counting at bottom. 
  OCR1A = 0;                // Set TOP to 0. Counter just keeps reseting back to 0.
                            // We break out of this by manually setting the TCNT higher than 0, in which case it will count all the way up to MAX and then overflow back to 0 and get locked up again.
  
  TCCR1A = _BV(COM1B0) | _BV(COM1B1) | _BV(WGM10) | _BV(WGM11); // Set on Match, clear on BOTTOM. Mode 15 Fast PWM.
  //TCCR1B = _BV(WGM12)| _BV(WGM13) | _BV(CS10);                  // Select Fast PWM mode 15
  //TCCR1B |= _BV(CS10);                                        // Start counter
  
  pinMode(10, OUTPUT);    // Set pin to output (OC1B = GPIO port PB2 = Arduino Digital Pin 10)

  Serial.begin(9600);
  
  message.attach(messageCompleted);
}

void loop()
{
  while(Serial.available()) message.process(Serial.read());
}
