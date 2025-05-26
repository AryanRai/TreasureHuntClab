import tkinter as tk
from tkinter import ttk
import serial
import serial.tools.list_ports
import threading
import re # For parsing

class SerialApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Treasure Hunt Control Panel")
        self.root.geometry("700x550") # Increased size for new elements

        self.serial_port = None
        self.is_connected = False
        self.read_thread = None

        # --- Connection Settings Frame ---
        connection_frame = ttk.LabelFrame(self.root, text="Connection Settings")
        connection_frame.pack(padx=10, pady=10, fill="x")

        ttk.Label(connection_frame, text="COM Port:").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.com_port_var = tk.StringVar()
        self.com_port_combobox = ttk.Combobox(connection_frame, textvariable=self.com_port_var, state="readonly")
        self.com_port_combobox['values'] = [port.device for port in serial.tools.list_ports.comports()]
        if self.com_port_combobox['values']:
            self.com_port_combobox.current(0)
        self.com_port_combobox.grid(row=0, column=1, padx=5, pady=5, sticky="ew")

        ttk.Label(connection_frame, text="Baud Rate:").grid(row=1, column=0, padx=5, pady=5, sticky="w")
        self.baud_rate_var = tk.StringVar(value="115200")
        baud_rates = ["9600", "19200", "38400", "57600", "115200"]
        self.baud_rate_combobox = ttk.Combobox(connection_frame, textvariable=self.baud_rate_var, values=baud_rates)
        self.baud_rate_combobox.grid(row=1, column=1, padx=5, pady=5, sticky="ew")

        self.connect_button = ttk.Button(connection_frame, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=2, column=0, columnspan=2, padx=5, pady=10)

        # --- Game State Frame ---
        game_state_frame = ttk.LabelFrame(self.root, text="Game State")
        game_state_frame.pack(padx=10, pady=5, fill="x")

        self.time_remaining_var = tk.StringVar(value="N/A")
        self.digs_remaining_var = tk.StringVar(value="N/A")
        self.treasures_remaining_var = tk.StringVar(value="N/A")

        ttk.Label(game_state_frame, text="Time Remaining:").grid(row=0, column=0, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, textvariable=self.time_remaining_var).grid(row=0, column=1, padx=5, pady=2, sticky="w")

        ttk.Label(game_state_frame, text="Digs Remaining:").grid(row=1, column=0, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, textvariable=self.digs_remaining_var).grid(row=1, column=1, padx=5, pady=2, sticky="w")

        ttk.Label(game_state_frame, text="Treasures Remaining:").grid(row=2, column=0, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, textvariable=self.treasures_remaining_var).grid(row=2, column=1, padx=5, pady=2, sticky="w")

        # --- Send Command Frame ---
        send_command_frame = ttk.LabelFrame(self.root, text="Send Command")
        send_command_frame.pack(padx=10, pady=5, fill="x")

        self.command_entry_var = tk.StringVar()
        self.command_entry = ttk.Entry(send_command_frame, textvariable=self.command_entry_var)
        self.command_entry.pack(side="left", fill="x", expand=True, padx=5, pady=5)
        self.send_button = ttk.Button(send_command_frame, text="Send", command=self.send_command, state="disabled")
        self.send_button.pack(side="left", padx=5, pady=5)


        # --- Serial Output Frame ---
        output_frame = ttk.LabelFrame(self.root, text="Serial Output Log")
        output_frame.pack(padx=10, pady=10, fill="both", expand=True)

        self.output_text = tk.Text(output_frame, wrap="word", state="disabled", height=10)
        self.output_text.pack(padx=5, pady=5, fill="both", expand=True)
        
        scrollbar = ttk.Scrollbar(self.output_text, command=self.output_text.yview)
        scrollbar.pack(side="right", fill="y")
        self.output_text.config(yscrollcommand=scrollbar.set)


    def toggle_connection(self):
        if not self.is_connected:
            self.connect_serial()
        else:
            self.disconnect_serial()

    def connect_serial(self):
        port = self.com_port_var.get()
        baud_rate = self.baud_rate_var.get()

        if not port:
            self.log_to_output_text("Error: COM port not selected.")
            return
        if not baud_rate:
            self.log_to_output_text("Error: Baud rate not selected.")
            return

        try:
            self.serial_port = serial.Serial(port, int(baud_rate), timeout=1)
            self.is_connected = True
            self.connect_button.config(text="Disconnect")
            self.log_to_output_text(f"Connected to {port} at {baud_rate} baud.")
            
            self.com_port_combobox.config(state="disabled")
            self.baud_rate_combobox.config(state="disabled")
            self.send_button.config(state="normal") # Enable send button

            # Reset game state display
            self.time_remaining_var.set("N/A")
            self.digs_remaining_var.set("N/A")
            self.treasures_remaining_var.set("N/A")

            self.read_thread = threading.Thread(target=self.read_serial_data, daemon=True)
            self.read_thread.start()

        except serial.SerialException as e:
            self.log_to_output_text(f"Error connecting: {e}")
            self.is_connected = False
            self.serial_port = None
            self.send_button.config(state="disabled")


    def disconnect_serial(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.is_connected = False # Set this before join to signal thread to stop
        if self.read_thread and self.read_thread.is_alive():
             self.read_thread.join(timeout=1) # Wait for thread to finish


        self.connect_button.config(text="Connect")
        self.log_to_output_text("Disconnected.")
        
        self.com_port_combobox.config(state="readonly")
        self.baud_rate_combobox.config(state="normal")
        self.send_button.config(state="disabled") # Disable send button
        # Optionally reset game state variables on disconnect
        # self.time_remaining_var.set("N/A")
        # self.digs_remaining_var.set("N/A")
        # self.treasures_remaining_var.set("N/A")


    def read_serial_data(self):
        try:
            while self.is_connected and self.serial_port and self.serial_port.is_open:
                if self.serial_port.in_waiting > 0:
                    try:
                        raw_data = self.serial_port.readline()
                        data = raw_data.decode('utf-8', errors='replace').strip()
                        if data:
                            self.root.after(0, self.parse_and_update_ui, data) # Process in main thread
                    except serial.SerialException as e: # Catch specific serial errors during read
                        self.log_to_output_text(f"Serial read error: {e}")
                        self.root.after(0, self.disconnect_serial) # Ensure clean disconnect on error
                        break # Exit read loop
                    except Exception as e:
                        self.log_to_output_text(f"Read error: {e}")
        except Exception as e: # Catch unexpected errors in the thread itself
            self.log_to_output_text(f"Serial thread error: {e}")
        finally:
            # This block ensures that if the loop terminates for any reason (e.g., disconnect),
            # the UI is updated to reflect the disconnected state from the main thread.
            if self.is_connected: # If still marked as connected, means an error occurred.
                 self.root.after(0, self.disconnect_serial)


    def parse_and_update_ui(self, data_string):
        parsed = False
        # Try to parse "TIME REMAINING:X"
        time_match = re.search(r"TIME REMAINING:(\d+)", data_string)
        if time_match:
            self.time_remaining_var.set(time_match.group(1))
            parsed = True

        # Try to parse "DIGS REMAINING:Y TREASURES:Z"
        digs_treasures_match = re.search(r"DIGS REMAINING:(\d+)\s+TREASURES:(\d+)", data_string)
        if digs_treasures_match:
            self.digs_remaining_var.set(digs_treasures_match.group(1))
            self.treasures_remaining_var.set(digs_treasures_match.group(2))
            parsed = True
        
        # Log to main output if not parsed or if it's a general message
        # We can refine this logic if some messages are only for status and not for log
        # For now, all messages are logged if they are not game status updates, or if they are.
        self.log_to_output_text(data_string) # Log all messages for now

    def send_command(self):
        if not self.is_connected or not self.serial_port or not self.serial_port.is_open:
            self.log_to_output_text("Error: Not connected to any device.")
            return

        command = self.command_entry_var.get()
        if command:
            try:
                # STM32 side might expect a newline character to process the command
                full_command = command + "\\r\\n" # Or just "\\n" depending on STM32 setup
                self.serial_port.write(full_command.encode('utf-8'))
                self.log_to_output_text(f"Sent: {command}")
                self.command_entry_var.set("") # Clear entry after sending
            except serial.SerialException as e:
                self.log_to_output_text(f"Error sending command: {e}")
            except Exception as e:
                self.log_to_output_text(f"Unexpected error sending command: {e}")
        else:
            self.log_to_output_text("Cannot send empty command.")


    def log_to_output_text(self, message):
        # This method might be called from a different thread,
        # so use `after` to schedule the UI update on the main thread.
        # It's now specifically for the main output log.
        self.root.after(0, self._log_to_output_text_ui, message)

    def _log_to_output_text_ui(self, message):
        self.output_text.config(state="normal")
        self.output_text.insert(tk.END, message + "\\n")
        self.output_text.see(tk.END) # Auto-scroll
        self.output_text.config(state="disabled")


if __name__ == "__main__":
    root = tk.Tk()
    app = SerialApp(root)
    root.mainloop()
