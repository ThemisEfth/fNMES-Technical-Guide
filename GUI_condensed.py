# -*- coding: utf-8 -*-
"""
# -----------------------------------------------------------------------------
# fNMES-GUI: Graphical User Interface for Facial Neuromuscular Electrical Stimulation
# -----------------------------------------------------------------------------
# Created on: September 11, 2020
# Author: Dr. Themis Niolas Efthimiou
# Contact: themiftw20@gmail.com
# -----------------------------------------------------------------------------
# Description:
# This Python script provides a graphical user interface (GUI) for controlling
# electrical stimulators via Arduino boards. It facilitates the precise adjustment
# of electrical stimulation parameters, enabling researchers to perform facial
# neuromuscular electrical stimulation (fNMES) in experimental settings.
# -----------------------------------------------------------------------------
# License:
# This project is licensed under the MIT License - see the LICENSE.md file for details.
# -----------------------------------------------------------------------------
"""

import time
import serial
from tkinter import *

# Find port address MAC
# To find the address of the serial port, open terminal and type: ls /dev/*
# The address should be something like: /dev/cu.usbmodem146301

# Find port address on Windows
# To find the address of the serial port, open Device Manager.
# Steps to follow:
# 1. Open Device Manager (you can search for it in the Start menu).
# 2. Expand the "Ports (COM & LPT)" section.
# 3. Locate your Arduino device. The port number will be displayed as "COM" followed by a number (e.g., COM3).
# Use this COM port number in your script.




# Create GUI Window
window = Tk()
window.title("Arduino Controller")
window.geometry("950x450")

# Define GUI functions

def blink():
    ser1.write(b'\x63')
    ser2.write(b'\x63')

def LEDOFF():
    ser1.write(b'\x44')
    ser1.write(b'\x00')
    ser2.write(b'\x44')
    ser2.write(b'\x00')

def LEDON():
    ser1.write(b'\x45')
    ser1.write(b'\x01')
    ser2.write(b'\x45')
    ser2.write(b'\x01')

def Train_Reset():
    ser1.write(b'\x4E')
    ser2.write(b'\x4E')

def Start_stim_DAC1():
    ser1.write(b'\x50')

def Start_stim_DAC2():
    ser2.write(b'\x50')

def Start_stim_DAC1_2():
    ser1.write(b'\x50')
    ser2.write(b'\x50')

def stop():
    ser1.write(b'\x21')
    ser2.write(b'\x21')

def Close_COM():
    ser1.close()
    ser2.close()

def Bipolar():
    ser1.write(b'\x62')
    ser2.write(b'\x62')

def MonopolarEN():
    ser1.write(b'\x61')
    ser2.write(b'\x61')

def Pulses_DAC1():
    Pulse_no = int(P_Input.get())
    HB_p = Pulse_no // 256
    LB_p = Pulse_no % 256
    P_HB = HB_p.to_bytes(1, byteorder="little")
    P_LB = LB_p.to_bytes(1, byteorder="little")
    ser1.write(b'\x59')
    ser1.write(P_HB)
    ser1.write(P_LB)

def Pulses_DAC2():
    Pulse_no = int(P_Input2.get())
    HB_p = Pulse_no // 256
    LB_p = Pulse_no % 256
    P_HB = HB_p.to_bytes(1, byteorder="little")
    P_LB = LB_p.to_bytes(1, byteorder="little")
    ser2.write(b'\x59')
    ser2.write(P_HB)
    ser2.write(P_LB)

