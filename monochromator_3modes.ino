#include <AccelStepper.h>

const int dirPin = 8;
const int stepPin = 9;
const int enablePin = 10; // Optional
const int photodiodeS1Pin = A0;
const int photodiodeS2Pin = A5;
const int ledS1Pin = 3;
const int ledS2Pin = 5;

int stepsPerRevolution = 200; // Change as per your stepper motor
int currentStep = 0; // Track the current step position
const int maxSteps = 10000; // Define maximum allowable steps to the right (adjust based on your hardware)

int stepsPerWavelength = 0;

#define motorInterfaceType 1

// Creates an instance
AccelStepper myStepper(motorInterfaceType, stepPin, dirPin);

bool emergencyStop = false; // Variable to track emergency stop state
int gratingMode = 0; // 0: VIS, 1: IR, 2: Both with switch at 650 nm

enum State { SELECT_MODE, INPUT_WAVELENGTH };
State currentState = SELECT_MODE;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Stepper motor setup
  myStepper.setMaxSpeed(500);
  myStepper.setAcceleration(50);
      
  // Set pin modes
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(enablePin, OUTPUT); // Optional
  pinMode(photodiodeS1Pin, INPUT);
  pinMode(photodiodeS2Pin, INPUT);
  pinMode(ledS1Pin, OUTPUT);
  pinMode(ledS2Pin, OUTPUT); 

  // Enable stepper motor driver
  digitalWrite(enablePin, LOW); // LOW to enable

  // Home the motor
  homeStepperMotor();
  myStepper.setCurrentPosition(0);
  myStepper.stop();

  // Ask user for the grating mode
  selectGratingMode();
}

void homeStepperMotor() {
  myStepper.move(50);
  while (myStepper.distanceToGo() != 0) {
    myStepper.run();
  }

  while (analogRead(photodiodeS2Pin) < 300 && !emergencyStop) {
    // Rotate stepper motor until S2 is blocked and S1 receives signal
    myStepper.move(-1000000);
    myStepper.run();
  }
  Serial.print("Photodiode S2: ");
  Serial.println(analogRead(photodiodeS2Pin));

  currentStep = 0; // Set home position
  Serial.println("Homed to initial position.");
}

void selectGratingMode() {
  Serial.println("Select the grating mode:");
  Serial.println("0: VIS Grating only (350-1000 nm)");
  Serial.println("1: IR Grating only (587+ nm)");
  Serial.println("2: Switch between VIS and IR Gratings at 650 nm");

  currentState = SELECT_MODE; // Set the state to mode selection
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read the input from Serial Monitor
    input.trim(); // Trim any leading/trailing whitespace

    if (currentState == SELECT_MODE) {
      int modeSelection = input.toInt();
      if (modeSelection == 0 || modeSelection == 1 || modeSelection == 2) {
        gratingMode = modeSelection;
        if (gratingMode == 0) {
          Serial.println("VIS Grating mode selected.");
        } else if (gratingMode == 1) {
          Serial.println("IR Grating mode selected.");
        } else if (gratingMode == 2) {
          Serial.println("Switching mode between VIS and IR Gratings at 650 nm.");
        }
        currentState = INPUT_WAVELENGTH; // Move to the next state
        Serial.println("Enter the target wavelength:");
      } else {
        Serial.println("Invalid selection. Please enter 0, 1, or 2:");
      }
    } else if (currentState == INPUT_WAVELENGTH) {
      if (input == "-1") {
        selectGratingMode(); // Switch back to grating mode selection
        return;
      }

      float targetWavelength = input.toFloat(); // Convert the input to a float
      int targetSteps;
      bool validInput = true;

      // Check if the input wavelength is within the valid range for the selected mode
      if (gratingMode == 0) { // VIS Grating only
        if (targetWavelength < 350 || targetWavelength > 1000) {
          Serial.println("Invalid wavelength for VIS Grating. Please enter a wavelength between 350 and 1000 nm or '-1' to switch mode:");
          validInput = false;
        } else {
          targetSteps = (targetWavelength + 389.2407) / 1.1127;
        }
      } else if (gratingMode == 1) { // IR Grating only
        if (targetWavelength < 587 || targetWavelength > 2000) {
          Serial.println("Invalid wavelength for IR Grating. Please enter a wavelength between 587 and 2000 nm or '-1' to switch mode:");
          validInput = false;
        } else {
          targetSteps = (targetWavelength + 4715.4390) / 0.5099;
        }
      } else { // Switching mode
        if (targetWavelength < 350 || targetWavelength > 2000) {
          Serial.println("Invalid wavelength for switching mode. Please enter a wavelength between 350 and 2000 nm or '-1' to switch mode:");
          validInput = false;
        } else {
          if (targetWavelength < 650) {
            targetSteps = (targetWavelength + 389.2407) / 1.1127; // VIS range
          } else {
            targetSteps = (targetWavelength + 4715.4390) / 0.5099; // IR range
          }
        }
      }

      if (validInput) {
        myStepper.moveTo(targetSteps);
        while (myStepper.distanceToGo() != 0) {
          if (Serial.available() > 0) {
            String stopCommand = Serial.readStringUntil('\n');
            stopCommand.trim();
            if (stopCommand == "") { // If enter is pressed
              Serial.println("Emergency stop initiated.");
              myStepper.stop(); // Stop the motor
              Serial.println("Enter new wavelength or '-1' to switch mode:");
              break;
            }
          }
          myStepper.run();
        }
         if (myStepper.distanceToGo() == 0) {
           Serial.print("Target wavelength reached. Current wavelength is: ");
           Serial.println(targetWavelength);
          Serial.println("Enter new wavelength or '-1' to switch mode:");
         }
       } 
    }
  }
}
