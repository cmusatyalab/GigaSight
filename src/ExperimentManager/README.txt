How to perform the video upload experiments.

1) Make the set-up
- install the "ExperimentClient" on each of the phones
- make your own private 802.11n setup
- add one phone to the Monsoon Power Monitor
- create user/password: cloudlet/gigasight on each machine
(this is needed because the password is hardcoded in the ExperimentManager)

2) Make sure the configuration fileis set correctly
The file is in src/ExperimentConfiguration.src

The most important settings are:
- baseDir: the directory where the chunked video segments can be found
- DEFAULT_VIDEO_UPLOAD_IP: the IP of the server where the Python upload server is running
- REFERENCE_NODE: this is the IP of the node to which the Monsoon is attached
- ENERGYMON_IP: this is the IP of the Windows machine running the PowerTool of the Monsoon

3) install tshark (with apt-get) on the upload server
- you have to create a directory "/tmptshark" on that machine as well, make sure
it has root permissions

3) Set up SSH keys
- you should be able to log-in with your public/private key on both the machine with the Python UploadServer and the Windows machine
- if you need an SSH server on the Windows machine for the energy measurements, just use Bitvise SSH server
- change the path in ExperimentManager.java to your SSH key path
(do a search for the line "jsch.addIdentity")

4) Upload the videos for the experiments
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
