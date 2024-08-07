# fNMES-GUI

This repository hosts a Python script for a graphical user interface (GUI) designed to control electrical stimulators via Arduino boards. The GUI facilitates the precise adjustment of electrical stimulation parameters.

## Contents
- [Requirements](#requirements)
- [GUI Functionality](#gui-functionality)
  - [Command Codes](#command-codes)
  - [GUI Commands](#gui-commands)
  - [Button Actions](#button-actions)
  - [Configuration Commands](#configuration-commands)
  - [Additional Details](#additional-details)
- [Getting Started with fNMES-GUI for Beginners](#getting-started-with-fnmes-gui-for-beginners)
  - [Prerequisites](#prerequisites)
  - [Download the Script](#download-the-script)
  - [Running the Script](#running-the-script)
  - [Configure the Port](#configure-the-port)
  - [Running the GUI](#running-the-gui)
- [License](#license)

## Requirements

The GUI is developed using Python 3.8.5 and utilizes the `tkinter` package for the interface. Additional Python modules required are:
- `serial`: for sending serial commands to Arduino
- `time`: to incorporate delays in commands using `time.sleep`

Ensure the Arduino is set with a BaudRate of 19200. Connection ports differ based on the operating system:
- **Windows**: Use a ‘COM’ port.
- **Mac OS & Linux**: Use a “/dev/ttyACM0” location.

### Finding Your Port

#### Mac OS & Linux:
1. Open terminal
2. Type: `ls /dev/tty*` and note the port number listed for `/dev/ttyUSB*` or `/dev/ttyACM*`
3. The port number is represented with `*` here.

#### Windows:
1. Open Device Manager (Start → Control Panel → Hardware and Sound → Device Manager)
2. Look under “Ports” to find the matching COM port.
3. Note the number in the bracket behind the port description.

## GUI Functionality

The GUI interfaces with the Arduino by sending Hex commands to trigger specific actions in the firmware (DS5 controlled, built by Andreas Gartus, 2020).

### Command Codes
- **Matlab**: Use decimal codes (e.g., `99` for start)
- **Python**: Use Hex codes without the leading `0` (e.g., `xFF` for start)

### GUI Commands

#### Button Actions
- **Start**: `255 (0xFF)`: Initiates device, sets default settings, and opens the port for commands.
- **Blink**: `99 (0x63)`: Blinks the display as a command test (do not use continuously).
- **Display Off**: `69 (0x45) + 1`: Turns the display off.
- **Display On**: `68 (0x44) + 0`: Turns the display on.
- **Reset Pulse**: `78 (0x4E)`: Resets pulse count to zero.
- **Query Data**: `65 (0x41)`: Displays current settings on Arduino.
- **Set All**: Sends all input parameters to the Arduino.
- **START**: `80 (0x50)`: Begins pulse sequence.
- **STOP**: `33 (0x21)`: Emergency stop for pulse sequence.
- **Close Port**: Closes the serial port. Restart the GUI and Arduino connection if needed.

#### Configuration Commands
- **Bipolar OpAmp Enabled**: `98 (0x62)`: Set pulse mode to bipolar.
- **Monopolar OpAmp Enabled**: `97 (0x61)`: Set pulse mode to monopolar with OpAmp.
- **Monopolar OpAmp Disabled**: `96 (0x60)`: Set pulse mode to monopolar without OpAmp.

### Additional Details
- OpAmp adjustments are necessary for enabling bipolar stimulation.
- Monopolar stimulation can be achieved with either setting of the OpAmp.

## Getting Started with fNMES-GUI for Beginners

To use the fNMES-GUI, you'll need a basic setup and understanding of how to run Python scripts. Here's a step-by-step guide to get you started:

### Prerequisites
Before running the GUI, make sure you have the following:
- **Python Installed**: Install Python 3.8.5 or higher. You can download it from [python.org](https://www.python.org/downloads/).
- **Python Modules**: Ensure the `tkinter`, `serial`, and `time` modules are installed. You can install any required modules using pip. For example:
  ```bash
  pip install pyserial
  ```

### Download the Script
- **From GitHub**: Navigate to the GitHub repository where the fNMES-GUI script is hosted.
  - Click on the `Code` button and select `Download ZIP`, or clone the repository using:
    ```bash
    git clone [repository-url]
    ```
  - Extract the ZIP file if you downloaded it.

### Running the Script
Once you have Python and the script on your local machine, you can run the script using one of the following methods:

#### Using the Command Line
- Open Command Prompt (Windows) or Terminal (Mac and Linux).
- Navigate to the directory containing the script. For example:
  ```bash
  cd path/to/your/script
  ```
- Run the script by typing:
  ```bash
  python fNMES_GUI.py
  ```

#### Using an Integrated Development Environment (IDE)
- Open your preferred IDE (e.g., PyCharm, VSCode).
- Load the script into the IDE.
- Run the script using the IDE’s run feature.

### Configure the Port
Before running the GUI, you need to configure the script to communicate with the correct Arduino port:
- **Find the Port**:
  - Use the instructions provided in the "Finding Your Port" section above to identify your COM port (Windows) or device path (Mac/Linux).
- **Edit the Script**:
  - Open the Python script in a text editor or IDE.
  - Locate the line of code where the port is specified, which might look something like this:
    ```python
    arduino = serial.Serial('COM3', 19200)
    ```
  - Replace `'COM3'` with the port you identified earlier.
  - Save the changes.

### Running the GUI
After configuring the port, run the script as described in step 3. The GUI should launch, allowing you to interact with the Arduino-connected stimulator.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details. This license allows you to use, modify, and distribute the software as you see fit.
