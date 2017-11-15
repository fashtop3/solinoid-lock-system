#include <LiquidCrystal.h>
#include <util/delay.h>
#include <Adafruit_Fingerprint.h>

//#define F_CPU 16000000L

// Modify these at your leisure.
#define ENTER_BUTTON A3
#define ID_BUTTON A4
#define ENROLL_BUTTON A5

// LCD pins declaration
#define rs 7
#define en 8
#define d4 9
#define d5 10
#define d6 11
#define d7 12

#define solenoidPin 13

int8_t k = 0;
volatile char enrolButtonState = HIGH;
char idButtonState = HIGH;
char downButtonState = HIGH;
volatile char enterButtonState = HIGH;
volatile uint8_t id = 0;
volatile uint8_t counter = 0;

uint8_t getFingerprintEnroll( int id);
int getFingerprintIDez();
void initialize();
void enrolButtonClicked();

const byte ledPin = 13;
volatile byte state = HIGH;
volatile int pressedConfidenceLevel = 0;
volatile int releasedConfidenceLevel = 0;
volatile int pressed = 0;
volatile byte trap = false;

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
SoftwareSerial lockSerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint( &lockSerial);


void setup() {

  PCMSK1 |= bit (PCINT13) | bit (PCINT12) | bit (PCINT11);
  PCICR |= (1 << PCIE1);

//  cli();//stop interrupts
  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 7;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
//  sei();//allow interrupts

  pinMode(ENTER_BUTTON, INPUT_PULLUP);
  pinMode(ID_BUTTON, INPUT_PULLUP);
  pinMode(ENROLL_BUTTON, INPUT_PULLUP);

  pinMode(solenoidPin, OUTPUT);

  while (!Serial);

  lcd.begin(16, 2);
  Serial.begin(9600);
  lcd.setCursor(0, 0);

  Serial.println("Loading/Checking Module.");
  initialize();
  // check and verify fingerprintModule

  Serial.println("Waiting for valid finger...");
  delay(1000);
  lcd.clear();
  lcd.print("Locked!!!");
  lcd.setCursor(0, 1);
  lcd.print("Sensor Ready!");
}


void loop() {

  while (1) {
    //  digitalWrite(ledPin, state);

    while (enrolButtonState == HIGH) {
      if (getFingerprintIDez() != -1) {
        Serial.println("Found one");
        if (finger.confidence >= 60 && finger.confidence <= 100) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Found ID #"); lcd.print(finger.fingerID);
          lcd.setCursor(0, 1);
          lcd.print("Confidence "); lcd.print(finger.confidence);
          _delay_ms(5000);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Locked!!!");
          lcd.setCursor(0, 1);
          lcd.print("Sensor Ready!");
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Finger not valid");
        }
      }
      _delay_ms(50);
    }

    getFingerprintIDez();
    _delay_ms(50);

    while (enrolButtonState == LOW) {
      Serial.println("Here");
      while (enterButtonState)
        continue;
      enrolButtonClicked();
      enrolButtonState = HIGH;
      break;
    }
  }
}

//ISR (PCINT1_vect){}


ISR(TIMER1_COMPA_vect) { //change the 0 to 1 for timer1 and 2 for timer2
  if (digitalRead(ENROLL_BUTTON) == LOW) {
    pressedConfidenceLevel++;
    if (pressedConfidenceLevel > 200) {
      if (pressed == 0) {
        //        state = !state;
        enrolButtonState = !enrolButtonState;
        pressed = 1;
        if (!enrolButtonState) {
          //Serial.println("Enroll!");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Enrolling!!!");
        }
        else {
          //Serial.println("Waiting...!");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Waiting...");
        }
      }
      pressedConfidenceLevel = 0;
    }
  }
  else if (digitalRead(ID_BUTTON) == LOW) {
    pressedConfidenceLevel++;
    if (pressedConfidenceLevel > 200) {
      if (pressed == 0) {
        //        state = !state;
        id = counter;
        (counter > 13) ? counter = 0 : counter++;
        //Serial.print("Enrolling ID #");
        //Serial.println(id);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enroll. ID #");
        lcd.print(id);
        pressed = 1;
        //        //Serial.println("Pressed!");
      }
      pressedConfidenceLevel = 0;
    }
  }
  else if (digitalRead(ENTER_BUTTON) == LOW) {
    pressedConfidenceLevel++;
    if (pressedConfidenceLevel > 200) {
      if (pressed == 0) {
        //        state = !state;
        enterButtonState = LOW;
        pressed = 1;
        //Serial.println("Enter Pressed!");
      }
      pressedConfidenceLevel = 0;
    }
  }
  else {
    releasedConfidenceLevel++;
    if (releasedConfidenceLevel > 200) {
      pressed = 0;
      enterButtonState = HIGH;
      //      //Serial.println("Unpressed!");
      releasedConfidenceLevel = 0;
      pressedConfidenceLevel = 0;
    }
  }
}

