# TODO
This is by no means a complete TODO list
- Think of a way to address the failure to open a log file (OfflineLogging.cpp)
- write a test suite for the offline logging functions
- test everything
- simplify the logic in main.cpp
- make a simple file header for all files in this project
- in the initialization fucntion for boardspecs, add in a parameter to pass in teh wifi access point so that you can set the wifi ssid and password without storing them twice (in boardspecs and the wifiInterface class)
- Add the remote pollling rate, or make some code to deal with that
# Planning

# notes
The get bulk data get request is probably not going to work. It will most likely need to be changed to something like `GET / HTTP/1.1\r\nHost: hostname.com\r\nConnection: close\r\n`  or something in that format

# scope creep features
- watchdog timere
- saving time, persistence of polling rate after changing (saving to file)
