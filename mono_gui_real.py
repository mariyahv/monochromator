import tkinter as tk
from tkinter import ttk
from mono_class import MonochromatorControl
import threading

class MonochromatorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Monochromator Control")

        self.mainframe = ttk.Frame(root, padding="10 10 10 10")
        self.mainframe.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        self.com_ports = MonochromatorControl.list_available_ports()
        ttk.Label(self.mainframe, text="Select COM Port:").grid(row=0, column=0, sticky=tk.W)
        self.combobox = ttk.Combobox(self.mainframe, values=self.com_ports)
        self.combobox.grid(row=0, column=1, sticky=(tk.W, tk.E))

        ttk.Button(self.mainframe, text="Connect", command=lambda: self.monochromator.connect_to_monochromator(self.combobox.get())).grid(row=0, column=2, sticky=tk.W)

        # Add the "Start Arduino" button
        ttk.Button(self.mainframe, text="Start Arduino", command=lambda: self.monochromator.initialize_arduino()).grid(row=1, column=2, sticky=tk.W)

        # Add the "Home Motor" button
        ttk.Button(self.mainframe, text="Home Motor", command=lambda: self.monochromator.home_motor()).grid(row=1, column=3, sticky=tk.W)

        # Add the dropdown menu for grating mode selection
        ttk.Label(self.mainframe, text="Select Grating Mode:").grid(row=2, column=0, sticky=tk.W)
        self.grating_modes = ["VIS Grating", "IR Grating", "Switch Mode"]
        self.grating_combobox = ttk.Combobox(self.mainframe, values=self.grating_modes)
        self.grating_combobox.grid(row=2, column=1, sticky=(tk.W, tk.E))
        ttk.Button(self.mainframe, text="Set Mode", command=lambda: threading.Thread(target=self.monochromator.select_grating_mode, args=(self.grating_combobox.get(),)).start()).grid(row=2, column=2, sticky=tk.W)

        ttk.Label(self.mainframe, text="Enter Target Wavelength:").grid(row=4, column=0, columnspan=2, sticky=tk.W)

        self.wavelength_entry = ttk.Entry(self.mainframe, width=7)
        self.wavelength_entry.grid(row=5, column=0, sticky=(tk.W, tk.E))
        ttk.Button(self.mainframe, text="Set Wavelength", command=lambda: threading.Thread(target=self.monochromator.set_wavelength, args=(self.wavelength_entry.get(),)).start()).grid(row=5, column=1, sticky=tk.W)

        ttk.Button(self.mainframe, text="Emergency Stop", command=lambda: threading.Thread(target=self.monochromator.emergency_stop).start()).grid(row=6, column=0, columnspan=2, sticky=tk.W)

        # Create a Text widget for the status area
        self.status_text = tk.Text(self.mainframe, height=10, wrap=tk.WORD, state=tk.DISABLED)
        self.status_text.grid(row=7, column=0, columnspan=3, sticky=(tk.W, tk.E, tk.N, tk.S))

        # Add a vertical scrollbar for the Text widget
        self.status_scrollbar = ttk.Scrollbar(self.mainframe, orient=tk.VERTICAL, command=self.status_text.yview)
        self.status_scrollbar.grid(row=7, column=3, sticky=(tk.N, tk.S))

        self.status_text.configure(yscrollcommand=self.status_scrollbar.set)

        # Adjust window resize behavior
        root.columnconfigure(0, weight=1)
        root.rowconfigure(0, weight=1)
        self.mainframe.columnconfigure(0, weight=1)
        self.mainframe.columnconfigure(1, weight=1)
        self.mainframe.columnconfigure(2, weight=1)

        # Initialize MonochromatorControl with GUI reference
        self.monochromator = MonochromatorControl(gui=self)

if __name__ == "__main__":
    root = tk.Tk()
    app = MonochromatorGUI(root)
    root.mainloop()
