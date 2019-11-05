# TODO
This is by no means a complete TODO list
- assigning a Sensor its own ID does nothing, remove the ID from the logic and the config file
- change the config file so that it has one variable for each connection quality, like so:
    RemoteIP:IPGoesHere
    HostName:NameGoesHere
    PortNumber:NumberGoesHere   
- document the config file    
- change the method by which the board scans the response for a polling rate

# scope creep features
- watchdog timere
- saving time, persistence of polling rate after changing (saving to file)
- add error logging (logging in general)
