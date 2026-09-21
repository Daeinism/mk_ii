# SCARA PC Client

This directory contains the PC-side TCP client. It uses only the Python standard
library, so no additional package installation is required.

From the project root, start the terminal with:

```powershell
python pc_client/scara_terminal.py
```

Press Enter to use the displayed default IP, or enter the current IP printed by
the ESP32 `wifiStatus` command. After connecting, enter SCARA commands directly:

```text
SCARA> setScaraAngles 20 -30
QUEUED
DONE
```

Enter `exit`, `quit`, or press Ctrl+C to close the client.

`scara_client.py` contains the reusable TCP layer intended for both this terminal
and a future Qt GUI.

## Supported commands

The TCP server uses the same parser and command queue as the USB console, so the
same commands are available through both interfaces.

```text
setScaraAngles <theta1> <theta2>
<theta1> <theta2>
home
hold
release
scaraPenUp
scaraPenDown
s <servo-angle>
fk <theta1> <theta2>
ik <x> <y> <arm-solution>
battery
wifiStatus
wifiScan
wifiConnect <network-index> <password>
wifiDisconnect
demoStart
demoStop
```

`arm-solution` is `0` for the right-arm solution and `1` for the left-arm
solution. The two-number form is a shortcut for `setScaraAngles`.

The server first returns `QUEUED` after accepting a command and later returns
`DONE` or `ERROR` after it executes. `DONE` for `demoStart` means that demo mode
started successfully; the demo continues looping until `demoStop`, `release`, a
movement failure, or a safety stop.

`wifiDisconnect` can close the TCP connection before its final result reaches
the client because it disconnects the network used by the command connection.

## Remembered Wi-Fi networks

After `wifiConnect` succeeds, the robot stores that network in non-volatile
storage. It remembers the two most recently connected SSIDs and their passwords.
On subsequent boots it scans for those networks and tries the most recently used
one first, followed by the second one if the first is unavailable or fails.

To provision two networks, connect to each one successfully once:

```text
wifiScan
wifiConnect <network-index> <password>
wifiDisconnect
wifiScan
wifiConnect <network-index> <password>
```

Use `-` in place of the password when connecting to an open network. A manual
`wifiDisconnect` disconnects the current session but does not erase remembered
credentials, so automatic connection remains available after the next reboot.
