# TreasureHuntClab


```
███╗   ███╗████████╗██████╗ ██╗  ██╗██████╗ ███████╗ ██████╗  ██████╗ 
████╗ ████║╚══██╔══╝██╔══██╗╚██╗██╔╝╚════██╗╚════██║██╔═████╗██╔═████╗
██╔████╔██║   ██║   ██████╔╝ ╚███╔╝  █████╔╝    ██╔╝██║██╔██║██║██╔██║
██║╚██╔╝██║   ██║   ██╔══██╗ ██╔██╗ ██╔═══╝    ██╔╝ ████╔╝██║████╔╝██║
██║ ╚═╝ ██║   ██║   ██║  ██║██╔╝ ██╗███████╗   ██║  ╚██████╔╝╚██████╔╝
╚═╝     ╚═╝   ╚═╝   ╚═╝  ╚═╝╚══════╝   ╚═╝   ╚═════╝  ╚═════╝ 
```
# TreasureHuntClab - Week 9 Group XYZ

> **Memory Operations | I/O Control | Serial Communication | Timer Management**

[![STM32F303VCT6](https://img.shields.io/badge/STM32-F303VCT6-blue)](https://canvas.sydney.edu.au/courses/63055/files/40530474?wrap=1)
[![Assembly](https://img.shields.io/badge/Language-Assembly-red)](https://developer.arm.com/)
[![Documentation](https://img.shields.io/badge/Docs-Markdown-lightgrey)](docs/)

## Team Members & Roles
-   (`530522656`) - 
- Steven Hughes (`311246486`) - 
- Aryan Rai (`530362258`) - 
-   (`530439147`) - 

## 🎯 Project Overview
This project implements core microcontroller functionality:
```
┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐
│  Memory Handling │   │    I/O Control   │   │  Serial Comms    │
│  ▪ Pointers     │   │  ▪ LED Patterns  │   │  ▪ UART Config   │
│  ▪ Iterators    │   │  ▪ Button Input  │   │                  │
└──────────────────┘   └──────────────────┘   └──────────────────┘
         │                      │                      │
         └──────────────────────┼──────────────────────┘
                               │
                    ┌──────────────────┐   ┌──────────────────┐
                    │    Integration   │   │  Timer Control   │
                    │  ▪ Module Sync  │   │  ▪ Delays        │
                    │  ▪ Data Flow    │   │  ▪ LED Patterns  │
                    └──────────────────┘   └──────────────────┘
```

## Exercise Implementation Map

### Exercise 1: Memory and Pointers
- **Location**: `/Final/TASKS/ex1/`
- **Implementation**:
  - Case conversion (Task a)
  - Palindrome detection (Task b)
  - Caesar cipher (Task c)
- **Key Files**: 
  - `assembly_encode.s`: Case Change, Palindrome Check, Cipher Encode implementation
  - `assembly_decode.s`: Case Change, Palindrome Check, Cipher Decode implementation

### Exercise 2: Digital I/O
- **Location**: `/Final/TASKS/ex2/`
- **Implementation**:
  - LED pattern control (Task a)
  - Button-controlled LED sequence (Tasks b, c)
  - Vowel/consonant counter with LED display (Task d)
- **Key Files**:
  - `LED_Loader/Src.142ab.s`: LED pattern handling, input processing
  - `VOWELS/Src/CSIN4011_142_VOWELS.s`: Text analysis, LED display

### Exercise 3: Serial Communication
- **Location**: `/Final/TASKS/ex3/`
- **Implementation**:
  - String transmission (Task a): `uart_transmit_rec_basic`
  - String reception (Task b): `Serial_Task4`
  - Clock speed configuration (Task c): `uart_transmit_rec_speed_change`
  - Echo functionality (Task d): `Serial_Task4`
  - Dual UART port forwarding (Task e)
- **Key Files**:
  - `assembly.s`: Combined transmission and reception code
  - `initialise.s`: UART configuration and initialisation

### Exercise 4: Hardware Timers
- **Location**: `/Final/TASKS/ex4/`
- **Implementation**:
  - Microsecond delay function (Task a)
  - Prescaler configuration (Task b)
  - Hardware-managed delays (Task c)
- **Key Files**:
  - `assembly.s`: Delay implementation, LED pattern display
  - `initialise.s`: Timer setup

### Exercise 5: Integration
- **Location**: `/Final/Main/`
- **Implementation**:
  - Board-to-board communication
  - Palindrome detection and cipher
  - LED-based character analysis
- **Key Files**:
  - `send/main.s`: First board implementation
  - `receive/main.s`: Second board implementation

## Assessment Coverage

### Core Requirements
1. Memory Handling (Ex1)
   - Pointer operations in cipher/palindrome
   - String iteration and manipulation

2. I/O Operations (Ex2)
   - LED pattern control
   - Button debouncing
   - Binary output display  

LED Pattern Control:

https://github.com/user-attachments/assets/bdac061a-b451-4314-88f6-3a179d7298a3

Counting Vowels:

https://github.com/user-attachments/assets/e06e7996-31d5-4a54-9618-5b1d6f606961

3. Serial Interface (Ex3)
   - UART configuration
   - Asynchronous communication
   - Multi-UART handling

[![](https://github.com/user-attachments/assets/5ab39d19-b37a-4301-a085-e49b0cb58d94)](https://github.com/user-attachments/assets/5ab39d19-b37a-4301-a085-e49b0cb58d94)

4. Timer Operations (Ex4)
   - Hardware timer configuration
   - Precise delay generation
   - Prescaler optimization

https://github.com/user-attachments/assets/bd740a32-b9af-4531-8906-b5b89e33c89a

5. Polling Interface (Ex2, Ex3)
   - Button state detection
   - Input validation

6. Module Integration (Ex5)
   - Inter-board communication
   - Function coordination
   - State management

7. Modular Design
   - Encapsulated functionality
   - Clear interfaces
   - Separated concerns

## 📁 Repository Structure
```
MTRX2700_Wk1_Grp9/
├── 📂 Assignment1/           # Development Archive
├── 📂 Final/                 # Implementation
│   ├── 📂 Main/             # Integration
│   │   ├── 📄 send/         # Transmission
│   │   └── 📄 receive/      # Reception
│   └── 📂 TASKS/           
        ├── 📄 ex1/          # Memory Ops
        ├── 📄 ex2/          # I/O Control
        ├── 📄 ex3/          # UART Comms
        └── 📄 ex4/          # Timers
```

## Module Organization

### Exercise 1: Memory Operations
- Text processing and cipher implementations
- Case conversion (upper/lower)
- Palindrome detection
- Caesar cipher (encode/decode)
- Located in `/Final/TASKS/ex1/`

### Exercise 2: I/O Operations
- LED control and GPIO handling
- Button interface management
- Binary counting display
- Located in `/Final/TASKS/ex2/`

### Exercise 3: UART Communication
- Serial interface implementation
- Buffer management
- Message handling
- Located in `/Final/TASKS/ex3/`

### Exercise 4: Timer Operations
- Basic and advanced timer control
- LED patterns and delays
- Located in `/Final/TASKS/ex4/`

### Exercise 5: Integration
- Send/receive functionality
- Module coordination
- Located in `/Final/Main/`

## User Instructions
1. Clone the repository
2. Open project in STM32CubeIDE
3. Build the project
4. Flash to STM32F303 Discovery board
5. For dual-board exercises (Ex5), connect:
   - UART pins between boards (send pin B3 -> rec pin B4)
   - Common ground
   - Power both boards
  
Below is a block diagram showing the communication between the two boards:
![Integration block diagram](https://github.com/user-attachments/assets/20b32018-30c2-4f97-a25b-1c915f4c2dba)

### General Timer Delay Formula

The time delay produced by a timer depends on its clock frequency and the number of ticks it counts. The key formula is:

Delay (seconds) = Number of Ticks × Tick Period (seconds)

Where:
- Tick Period = 1 / Timer Clock Frequency (Hz)
- Timer Clock Frequency = System Clock Frequency (Hz) / (Prescaler + 1)

#### Variables:

#### Specific Formulas for Your Code

#### 1. accurate_delay_us (Output Compare Mode)
This function delays for a duration specified in microseconds (passed in R1), using output compare channel 1 (TIM_CCR1).

Formula:
Delay (µs) = TIM_CCR1 × Tick Period (µs)
Where:
- Tick Period (µs) = 1 / Timer Clock Frequency (MHz)
- Timer Clock Frequency (MHz) = System Clock Frequency (MHz) / (PSC + 1)

Code Values:
- System Clock = 8 MHz (assumed HSI default)
- PSC = 7 (set via MOV R4, #7; STR R4, [R0, #TIM_PSC])
- TIM_CCR1 = R1 (e.g., 500000 in blink_leds)

Calculation:
1. Timer Clock Frequency = 8,000,000 / 8 = 1,000,000 Hz = 1 MHz
2. Tick Period = 1 / 1,000,000 = 1 µs
3. Delay = TIM_CCR1 × 1 µs

Example (blink_leds):
- R1 = 500000:
  - Delay = 500,000 × 1 µs = 500,000 µs = 500 ms = 0.5 seconds

#### 2. demo_100us (Update Event Mode)
This function generates a 0.1ms period and counts 10,000 periods to total 1 second.

Period Formula:
Period (µs) = (TIM_ARR + 1) × Tick Period (µs)
Note: +1 because counter resets after reaching ARR (counts from 0 to ARR inclusive)

Total Delay Formula:
Total Delay (µs) = (TIM_ARR + 1) × Number of Periods × Tick Period (µs)

Code Values:
- System Clock = 8 MHz
- PSC = 7 (1 MHz timer clock, 1 µs/tick)
- TIM_ARR = 100
- Number of Periods = 10,000

Calculation:
1. Timer Clock = 1 MHz (as above)
2. Tick Period = 1 µs
3. Period = (100 + 1) × 1 µs = 101 µs ≈ 0.1 ms
4. Total Delay = 101 × 10,000 × 1 µs = 1,010,000 µs = 1.01 seconds

For Exact 0.1ms:
- Use TIM_ARR = 99:
  - Period = (99 + 1) × 1 µs = 100 µs = 0.1 ms
  - Total = 100 × 10,000 × 1 µs = 1,000,000 µs = 1 second

## Testing Procedures
Each module includes unit tests verifying:
- Memory operations
- I/O functionality
- UART communication
- Timer 
- Integration tests

Detailed test procedures and results are documented in each module's respective folder within Docs.md

## Documentation Links

### Main Task Documentation
- [Timer Documentation](Final/TASKS/TimerDocs.md) - Timer configuration and LED control
- [UART Documentation](Final/TASKS/UARTDocs.md) - UART communication implementation
- [IO Documentation](Final/TASKS/IODocs.md) - GPIO and LED control
- [Memory Documentation](Final/TASKS/CypherDocs.md) - Text processing and cipher operations
- [Integration Documentation](Final/MAIN/Docs.md) - Two-board communication system

### Quick Task Overview

#### Timer Module (TimerDocs.md)
- LED blinking control using Timer 2
- Precise microsecond delays
- Configurable prescaler settings
- LED pattern display

#### UART Module (UARTDocs.md)
- UART communication implementation
- Configurable transmission settings
- Buffer management
- Message handling

#### Vowel/Consonant Counter (VOWELSDocs.md)
- Text analysis for vowels/consonants
- LED binary display output
- Mode switching functionality
- UART input processing

#### Text Processing & Cipher Operations
- Case conversion (upper/lower)
- Palindrome detection
- Caesar cipher encoding/decoding
- ASCII validation and handling
- Configurable shift values

### Integration Module (Final/MAIN/Docs.md)

#### Board 1 (Sender)
- UART1 terminal communication (PC4/PC5)
- UART2 board communication (PB3/PB4)
- Palindrome detection and encryption
- Debug output and ACK handling

#### Board 2 (Receiver)
- UART2 input processing
- Caesar cipher decryption
- Vowel/consonant LED display
- 500ms display toggle
- Button-controlled mode switching

#### Communication Protocol
1. Board 1 receives string via UART1
2. Processes palindrome check
3. Encrypts if palindrome (+5 shift)
4. Sends to Board 2 via UART2
5. Waits for ACK before next input

#### Hardware Setup
- **Board 1**:
  - UART1: PC4(TX)/PC5(RX) - Terminal
  - UART2: PB3(TX)/PB4(RX) - Board 2
- **Board 2**:
  - UART2: PB3(RX)/PB4(TX) - Board 1
  - GPIOE[7:0]: LED output
  - PA0: Mode button

#### Testing Setup
- **Terminal Software**:
  - Board 1: Arduino IDE Serial Monitor/PuTTY (115200, 8N1, Local echo) (Use Newline Termination) 
  - Board 2: Arduino IDE Serial Monitor/PuTTY (115200, 8N1, Local echo) (Use Termination)
- **Hardware Connections**:
  - USB-Serial adapters for both boards
  - Cross-connected UART2 pins
  - Common ground between boards
- **Verification Tools**:
  - Real-time hex display in PuTTY
  - Decoded output in Arduino Monitor
  - Visual LED pattern verification

## Detailed Testing Setup

### Exercise 1: Memory Operations Testing
- **Requirements**:
  - Single STM32F303 board
  - USB-Serial adapter
  - Terminal software (115200 baud, 8N1)
- **Test Cases**:
  - Case conversion: "Hello World" → "HELLO WORLD"
  - Palindrome: "radar", "hello"
  - Caesar cipher: "HELLO" (+3 shift)

### Exercise 2: I/O Testing
- **Hardware Setup**:
  - STM32F303 board
  - User button (PA0)
  - LEDs (GPIOE[7:0])
- **Test Sequences**:
  - LED patterns: Binary counting (0-255)
  - Button debounce: Press/release timing
  - Vowel counter: Input "HELLO" → LED display

### Exercise 3: UART Testing
- **Equipment**:
  - STM32F303 board
  - 2x USB-Serial adapters
  - Jumper wires
- **Connections**:
  - UART1: PC4(TX) → USB-Serial
  - UART2: PB3(TX) → PB4(RX) (loopback)
- **Test Protocol**:
  - Basic transmission: "TEST"
  - Speed changes: 9600, 115200 baud
  - Echo test: Send/receive verification

### Exercise 4: Timer Testing
- **Setup**:
  - STM32F303 board
  - Oscilloscope (optional)
- **Verification**:
  - 1ms delay accuracy
  - LED blink patterns
  - Prescaler configurations
- **Test Patterns**:
  - 500ms toggle
  - Variable duty cycles
  - Pattern sequences

### Exercise 5: Integration Testing
- **Hardware**:
  - 2x STM32F303 boards
  - 2x USB-Serial adapters
  - Jumper wires
- **Connections**:
  - Board1 PB3 → Board2 PB4 (UART2)
  - Common ground
- **Test Sequence**:
  1. Send palindrome "radar"
  2. Verify encryption
  3. Check LED display
  4. Test mode switching
  5. Verify ACK protocol

# End of Documentation