def Output_Voltage_DAC1():
    OV = float(OV_Input.get())
    HB_ov = int(OV // 128)
    LB_ov = int((OV % 128) / 0.5)
    OV_HB = HB_ov.to_bytes(1, byteorder="little")
    OV_LB = LB_ov.to_bytes(1, byteorder="little")
    ser1.write(b'\x53')
    ser1.write(OV_HB)
    ser1.write(OV_LB)

def Output_Voltage_DAC2():
    OV = float(OV_Input2.get())
    HB_ov = int(OV // 128)
    LB_ov = int((OV % 128) / 0.5)
    OV_HB = HB_ov.to_bytes(1, byteorder="little")
    OV_LB = LB_ov.to_bytes(1, byteorder="little")
    ser2.write(b'\x53')
    ser2.write(OV_HB)
    ser2.write(OV_LB)

def Pulse_Delay_ms_DAC1():
    PD_ms = int(PP_Input.get())
    PDms_hb = PD_ms // 25.6
    PDms_lb = int(round((PD_ms % 25.6) / 0.1))
    PDms_HB = PDms_hb.to_bytes(1, byteorder="little")
    PDms_LB = PDms_lb.to_bytes(1, byteorder="little")
    if PDms_hb > 0 or PDms_lb > 0:
        ser1.write(b'\x58')
        ser1.write(PDms_HB)
        ser1.write(PDms_LB)

def Pulse_Delay_ms_DAC2():
    PD_ms = int(PP_Input2.get())
    PDms_hb = PD_ms // 25.6
    PDms_lb = int(round((PD_ms % 25.6) / 0.1))
    PDms_HB = PDms_hb.to_bytes(1, byteorder="little")
    PDms_LB = PDms_lb.to_bytes(1, byteorder="little")
    if PDms_hb > 0 or PDms_lb > 0:
        ser2.write(b'\x58')
        ser2.write(PDms_HB)
        ser2.write(PDms_LB)

def Pulse_Width_us_DAC1():
    PW_us = int(PWus_Input.get())
    PWus_hb = PW_us // 256
    PWus_lb = PW_us % 256
    PWus_HB = PWus_hb.to_bytes(1, byteorder="little")
    PWus_LB = PWus_lb.to_bytes(1, byteorder="little")
    if PWus_hb > 0 or PWus_lb > 0:
        ser1.write(b'\x39')
        ser1.write(PWus_HB)
        ser1.write(PWus_LB)

def Pulse_Width_us_DAC2():
    PW_us2 = int(PWus_Input2.get())
    PWus_hb2 = PW_us2 // 256
    PWus_lb2 = PW_us2 % 256
    PWus_HB2 = PWus_hb2.to_bytes(1, byteorder="little")
    PWus_LB2 = PWus_lb2.to_bytes(1, byteorder="little")
    if PWus_hb2 > 0 or PWus_lb2 > 0:
        ser2.write(b'\x39')
        ser2.write(PWus_HB2)
        ser2.write(PWus_LB2)

def Send_DAC1():
    Pulses_DAC1()
    Pulse_Delay_ms_DAC1()
    Pulse_Width_us_DAC1()
    Output_Voltage_DAC1()

def Send_DAC2():
    Pulses_DAC2()
    Pulse_Delay_ms_DAC2()
    Pulse_Width_us_DAC2()
    Output_Voltage_DAC2()

def Send_DACS():
    Send_DAC1()
    Send_DAC2()

# Define GUI elements

Blink = Button(window, text="Blink", bg="white", fg="black", width=22, height=2, command=blink)
Blink.grid(column=1, row=0)

LED_OFF = Button(window, text="Screen Off", bg="white", fg="black", width=22, height=2, command=LEDOFF)
LED_OFF.grid(column=0, row=1)

LED_ON = Button(window, text="Screen On", bg="white", fg="black", width=22, height=2, command=LEDON)
LED_ON.grid(column=1, row=1)

Set_Count_0 = Button(window, text="Reset Count", bg="white", fg="black", width=22, height=2, font="bold", command=Train_Reset)
Set_Count_0.grid(column=0, row=10)

Start_DAC1 = Button(window, text="Start DAC1", bg="green", fg="black", font="bold", width=22, height=2, command=Start_stim_DAC1)
Start_DAC1.grid(column=1, row=12)

Start_DAC2 = Button(window, text="Start DAC2", bg="green", fg="black", font="bold", width=22, height=2, command=Start_stim_DAC2)
Start_DAC2.grid(column=2, row=12)

Start_DAC1_2 = Button(window, text="Start DACS", bg="green", fg="black", font="bold", width=22, height=2, command=Start_stim_DAC1_2)

Start_DAC1_2 = Button(window, text="Start DACS", bg="green", fg="black", font="bold", width=22, height=2, command=Start_stim_DAC1_2)
Start_DAC1_2.grid(column=3, row=12)

Stop_Train = Button(window, text="STOP", bg="red", fg="black", font="bold", width=22, height=2, command=stop)
Stop_Train.grid(column=0, row=12)

Close_Port = Button(window, text="Close Port", bg="white", fg="black", width=22, height=2, command=Close_COM)
Close_Port.grid(column=0, row=0)

Bipolar_En = Button(window, text="Bipolar (OpAmp Enabled)", bg="white", fg="black", width=22, height=2, command=Bipolar)
Bipolar_En.grid(column=3, row=3)

Mono_EN = Button(window, text="Monopolar (OpAmp Enabled)", bg="white", fg="black", width=22, height=2, command=MonopolarEN)
Mono_EN.grid(column=3, row=5)

Set_Message = Label(window, text="Settings", bg="white", fg="black", width=22, height=2)
Set_Message.grid(column=0, row=2)

Input_User_Message1 = Label(window, text="DAC 1 Values", bg="white", fg="black", width=22, height=2)
Input_User_Message1.grid(column=1, row=2)

Input_User_Message2 = Label(window, text="DAC 2 Values", bg="white", fg="black", width=22, height=2)
Input_User_Message2.grid(column=2, row=2)

P_Message = Label(window, text="Number of Pulses (1-9999)", bg="white", fg="black", width=22, height=2)
P_Message.grid(column=0, row=4)

P_Input = Entry(window, bg="white", fg="black", font="bold")
P_Input.insert(0, "30")
P_Input.grid(column=1, row=4)

P_Input2 = Entry(window, bg="white", fg="black", font="bold")
P_Input2.insert(0, "30")
P_Input2.grid(column=2, row=4)

OV_Message = Label(window, text="Output Voltage (mV)", bg="white", fg="black", width=22, height=2)
OV_Message.grid(column=0, row=9)

OV_Input = Entry(window, bg="white", fg="black", font="bold")
OV_Input.insert(0, "0")
OV_Input.grid(column=1, row=9)

OV_Input2 = Entry(window, bg="white", fg="black", font="bold")
OV_Input2.insert(0, "0")
OV_Input2.grid(column=2, row=9)

PP_Message = Label(window, text="Pulse Delay (ms)", bg="white", fg="black", width=22, height=2)
PP_Message.grid(column=0, row=5)

PP_Input = Entry(window, bg="white", fg="black", width=20, font="bold")
PP_Input.insert(0, "17")
PP_Input.grid(column=1, row=5)

PP_Input2 = Entry(window, bg="white", fg="black", width=20, font="bold")
PP_Input2.insert(0, "17")
PP_Input2.grid(column=2, row=5)

PWus_Message = Label(window, text="Pulse Width (μs)", bg="white", fg="black", width=22, height=2)
PWus_Message.grid(column=0, row=8)

PWus_Input = Entry(window, bg="white", fg="black", font="bold")
PWus_Input.insert(0, "100")
PWus_Input.grid(column=1, row=8)

PWus_Input2 = Entry(window, bg="white", fg="black", font="bold")
PWus_Input2.insert(0, "100")
PWus_Input2.grid(column=2, row=8)

Send_DAC1_button = Button(window, text="Set DAC1", bg="white", fg="black", font="bold", width=22, height=2, command=Send_DAC1)
Send_DAC1_button.grid(column=1, row=10)

Send_DAC2_button = Button(window, text="Set DAC2", bg="white", fg="black", font="bold", width=22, height=2, command=Send_DAC2)
Send_DAC2_button.grid(column=2, row=10)

Send_DACS_button = Button(window, text="Set DAC1 & DAC2", bg="white", fg="black", font="bold", width=22, height=2, command=Send_DACS)
Send_DACS_button.grid(column=3, row=10)

# Loop window
window.mainloop()
