import serial
import serial.tools.list_ports
import time
from tkinter import messagebox
import tkinter as tk
import sys
import os

sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 1)  # Reopen stdout in unbuffered mode

class MonochromatorControl:
    #Create an instance of MonochromatorControl 
    # without immediately needing to connect to the Arduino or the COM.
    def __init__(self, gui, port=None, baud_rate=9600, timeout=1):
        self.ser = None
        self.gui = gui  # Reference to the GUI instance
        self.status_messages = ["First select a COM port."]

        self.arduino_initialized = False  # Flag to track Arduino initialization
        self.motor_homed = False #Flag to track the initial homing of the motor
        self.grating_selected = False #Flag to track if grating mode is selected
        
        # Update the status right after initializing the class
        self.update_status()
        
        if port:
            self.connect_to_monochromator(port, baud_rate, timeout)

    def list_available_ports():
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]


    #Connecting to the COM port, without communicating with the Arduino
    def connect_to_monochromator(self, selected_port, baud_rate=9600, timeout=1):
        if selected_port:
            self.ser = serial.Serial(selected_port, baud_rate, timeout=timeout)
            time.sleep(0.1)  # Give some time for the connection to establish
            self._add_status(f"Connected to {selected_port}, you can now initialize and start your Monochromator.")
        else:
            messagebox.showerror("Error", "Please select a COM port.")

    #Assumes the COM port is connected and prepeares the Arduino to receive commands
    def initialize_arduino(self):
        if not self.ser:
            self._add_status("COM port is not selected.")
            return
         # Read all available lines from the serial buffer
        responses = []  # Initialize the list to store responses
        time.sleep(0.1)  # Allow some time for Arduino to initialize and send data

        while self.ser.in_waiting > 0:  # Check if there are more lines to read
            line = self.ser.readline().decode().strip()
            if line:
                responses.append(line)
    
        # Combine all the responses into one status message
        if responses:
            combined_response = "\n".join(responses)
            self._add_status(combined_response)
    
        self.arduino_initialized = True
    
    
    def home_motor(self):
        if not self.ser:
            self._add_status("COM port is not selected.")
            return
        if not self.arduino_initialized:
            self._add_status("The Arduino is off, please press START.")
            return

        self.ser.write(b'home\n')

        # Wait for immediate response
        immediate_response = ""
        while True:
            if self.ser.in_waiting > 0:
                immediate_response = self.ser.readline().decode().strip()
                self._add_status(immediate_response)
                self.gui.root.update_idletasks()  # Process any pending GUI events
                break

        # Loop to read all incoming lines
        responses = []  # Initialize the list to store responses
        while True:
            while self.ser.in_waiting > 0:  # Check if there are more lines to read
                line = self.ser.readline().decode().strip()
                if line:
                    responses.append(line)
            self.gui.root.update_idletasks()  # Ensure the GUI stays responsive

            if responses:  # Check if we have collected some responses
                combined_response = "\n".join(responses)
                self._add_status(combined_response)
                break  # Exit loop once all responses are processed

            # Wait a small amount of time and try again
            time.sleep(0.1)

        self.motor_homed = True  # Update the motor homed status
    
    def select_grating_mode(self, mode):
        if not self.ser:
            self._add_status("COM port is not selected.")
            return
        if not self.arduino_initialized:
            self._add_status("The Arduino is off, please press START.")
            return
        if not self.motor_homed:
            self._add_status("Please home the motor first to start operation.")
            return

        if self.grating_selected:
            # Ask the user if they want to change the grating mode
            confirm = messagebox.askyesno("Change Grating Mode", "A grating mode is already selected. Are you sure you want to change it?")
            if not confirm:
                self._add_status("Grating mode change cancelled.")
                return  # Exit the method if the user does not want to change the mode

        # Send the initial mode selection command
        self.ser.write(b'mode_selected\n')

        # Send the grating mode selection based on user input
        if mode == "VIS Grating":
            self.ser.write(b'0\n')
        elif mode == "IR Grating":
            self.ser.write(b'1\n')
        elif mode == "Switch Mode":
            self.ser.write(b'2\n')
        else:
            self._add_status("Invalid grating mode selected.")
            return

        # Read and display all responses from the Arduino
        responses = []
        while True:
            while self.ser.in_waiting > 0:  # Check if there are more lines to read
                line = self.ser.readline().decode().strip()
                responses.append(line)
                self._add_status(line)
            self.gui.root.update_idletasks()  # Ensure the GUI stays responsive

            # Break the loop if no new data comes in for a short period
            time.sleep(0.1)
            if not self.ser.in_waiting:
                break
        self.grating_selected = True

    def set_wavelength(self, wavelength):
        if not self.ser:
            self._add_status("COM port is not selected.")
            return
        if not self.arduino_initialized:
            self._add_status("The arduino is off, please press START.")
            return
        if not self.motor_homed:
            self._add_status("Please home the motor first to start operation.")
            return
        if not self.grating_selected:
            self._add_status("Please select prefered grating ro work with and then move to desired wavelength.")
            return 
        
        # Send the command along with the wavelength to the Arduino
        command = f'wavelength_selected {wavelength}\n'
        self.ser.write(command.encode())

       # Read and display all responses from the Arduino
        responses = []
        while True:
            if self.ser.in_waiting > 0:  # Check if there are more lines to read
                line = self.ser.readline().decode().strip()
                responses.append(line)
                self._add_status(line)
                self.gui.root.update_idletasks()  # Ensure the GUI stays responsive

                # Check for the final message that indicates success or failure
                if "You can now enter a new wavelength or choose another grating to work with." in line:
                    break
                elif "Invalid wavelength" in line:
                    break

            else:
                # If there's no data, give control back to the GUI and other processes
                self.gui.root.update_idletasks()
                time.sleep(0.1)  # Small sleep to prevent tight loop from hogging the CPU


    def emergency_stop(self):
        if not self.ser:
            self._add_status("COM port is not selected.")
            return
        if not self.arduino_initialized:
            self._add_status("The arduino is off, please press START.")
            return
        if not self.motor_homed:
            self._add_status("Please home the motor first to start operation.")
            return
        self.ser.write(b'stop\n')
        time.sleep(0.1)
        response = self.ser.readline().decode().strip()
        self._add_status(response)


    def get_status(self):
        return "\n".join(self.status_messages)

    def update_status(self):
        status = self.get_status()
        self.gui.status_text.config(state=tk.NORMAL)
        self.gui.status_text.delete(1.0, tk.END)
        self.gui.status_text.insert(tk.END, status)
        self.gui.status_text.see(tk.END)  # Scroll to the end of the text
        self.gui.status_text.config(state=tk.DISABLED)

    def _add_status(self, message):
        self.status_messages.append(message)
        self.update_status()  # Trigger GUI update

    def disconnect(self):
        if self.ser:
            try:
                self.ser.close()
                self._add_status("Disconnected from Arduino.")
            except Exception as e:
                self._add_status(f"Error disconnecting: {e}")
            finally:
                self.ser = None
        else:
            self._add_status("No serial connection to disconnect.")