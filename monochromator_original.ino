///////////////////////////////////////////////////////////////////////////////////////////
//CONSTANTS AND VARIABLES
///////////////////////////////////////////////////////////////////////////////////////////

// Define pins
const int dirPin = 8;
const int stepPin = 9;
const int enablePin = 10; // Optional
const int shutterPin = 7;
const int photodiodeS1Pin = A0;
const int photodiodeS2Pin = A1;
const int ledS1Pin = 3;
const int ledS2Pin = 5;

int shutterMode = 0;
unsigned long openTime = 1000; // Default open time in milliseconds
unsigned long closeTime = 1000; // Default close time in milliseconds
unsigned long previousMillis = 0;
bool shutterState = false; // false: closed, true: open

const unsigned long maxOpenTime = 60000; // Maximum open time in milliseconds (1 minute)
const unsigned long maxCloseTime = 60000; // Maximum close time in milliseconds (1 minute)

int stepsPerRevolution = 200; // Change as per your stepper motor
int currentStep = 0; // Track the current step position
int currentWavelength = 400; // Assume initial wavelength is 400nm
const int maxSteps = 10000; // Define maximum allowable steps to the right (adjust based on your hardware)

bool stepperEnabled = true; // Track the stepper motor driver state
unsigned long lastDisableTime = 0; // Variable to track the last time the stepper motor was disabled

int stepsPerWavelength = 0;

const int numCalibrationPoints = 7; // Number of calibration points
const int calibrationWavelengths[numCalibrationPoints] = {400, 450, 500, 550, 600, 650, 700};
int calibrationSteps[numCalibrationPoints] = {0}; // Array to store the step positions for calibration points

bool emergencyStop = false; // Variable to track emergency stop state

/////////////////////////////////////////////////////////////////////////////////////////////////
//SETUP
///////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Set pin modes
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(enablePin, OUTPUT); // Optional
  pinMode(shutterPin, OUTPUT);
  pinMode(photodiodeS1Pin, INPUT);
  pinMode(photodiodeS2Pin, INPUT);
  pinMode(ledS1Pin, OUTPUT);
  pinMode(ledS2Pin, OUTPUT); 

  // Enable stepper motor driver
  digitalWrite(enablePin, LOW); // LOW to enable

  // Initially turn off the LEDs
  digitalWrite(ledS1Pin, HIGH);
  digitalWrite(ledS2Pin, HIGH);

  // Home the motor
  homeStepperMotor();
}

///////////////////////////////////////////////////////////////////////////////////////////
//CALIBRATION
///////////////////////////////////////////////////////////////////////////////////////////

void hybridCalibration() {
  Serial.println("Starting hybrid calibration...");

  homeStepperMotor();

  int initialStep = currentStep;
  moveOneWavelength(true);

  int finalStep = currentStep;
  stepsPerWavelength = finalStep - initialStep;

  if (stepsPerWavelength > 0) {
    Serial.print("Steps per wavelength determined: ");
    Serial.println(stepsPerWavelength);
  } else {
    Serial.println("Error: Calibration failed. The steps per wavelength are not properly initialized. Please try again.");
  }

  // Move back to initial position
  homeStepperMotor();
  currentWavelength = 400;

  Serial.println("Hybrid calibration completed.");
}

void moveOneWavelength(bool forward) {
  int direction = forward ? HIGH : LOW;
  digitalWrite(dirPin, direction);
  int steps = 0;

  while (true) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1000); // Adjust delay as needed
    digitalWrite(stepPin, LOW);
    delayMicroseconds(1000); // Adjust delay as needed
    steps++;

    // Check if S1 has completed a rotation
    if (detectEncoderRotation() || emergencyStop) {
      currentStep += (forward ? steps : -steps);
      break;
    }
  }
}

