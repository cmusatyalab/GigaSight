How to perform the video upload experiments.

1) Make the set-up
- install the "ExperimentClient" on each of the phones
- make your own private 802.11n setup
- connect one phone to the Monsoon Power Monitor + Windows PC with the related PowerTool installed
- configure the Monsoon to a voltage of 3.89 V for Google Nexus smartphone
- add one server to run the Python script for the upload
- create user/password: cloudlet/gigasight on the upload server, and on the Windows machine for the PowerTool 
(this is needed because the password is hardcoded in the ExperimentManager)
- add one laptop for the ExperimentManager

2) Make sure the configuration file is set correctly before you start the ExperimentManager
The file is in src/ExperimentConfiguration.java

The most important settings are:
- baseDir: the directory where the chunked video segments can be found
- DEFAULT_VIDEO_UPLOAD_IP: the IP of the server where the Python upload server is running
- REFERENCE_NODE: this is the IP of the phone to which the Monsoon is attached
- ENERGYMON_IP: this is the IP of the Windows machine running the PowerTool of the Monsoon

3) install tshark (with apt-get) on the upload server
- you have to create a directory "/tmptshark" on that machine as well, make sure
it has root permissions because tshark must run as root, and to write its log files it will
need the correct permissions

3) Set up SSH keys
- you should be able to log-in with your public/private key from the laptop with the ExperimentManager on both the machine with the Python UploadServer and the Windows machine
- if you need an SSH server on the Windows machine for the energy measurements, just use Bitvise SSH server
- change the path in ExperimentManager.java to your SSH key path (~/.ssh)
(do a search for the line "jsch.addIdentity")

4) Place the videos for the experiments in the appropriate directory on the laptop running the ExperimentManager
- they should be in "baseDir" (which you configured in ExperimentConfiguration.java)
- structure must be as follows:
baseDir/480p_3_Pittsburgh/5s
".."/30s
".."/300s
baseDir/480p_5_Pittsburgh/5s
".."/30s
".."/300s
baseDir/480p_7_Walnut/5s
".."/30s
".."/300s
baseDir/1080p_2_Pittsburgh/5s
".."/30s
".."/300s
baseDir/1080p_4_Pittsburgh/5s
".."/30s
".."/300s
basedir/1080p_6_Pittsburgh/5s
".."/30s
".."/300s

5) follow the "HOWTODOEXPERIMENTS.txt" for details on the
actual experiments
