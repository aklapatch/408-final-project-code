# TODO
This is by no means a complete TODO list
- Think of a way to address the failure to open a log file (OfflineLogging.cpp)
- write a test suite for the offline logging functions
- test everything
- Flash remaining wifi chips to latest firmware
- simplify the logic in main.cpp
- make a simple file header for all files in this project
- (maybe) for boardspecs struct, make the nework SSID and password a constant size (they have limits to length in standards) the SSID is 32 (+ \0 = 33) and the password is 64 including \0

# Planning

# notes
The get bulk data get request is probably not going to work. It will most likely need to be changed to something like `GET / HTTP/1.1\r\nHost: hostname.com\r\nConnection: close\r\n`  or something in that format