bool detectEncoderRotation() {
  static bool lastState = LOW;
  bool currentState = analogRead(photodiodeS1Pin);

  if (currentState == HIGH && lastState == LOW) {
    lastState = HIGH;
    return true; // Detected a complete rotation
  }

  lastState = currentState;
  return false;
}

void recalibrateMonochromator() {
  Serial.println("Starting recalibration...");

  for (int i = 0; i < numCalibrationPoints; i++) {
    moveToWavelengthStandard(calibrationWavelengths[i]);
    
    Serial.print("Measure the output wavelength for ");
    Serial.print(calibrationWavelengths[i]);
    Serial.println(" nm and enter the measured value:");

    while (Serial.available() == 0) {
      // Wait for user input
    }
    
    int measuredWavelength = Serial.parseInt();
    int discrepancy = measuredWavelength - calibrationWavelengths[i];
    
    calibrationSteps[i] = currentStep + (discrepancy * stepsPerWavelength);

    Serial.print("Calibration point ");
    Serial.print(calibrationWavelengths[i]);
    Serial.print(" nm set to step position ");
    Serial.println(calibrationSteps[i]);
  }

  Serial.println("Recalibration completed.");
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////

void shutter() {
  if (shutterMode == 1) {
    unsigned long currentMillis = millis();
    if (shutterState && (currentMillis - previousMillis >= openTime)) {
      digitalWrite(shutterPin, LOW); // Close shutter
      shutterState = false;
      previousMillis = currentMillis;
      Serial.println("Shutter closed.");
    } else if (!shutterState && (currentMillis - previousMillis >= closeTime)) {
      digitalWrite(shutterPin, HIGH); // Open shutter
      shutterState = true;
      previousMillis = currentMillis;
      Serial.println("Shutter opened.");
    }
  }
}

void homeStepperMotor() {
  while ((analogRead(photodiodeS2Pin) == LOW || analogRead(photodiodeS1Pin) == HIGH) && !emergencyStop) {
    // Rotate stepper motor until S2 is blocked and S1 receives signal
    stepMotor(-1); // Move stepper motor one step backward
  }
  currentStep = 0; // Set home position
  currentWavelength = 400; // Assume the home position corresponds to 400nm
  Serial.println("Homed to initial position.");
}

void stepMotor(int stepCount) {
  int direction = stepCount > 0 ? HIGH : LOW;
  digitalWrite(dirPin, direction);

  stepCount = abs(stepCount);
  for (int i = 0; i < stepCount && !emergencyStop; i++) {
    if ((currentStep >= maxSteps && direction == HIGH) || (currentStep <= 0 && direction == LOW)) {
      Serial.println("Error: Attempt to move beyond allowed range.");
      break;
    }

    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1000); // Adjust delay as needed
    digitalWrite(stepPin, LOW);
    delayMicroseconds(1000); // Adjust delay as needed
    currentStep += direction == HIGH ? 1 : -1;
  }
}

void moveToWavelengthStandard(int desiredWavelength) {
  if (stepsPerWavelength == 0) {
    Serial.println("Error: Calibration required. Perform initial calibration first.");
    return;
  }

  if (desiredWavelength < 400 || desiredWavelength > (400 + maxSteps)) {
    Serial.println("Error: Wavelength out of range.");
    return;
  }

  int stepsToMove = (desiredWavelength - currentWavelength) * stepsPerWavelength;

  if (stepsToMove > 0) {
    stepMotor(stepsToMove);
  } else {
    stepMotor(stepsToMove);
  }

  currentWavelength = desiredWavelength;
  Serial.print("Moved to wavelength: ");
  Serial.println(desiredWavelength);
}

void enableStepper() {
  if (!stepperEnabled) {
    digitalWrite(enablePin, LOW); // LOW to enable
    stepperEnabled = true;
    Serial.println("Stepper motor enabled.");
  }
}

void disableStepper() {
  if (stepperEnabled) {
    digitalWrite(enablePin, HIGH); // HIGH to disable
    stepperEnabled = false;
    Serial.println("Stepper motor disabled.");
  }
}