void enrolButtonClicked()
{
  int enroll_id = id;
  while ( getFingerprintEnroll(enroll_id) == -1 );
  _delay_ms(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting...");
}


void initialize()
{
  // set the data rate for the sensor serial port
  finger.begin(57600);

  uint8_t i = 0;
  while (i < 7) {
    if (i == 0) lcd.print("Loading");
    if (i > 0) {
      lcd.print(".");
      if (finger.verifyPassword()) {
        lcd.clear();
        //Serial.println("Found Sensor!");
        lcd.print("Found Sensor!");
        break; //break out of the loop if sensor is found
      } else {
        lcd.clear();
        //Serial.println("Sensor Error!");
        lcd.print("Sensor Error!");
        while (1); //halt
      }
    }
    if (i == 6) {
      lcd.clear();
      i = 0;
      continue;
    }
    _delay_ms(1000);
    i++;
  }
}

// Enroll fingerprint if matched
uint8_t getFingerprintEnroll( int id) {

  int p = -1;
  //Serial.print("Waiting for valid finger to enroll as #"); //Serial.println(id);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place finger to");
  lcd.setCursor(0, 1);
  lcd.print("enroll as #");
  lcd.print(id);

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        //Serial.println("Image taken");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        //Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        //Serial.println("Communication error");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Communication");
        lcd.setCursor(0, 1);
        lcd.print("Error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        //Serial.println("Imaging error");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Imaging error");
        break;
      default:
        //Serial.println("Unknown error");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image converted");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      //Serial.println("Could not find fingerprint features");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }

  //Serial.println("Remove finger");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  //Serial.print("ID "); //Serial.println(id);
  p = -1;
  //Serial.println("Place same finger again");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Same finger");
  lcd.setCursor(0, 1);
  lcd.print("again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        //Serial.println("Image taken");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        //Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        //Serial.println("Communication error");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Communication");
        lcd.setCursor(0, 1);
        lcd.print("Error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        //Serial.println("Imaging error");
        break;
      default:
        //Serial.println("Unknown error");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Unknown Error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      //Serial.println("Could not find fingerprint features");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  //Serial.print("Creating model for #");  //Serial.println(id);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Creating model");
  lcd.setCursor(0, 1);
  lcd.print(" for #");
  lcd.print(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    //Serial.println("Prints matched!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    //Serial.println("Fingerprints did not match");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fingerprints");
    lcd.setCursor(0, 1);
    lcd.print("did not match");
    return p;
  } else {
    //Serial.println("Unknown error");
    return p;
  }

  //Serial.print("ID "); //Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    //Serial.println("Stored!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Stored!!!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    //Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    //Serial.println("Error writing to flash");
    return p;
  } else {
    //Serial.println("Unknown error");
    return p;
  }
}

// get fingerprint if matched
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      //Serial.println("Imaging error");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      //Serial.println("Could not find fingerprint features");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    //Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    //Serial.println("Did not find a match");
    return p;
  } else {
    //Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); //Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); //Serial.println(finger.confidence);
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  Serial.print("Found ID #"); //Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); //Serial.println(finger.confidence);
  return finger.fingerID;
}
