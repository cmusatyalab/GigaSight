import java.io.BufferedWriter;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;

public class MobileNode {

	private static ExperimentManager expManager = ExperimentManager.getInstance();
	String ip;
	int controllerPort;
	int videoUploadPort;  //port of the personal VM
	String videoUploadIP; //IP of the personal VM
	int fileReceivePort;
	boolean connected;
	String name;

	private StatusPanel gui_panel; // graphical representation of the state of this object
	private MobileWorker mw;       // to communicate with the actual node

	public MobileNode(String IP, MobileWorker mw) {
		this.ip = IP;
		this.name = "Node_" + IP.substring(IP.lastIndexOf(".") + 1);
		this.mw = mw;
		fileReceivePort = -1;
		videoUploadPort = ExperimentConfiguration.DEFAULT_VIDEO_UPLOADPORT;
		videoUploadIP = ExperimentConfiguration.DEFAULT_VIDEO_UPLOADIP;
	}
	
	public void setGUIPanel(StatusPanel p){
		this.gui_panel = p;
	}
	
	public StatusPanel getGUIPanel(){
		return this.gui_panel;
	}
	
	public int getVideoUploadPort(){
		return this.videoUploadPort;
	}

	public String getVideoUploadIP(){
		return this.videoUploadIP;
	}
	
	private void sendOpcode(byte opcode){
		byte [] data = { opcode };
		mw.send(data);
	}
	
	public void startExperiment(){
		sendOpcode(ConfigurationProtocol.START_EXPERIMENT);
		this.gui_panel.setRunning();
	}
	
	public void stopExperiment(){
		sendOpcode(ConfigurationProtocol.STOP_EXPERIMENT);
		this.gui_panel.setStopped();
	}
	
	public void sendLoopUploads(boolean loopUpload){
		ByteBuffer data = ByteBuffer.allocate(5);
		data.put(ConfigurationProtocol.SET_LOOPUPLOAD);
		if(loopUpload)
			data.putInt(1);
		else
			data.putInt(0);
		mw.send(data.array());
	}
	
	public void sendStartRandomChunk(){
		this.sendOpcode(ConfigurationProtocol.START_RANDOM_CHUNK);
	}
	
	public void sendRandomStartInterval(){
		this.sendOpcode(ConfigurationProtocol.START_RANDOM_INTERVAL);
	}
	
	public void sendStartDelay(int delay){
		ByteBuffer data = ByteBuffer.allocate(5);
		data.put(ConfigurationProtocol.SET_STARTDELAY);
		data.putInt(delay);
		mw.send(data.array());
	}
	
	public void sendChunkSize(int chunk){
		ByteBuffer data = ByteBuffer.allocate(5);
		data.put(ConfigurationProtocol.SET_CHUNKINTERVAL);
		data.putInt(chunk);
		mw.send(data.array());
	}
	//blocking call
	public boolean sendFiles(String [] fileNames, boolean erase){
		gui_panel.setInitializing();
		
		if(fileNames.length == 0){
			gui_panel.setInitialized();
			return true;
		}
		
		if(erase){
			sendOpcode(ConfigurationProtocol.DELETE_FILES);
		}
		System.out.println("Sending files");
		//open receive port
		sendOpcode(ConfigurationProtocol.OPEN_RECEIVE_PORT);

		
		//wait on confirmation of client
		int noRetries = 0;
		while(fileReceivePort < 0 && noRetries < 10){
			try {
				Thread.sleep(100);
			} catch (InterruptedException e) {				
				e.printStackTrace();
			}
		}
		
	
		if(noRetries == 10){
			gui_panel.setConnected(this.ip);
			return false;
		}
		
		//send files one-by-one
		int noFiles = fileNames.length;
		for(int i = 0; i < noFiles; i++){
			File f = new File(fileNames[i]);
			String fileNoText = ("Sending "+i+"/"+noFiles+"\n");
			String fileSizeText = f.length()/1000000+" MB";

			FileInputStream fin = null;
			DataOutputStream dout = null;
			Socket sock = null;
			byte [] buffer = new byte[64*1024];
			try {
				sock = new Socket(ip,this.fileReceivePort);
				dout = new DataOutputStream(sock.getOutputStream());
				fin = new FileInputStream(f);				
				String fileName = f.getName();
				dout.writeInt(fileName.getBytes().length);
				dout.write(fileName.getBytes());
				dout.flush();
				
				//write actual file
				int bytes_read = 0;				
				int total_bytes = 0;
				while((bytes_read = fin.read(buffer)) != -1){
					dout.write(buffer, 0, bytes_read);
					dout.flush();
					total_bytes += bytes_read;
					gui_panel.setMessage("<html>"+fileNoText+"<br>"+total_bytes/1000000+"/"+fileSizeText+"</html>");
				}
				//System.out.println("Wrote "+total_bytes+" of file "+fileName+" with size "+f.length()+" to "+ip);
				
			} catch (IOException e) {
				e.printStackTrace();
			} finally{ //this is all really ugly code...
				try {
					fin.close();
					sock.close();
					dout.close();
				} catch (IOException e) {					
					e.printStackTrace();
				}
				
			}
			
		}
		
		//close receive port
		sendOpcode(ConfigurationProtocol.CLOSE_RECEIVE_PORT);
		gui_panel.setInitialized();
		return true;
	}
	
	public void connected(){
		connected = true;
		expManager.register(this); //this call will assign this.gui_panel 
		gui_panel.setConnected(this.ip);
	}
	
	public void configured(){
		gui_panel.setMessage("Configured");
	}
	
	public void disconnected() {
		this.ip = "";
		this.name = "";
		connected = false;
		fileReceivePort = -1;
		gui_panel.setDisconnected();
		expManager.deregister(this);
	}
	
	public void ready(){
		expManager.notifyFinished(this);
		gui_panel.setMessage("Exp finished");
	}

}