void returnToHome() {
  if (currentStep > 0) {
    stepMotor(-currentStep); // Move back to initial position
  }
  Serial.println("Returned to home position.");
}

void readPhotodiodes() {
  // Read photodiode values
  int photodiodeS1Value = analogRead(photodiodeS1Pin);
  int photodiodeS2Value = analogRead(photodiodeS2Pin);
}

void controlLEDs(char command) {
  if (command == 'L') {
    digitalWrite(ledS1Pin, HIGH); // Turn on LED S1
    Serial.println("LED S1 turned on.");
  } else if (command == 'l') {
    digitalWrite(ledS1Pin, LOW); // Turn off LED S1
    Serial.println("LED S1 turned off.");
  } else if (command == 'M') {
    digitalWrite(ledS2Pin, HIGH); // Turn on LED S2
    Serial.println("LED S2 turned on.");
  } else if (command == 'm') {
    digitalWrite(ledS2Pin, LOW); // Turn off LED S2
    Serial.println("LED S2 turned off.");
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//LOOP
///////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  // Check if data is available on the serial port
  if (Serial.available() > 0) {
    // Read the incoming byte
    char command = Serial.read();

    int photodiodeS1Value = analogRead(A0);
    int photodiodeS2Value = analogRead(A1);
    Serial.print("Photodiode S1: ");
    Serial.println(photodiodeS1Value);
    Serial.print("Photodiode S2: ");
    Serial.println(photodiodeS2Value);
    delay(1000);

    if (command == 13) { // Enter key (ASCII code 13) for emergency stop
      emergencyStop = true;
      Serial.println("Emergency stop triggered!");
      disableStepper();
    } else if (command == '1') {
      shutterMode = 0; // Always open
    } else if (command == '2') {
      shutterMode = 1; // Customized pattern
    } else if (command == 'o') {
      // Receive open time
      unsigned long receivedOpenTime = Serial.parseInt() * 1000; // Convert seconds to milliseconds
      if (receivedOpenTime <= maxOpenTime) {
        openTime = receivedOpenTime;
      } else {
        Serial.println("Error: Open time exceeds maximum limit.");
      }
    } else if (command == 'c') {
      // Receive close time
      unsigned long receivedCloseTime = Serial.parseInt() * 1000; // Convert seconds to milliseconds
      if (receivedCloseTime <= maxCloseTime) {
        closeTime = receivedCloseTime;
      } else {
        Serial.println("Error: Close time exceeds maximum limit.");
      }
    } else if (command == 'o_manual') {
      digitalWrite(shutterPin, HIGH); // Manually open shutter
      Serial.println("Shutter manually opened.");
    } else if (command == 'c_manual') {
      digitalWrite(shutterPin, LOW); // Manually close shutter
      Serial.println("Shutter manually closed.");
    }else if (command == 'w') {
      int desiredWavelength = Serial.parseInt();
      moveToWavelengthStandard(desiredWavelength);
    }else if (command == 'h') {
      returnToHome(); // Return to home position
    }else if (command == 'calibrate_initial') {
      hybridCalibration(); // Perform initial hybrid calibration
    } else if (command == 'recalibrate') {
      recalibrateMonochromator(); // Perform recalibration
    }else {
      controlLEDs(command); // Control LEDs
    }
  }

  // Continuously call the shutter function if mode is 1
  if (shutterMode == 1) {
    shutter();
  } else if (shutterMode == 0) {
    digitalWrite(shutterPin, HIGH); // Hold shutter open always
  }

  // Disable stepper motor driver if it has been idle for a while
  if (millis() - lastDisableTime > 1000 && stepperEnabled) {
    disableStepper();
  }

  lastDisableTime = millis(); // Reset idle timer
}

/////////////////////////////////////////////////////////////////////////////////////////////


