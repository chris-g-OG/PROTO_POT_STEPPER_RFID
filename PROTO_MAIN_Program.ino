
#include <pt.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Stepper.h>

   // Motor Driver
#define motorPin1  44      // IN1 on the ULN2003 driver
#define motorPin2  45      // IN2 on the ULN2003 driver
#define motorPin3  46     // IN3 on the ULN2003 driver
#define motorPin4  47     // IN4 on the ULN2003 driver
    // Pot analog pin
#define sensorPIN A0
 
 // LED pins
#define LED1 5 // green
#define LED2 6 // red
#define LED3 7 // Any color

   // RFID SPI pins
#define SS_PIN 53
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);   

static struct pt pt1, pt2;

const int stepsPerRevolution = 32;

Stepper myStepper(stepsPerRevolution, motorPin1, motorPin3, motorPin2, motorPin4);

volatile int currentStep = 0; 

//volatile unsigned long distance = 0; // Sonar distance

String content = ""; // Card ID content

bool auth_value = false;

unsigned int time2read; 

// Motor rotating protothread function
static int pot2step(struct pt *pt) {
  
  PT_BEGIN(pt);
  while(1) {
    
      PT_YIELD_UNTIL(pt, auth_value == true);
    
      int newStep = map(analogRead(sensorPIN), 0, 1023, 0, 2047); // Mapped step Read Pot
      int changeStep = newStep - currentStep;  
    
      if(newStep != currentStep && abs(changeStep) > 4 ) {  // Reduse Motor noise 4 step gap
        changeStep = constrain(changeStep, -32, 32); // Contrain into -32 - +32 the value
        myStepper.step(changeStep);
        currentStep += changeStep;
      }    
    
    PT_YIELD(pt); // Let the other threads work
  
  }
  PT_END(pt);

}
static int proto_RFID522(struct pt *pt) {
  
  PT_BEGIN(pt);
  while(1) {
            Serial.println("Approximate your card to the reader...");
            Serial.println("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
            
            // Locked state 
            digitalWrite(LED1, LOW);
            digitalWrite(LED2, LOW);
            digitalWrite(LED3, HIGH);
 
            content = "";
            auth_value = false;
          
            if ( !mfrc522.PICC_IsNewCardPresent()) 
                return 0;      

            if ( !mfrc522.PICC_ReadCardSerial() )
                return 0;
           
            //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

            Serial.print("U-ID tag :");   
            for (byte i = 0; i < mfrc522.uid.size; i++) 
            {
                content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
                content.concat(String(mfrc522.uid.uidByte[i], HEX));  
            }
            Serial.println();
            Serial.print("Message : ");
            content.toUpperCase();
            
            // Access granted here
            if (content.substring(1) == "42 1E A0 49") // Access UID number
            {
                auth_value = true;
                Serial.println("Authorised access");
                Serial.println();
                digitalWrite(LED1, HIGH);
                digitalWrite(LED2, LOW);
                digitalWrite(LED3, LOW);    
            }                     
            else { 
                auth_value = false;
                Serial.println(" Access denied ");
                digitalWrite(LED1, LOW);
                digitalWrite(LED2, HIGH);
                digitalWrite(LED3, LOW);
            }

            //Serial.println(content.substring(1));
            time2read = millis() + 2000; 
            PT_YIELD_UNTIL(pt, millis() >= time2read);
            //PT_YIELD_TIME_msec(2000); // 2 Seconds           
            //PT_YIELD(pt);
        }

  PT_END(pt);
}

void setup() 
{
  //Initialize the PThreads
  PT_INIT(&pt1);
  PT_INIT(&pt2);
  
  Serial.begin(9600);  

  SPI.begin();      
  mfrc522.PCD_Init();   
  
  myStepper.setSpeed(700);
}
void loop() 
{
    pot2step(&pt1);
    proto_RFID522(&pt2);
//  PT_SCHEDULE(proto_RFID522(&pt1));
}