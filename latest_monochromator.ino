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
int currentWavelength = 400; // Assume initial wavelength is 400nm
const int maxSteps = 10000; // Define maximum allowable steps to the right (adjust based on your hardware)

int stepsPerWavelength = 0;

#define motorInterfaceType 1

// Creates an instance
AccelStepper myStepper(motorInterfaceType, stepPin, dirPin);

bool emergencyStop = false; // Variable to track emergency stop state

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
}

void homeStepperMotor() {
  myStepper.move(50);
  while (myStepper.distanceToGo() != 0) {
    myStepper.run();
  }

  while (analogRead(photodiodeS2Pin) < 300  && !emergencyStop) {
    // Rotate stepper motor until S2 is blocked and S1 receives signal
    myStepper.move(-1000000);
    myStepper.run();
  }
  Serial.print("Photodiode S2: ");
  Serial.println(analogRead(photodiodeS2Pin));
  
  currentStep = 0; // Set home position
  currentWavelength = 400; // Assume the home position corresponds to 400nm
  Serial.println("Homed to initial position.");
  Serial.println("Enter the target wavelength:");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read the input from Serial Monitor
    float targetWavelength = input.toFloat(); // Convert the input to a float
    int targetSteps;

    // Convert the wavelength to steps using the linear equation: steps = (wavelength + 403.58) / 1.13266
    if (targetWavelength < 650) {
      // Convert the wavelength to steps using the VIS grating equation
      targetSteps = (targetWavelength + 403.58) / 1.13266;
    } else {
      // Switch to the second grating and use the corresponding equation
      targetSteps = (targetWavelength + 4715.4390) / 0.5099;
    }
    //Serial.print("Equivalent steps: ");
    //Serial.println(targetSteps);

    myStepper.moveTo(targetSteps);
    while (myStepper.distanceToGo() != 0) {
      myStepper.run();
    }
    Serial.print("Target wavelength reached. Current wavelength is: ");
    Serial.println(targetWavelength);
  }
}
