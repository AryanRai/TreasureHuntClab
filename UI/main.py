import tkinter as tk
from tkinter import ttk
import serial
import serial.tools.list_ports
import threading
import re # For parsing
import datetime # For timestamps

class SerialApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Treasure Hunt Control Panel - Dual Output")
        self.root.geometry("1000x750") # Increased size for new elements

        # Serial port objects and states
        self.magnetometer_serial = None
        self.game_serial = None
        self.is_magnetometer_connected = False
        self.is_game_connected = False
        self.magnetometer_read_thread = None
        self.game_read_thread = None

        # --- Top Panes for Controls ---
        controls_pane = ttk.Frame(self.root)
        controls_pane.pack(padx=10, pady=10, fill="x")

        # --- Magnetometer Connection Settings Frame ---
        mag_connection_frame = ttk.LabelFrame(controls_pane, text="Magnetometer Connection Settings")
        mag_connection_frame.pack(side="left", padx=5, pady=5, fill="x", expand=True)

        ttk.Label(mag_connection_frame, text="COM Port:").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.mag_com_port_var = tk.StringVar()
        self.mag_com_port_combobox = ttk.Combobox(mag_connection_frame, textvariable=self.mag_com_port_var, state="readonly", width=10)
        self.mag_com_port_combobox['values'] = [port.device for port in serial.tools.list_ports.comports()]
        if self.mag_com_port_combobox['values']:
            self.mag_com_port_combobox.current(0)
        self.mag_com_port_combobox.grid(row=0, column=1, padx=5, pady=5, sticky="ew")

        ttk.Label(mag_connection_frame, text="Baud Rate:").grid(row=1, column=0, padx=5, pady=5, sticky="w")
        self.mag_baud_rate_var = tk.StringVar(value="115200")
        mag_baud_rates = ["9600", "19200", "38400", "57600", "115200"]
        self.mag_baud_rate_combobox = ttk.Combobox(mag_connection_frame, textvariable=self.mag_baud_rate_var, values=mag_baud_rates, width=10)
        self.mag_baud_rate_combobox.grid(row=1, column=1, padx=5, pady=5, sticky="ew")

        self.mag_connect_button = ttk.Button(mag_connection_frame, text="Connect Mag", command=self.toggle_magnetometer_connection)
        self.mag_connect_button.grid(row=2, column=0, columnspan=2, padx=5, pady=10)

        # --- Game Connection Settings Frame ---
        game_connection_frame = ttk.LabelFrame(controls_pane, text="Game Connection Settings")
        game_connection_frame.pack(side="left", padx=5, pady=5, fill="x", expand=True)

        ttk.Label(game_connection_frame, text="COM Port:").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.game_com_port_var = tk.StringVar()
        self.game_com_port_combobox = ttk.Combobox(game_connection_frame, textvariable=self.game_com_port_var, state="readonly", width=10)
        self.game_com_port_combobox['values'] = [port.device for port in serial.tools.list_ports.comports()]
        if self.game_com_port_combobox['values']:
            if len(self.game_com_port_combobox['values']) > 1:
                self.game_com_port_combobox.current(1) # Try to pick a different default if available
            else:
                self.game_com_port_combobox.current(0)
        self.game_com_port_combobox.grid(row=0, column=1, padx=5, pady=5, sticky="ew")

        ttk.Label(game_connection_frame, text="Baud Rate:").grid(row=1, column=0, padx=5, pady=5, sticky="w")
        self.game_baud_rate_var = tk.StringVar(value="115200")
        game_baud_rates = ["9600", "19200", "38400", "57600", "115200"]
        self.game_baud_rate_combobox = ttk.Combobox(game_connection_frame, textvariable=self.game_baud_rate_var, values=game_baud_rates, width=10)
        self.game_baud_rate_combobox.grid(row=1, column=1, padx=5, pady=5, sticky="ew")

        self.game_connect_button = ttk.Button(game_connection_frame, text="Connect Game", command=self.toggle_game_connection)
        self.game_connect_button.grid(row=2, column=0, columnspan=2, padx=5, pady=10)

        # --- Middle Panes for Game State & Commands ---
        mid_pane = ttk.Frame(self.root)
        mid_pane.pack(padx=10, pady=5, fill="x")

        # --- Game State Frame (Associated with Game Connection) ---
        game_state_frame = ttk.LabelFrame(mid_pane, text="Game State")
        game_state_frame.pack(side="left", padx=5, pady=5, fill="x", expand=True)

        self.time_remaining_var = tk.StringVar(value="N/A")
        self.score_var = tk.StringVar(value="N/A")
        self.digs_remaining_var = tk.StringVar(value="N/A")
        self.digs_taken_var = tk.StringVar(value="N/A")
        self.treasures_remaining_var = tk.StringVar(value="N/A")
        self.treasures_found_var = tk.StringVar(value="N/A")
        self.peeks_used_var = tk.StringVar(value="N/A")

        ttk.Label(game_state_frame, text="Time Remaining:").grid(row=0, column=0, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, textvariable=self.time_remaining_var).grid(row=0, column=1, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, text="Score:").grid(row=0, column=2, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, textvariable=self.score_var).grid(row=0, column=3, padx=5, pady=2, sticky="w")

        ttk.Label(game_state_frame, text="Digs Remaining:").grid(row=1, column=0, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, textvariable=self.digs_remaining_var).grid(row=1, column=1, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, text="Digs Taken:").grid(row=1, column=2, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, textvariable=self.digs_taken_var).grid(row=1, column=3, padx=5, pady=2, sticky="w")

        ttk.Label(game_state_frame, text="Treasures Remaining:").grid(row=2, column=0, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, textvariable=self.treasures_remaining_var).grid(row=2, column=1, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, text="Treasures Found:").grid(row=2, column=2, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, textvariable=self.treasures_found_var).grid(row=2, column=3, padx=5, pady=2, sticky="w")
        
        ttk.Label(game_state_frame, text="Peeks Used:").grid(row=3, column=0, padx=5, pady=2, sticky="w")
        ttk.Label(game_state_frame, textvariable=self.peeks_used_var).grid(row=3, column=1, padx=5, pady=2, sticky="w")


        # --- Send Command Frame (Associated with Game Connection) ---
        send_command_frame = ttk.LabelFrame(mid_pane, text="Send Command to Game")
        send_command_frame.pack(side="left", padx=5, pady=5, fill="x", expand=True)

        self.command_entry_var = tk.StringVar()
        self.command_entry = ttk.Entry(send_command_frame, textvariable=self.command_entry_var)
        self.command_entry.pack(side="left", fill="x", expand=True, padx=5, pady=5)
        self.send_button = ttk.Button(send_command_frame, text="Send", command=self.send_command, state="disabled")
        self.send_button.pack(side="left", padx=5, pady=5)

        # --- Bottom Panes for Logs ---
        logs_pane = ttk.Frame(self.root)
        logs_pane.pack(padx=10, pady=10, fill="both", expand=True)

        # --- Magnetometer Output Log Frame ---
        mag_output_frame = ttk.LabelFrame(logs_pane, text="Magnetometer Output Log")
        mag_output_frame.pack(side="left", padx=5, pady=5, fill="both", expand=True)

        self.mag_output_text = tk.Text(mag_output_frame, wrap="word", state="disabled", height=10)
        self.mag_output_text.pack(padx=5, pady=5, fill="both", expand=True)
        mag_scrollbar = ttk.Scrollbar(self.mag_output_text, command=self.mag_output_text.yview)
        mag_scrollbar.pack(side="right", fill="y")
        self.mag_output_text.config(yscrollcommand=mag_scrollbar.set)

        # --- Game Output Log Frame ---
        game_output_frame = ttk.LabelFrame(logs_pane, text="Game Output Log")
        game_output_frame.pack(side="left", padx=5, pady=5, fill="both", expand=True)

        self.game_output_text = tk.Text(game_output_frame, wrap="word", state="disabled", height=10)
        self.game_output_text.pack(padx=5, pady=5, fill="both", expand=True)
        game_scrollbar = ttk.Scrollbar(self.game_output_text, command=self.game_output_text.yview)
        game_scrollbar.pack(side="right", fill="y")
        self.game_output_text.config(yscrollcommand=game_scrollbar.set)

    def get_timestamp(self):
        return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]

    def toggle_magnetometer_connection(self):
        if not self.is_magnetometer_connected:
            self.connect_serial_port('magnetometer')
        else:
            self.disconnect_serial_port('magnetometer')

    def toggle_game_connection(self):
        if not self.is_game_connected:
            self.connect_serial_port('game')
        else:
            self.disconnect_serial_port('game')

    def connect_serial_port(self, port_type):
        if port_type == 'magnetometer':
            port = self.mag_com_port_var.get()
            baud_rate_str = self.mag_baud_rate_var.get()
            connect_button = self.mag_connect_button
            com_combobox = self.mag_com_port_combobox
            baud_combobox = self.mag_baud_rate_combobox
        elif port_type == 'game':
            port = self.game_com_port_var.get()
            baud_rate_str = self.game_baud_rate_var.get()
            connect_button = self.game_connect_button
            com_combobox = self.game_com_port_combobox
            baud_combobox = self.game_baud_rate_combobox
            send_cmd_button = self.send_button
        else:
            return

        if not port:
            self.log_to_output(f"Error: {port_type.capitalize()} COM port not selected.", port_type)
            return
        if not baud_rate_str:
            self.log_to_output(f"Error: {port_type.capitalize()} Baud rate not selected.", port_type)
            return
        
        try:
            baud_rate = int(baud_rate_str)
            serial_instance = serial.Serial(port, baud_rate, timeout=1)
            
            if port_type == 'magnetometer':
                self.magnetometer_serial = serial_instance
                self.is_magnetometer_connected = True
                self.magnetometer_read_thread = threading.Thread(target=self.read_serial_data, args=(port_type,), daemon=True)
                self.magnetometer_read_thread.start()
            elif port_type == 'game':
                self.game_serial = serial_instance
                self.is_game_connected = True
                send_cmd_button.config(state="normal")
                # Reset game state display on new game connection
                self.time_remaining_var.set("N/A")
                self.score_var.set("N/A")
                self.digs_remaining_var.set("N/A")
                self.digs_taken_var.set("N/A")
                self.treasures_remaining_var.set("N/A")
                self.treasures_found_var.set("N/A")
                self.peeks_used_var.set("N/A")
                self.game_read_thread = threading.Thread(target=self.read_serial_data, args=(port_type,), daemon=True)
                self.game_read_thread.start()

            connect_button.config(text=f"Disconnect {port_type.capitalize()}")
            self.log_to_output(f"Connected to {port_type.capitalize()} ({port}) at {baud_rate} baud.", port_type)
            com_combobox.config(state="disabled")
            baud_combobox.config(state="disabled")

        except serial.SerialException as e:
            self.log_to_output(f"Error connecting to {port_type.capitalize()}: {e}", port_type)
            if port_type == 'magnetometer':
                self.is_magnetometer_connected = False
                self.magnetometer_serial = None
            elif port_type == 'game':
                self.is_game_connected = False
                self.game_serial = None
                send_cmd_button.config(state="disabled")
        except ValueError:
            self.log_to_output(f"Error: Invalid Baud rate for {port_type.capitalize()}.", port_type)


    def disconnect_serial_port(self, port_type):
        serial_port_instance = None
        # is_connected_flag = False # This variable seems unused, can be removed
        read_thread_instance = None
        connect_button = None
        com_combobox = None
        baud_combobox = None

        if port_type == 'magnetometer':
            serial_port_instance = self.magnetometer_serial
            self.is_magnetometer_connected = False # Signal thread to stop
            read_thread_instance = self.magnetometer_read_thread
            connect_button = self.mag_connect_button
            com_combobox = self.mag_com_port_combobox
            baud_combobox = self.mag_baud_rate_combobox
        elif port_type == 'game':
            serial_port_instance = self.game_serial
            self.is_game_connected = False # Signal thread to stop
            read_thread_instance = self.game_read_thread
            connect_button = self.game_connect_button
            com_combobox = self.game_com_port_combobox
            baud_combobox = self.game_baud_rate_combobox
            self.send_button.config(state="disabled")
        else:
            return

        if serial_port_instance and serial_port_instance.is_open:
            serial_port_instance.close()
        
        if read_thread_instance and read_thread_instance.is_alive():
            read_thread_instance.join(timeout=1)

        if port_type == 'magnetometer':
            self.magnetometer_serial = None
        elif port_type == 'game':
            self.game_serial = None
            # Optionally reset game state variables on game disconnect
            # self.time_remaining_var.set("N/A") ... etc.

        connect_button.config(text=f"Connect {port_type.capitalize()}")
        self.log_to_output(f"{port_type.capitalize()} disconnected.", port_type)
        com_combobox.config(state="readonly")
        # Mag baud usually fixed by device, game baud can be changed
        baud_combobox.config(state="normal" if port_type == 'game' or not self.mag_baud_rate_combobox['values'] else "readonly")


    def read_serial_data(self, port_type):
        serial_port_instance = None
        is_connected_check = lambda: False

        if port_type == 'magnetometer':
            serial_port_instance = self.magnetometer_serial
            is_connected_check = lambda: self.is_magnetometer_connected
        elif port_type == 'game':
            serial_port_instance = self.game_serial
            is_connected_check = lambda: self.is_game_connected
        else:
            return

        try:
            while is_connected_check() and serial_port_instance and serial_port_instance.is_open:
                if serial_port_instance.in_waiting > 0:
                    try:
                        raw_data = serial_port_instance.readline()
                        data = raw_data.decode('utf-8', errors='replace').strip()
                        if data:
                            # Process in main thread
                            self.root.after(0, self.process_received_data, data, port_type)
                    except serial.SerialException as e:
                        self.log_to_output(f"Serial read error on {port_type}: {e}", port_type)
                        self.root.after(0, self.disconnect_serial_port, port_type)
                        break
                    except Exception as e:
                        self.log_to_output(f"Read error on {port_type}: {e}", port_type)
        except Exception as e:
            self.log_to_output(f"Serial thread error on {port_type}: {e}", port_type)
        finally:
            if is_connected_check(): # If still marked connected, an error likely occurred
                 self.root.after(0, self.disconnect_serial_port, port_type)


    def process_received_data(self, data_string, port_type):
        self.log_to_output(data_string, port_type) # Log all data with timestamp

        if port_type == 'game':
            self.parse_and_update_game_ui(data_string)

    def parse_and_update_game_ui(self, data_string):
        # Try to parse "GAME STATE: Score: X | Digs Left: Y, Digs Taken: A | Treasures Left: Z, Treasures Found: B | Peeks Used: C | Time: D"
        game_state_match = re.search(
            r"GAME STATE: Score: (\d+) \| Digs Left: (\d+), Digs Taken: (\d+) \| Treasures Left: (\d+), Treasures Found: (\d+) \| Peeks Used: (\d+) \| Time: (\d+)",
            data_string
        )
        if game_state_match:
            self.score_var.set(game_state_match.group(1))
            self.digs_remaining_var.set(game_state_match.group(2))
            self.digs_taken_var.set(game_state_match.group(3))
            self.treasures_remaining_var.set(game_state_match.group(4))
            self.treasures_found_var.set(game_state_match.group(5))
            self.peeks_used_var.set(game_state_match.group(6))
            self.time_remaining_var.set(game_state_match.group(7))
            return # Parsed game state, no need to check older formats for this line

        # Fallback for "TIME REMAINING:X" if GAME STATE not matched
        time_match = re.search(r"TIME REMAINING:(\d+)", data_string)
        if time_match:
            self.time_remaining_var.set(time_match.group(1))

        # Fallback for "DIGS REMAINING:Y TREASURES:Z"
        # This might be redundant if GAME STATE is comprehensive and consistently sent
        digs_treasures_match = re.search(r"DIGS REMAINING:(\d+)\s+TREASURES:(\d+)", data_string)
        if digs_treasures_match:
            self.digs_remaining_var.set(digs_treasures_match.group(1))
            self.treasures_remaining_var.set(digs_treasures_match.group(2))


    def send_command(self):
        if not self.is_game_connected or not self.game_serial or not self.game_serial.is_open:
            self.log_to_output("Error: Not connected to Game device.", 'game')
            return

        command = self.command_entry_var.get()
        if command:
            try:
                full_command = command + "\r" 
                self.game_serial.write(full_command.encode('utf-8'))
                self.log_to_output(f"Sent to Game: {command}", 'game')
                self.command_entry_var.set("")
            except serial.SerialException as e:
                self.log_to_output(f"Error sending command to Game: {e}", 'game')
            except Exception as e:
                self.log_to_output(f"Unexpected error sending command to Game: {e}", 'game')
        else:
            self.log_to_output("Cannot send empty command to Game.", 'game')


    def log_to_output(self, message, port_type):
        timestamp = self.get_timestamp()
        full_message = f"[{timestamp}] {message}"
        # Schedule the UI update on the main thread.
        self.root.after(0, self._log_to_output_ui_threaded, full_message, port_type)

    def _log_to_output_ui_threaded(self, full_message, port_type):
        text_widget = None
        if port_type == 'magnetometer':
            text_widget = self.mag_output_text
        elif port_type == 'game':
            text_widget = self.game_output_text
        else:
            return # Should not happen

        if text_widget:
            text_widget.config(state="normal")
            text_widget.insert(tk.END, full_message + "\n")
            text_widget.see(tk.END) # Auto-scroll
            text_widget.config(state="disabled")


if __name__ == "__main__":
    root = tk.Tk()
    app = SerialApp(root)
    root.mainloop()
