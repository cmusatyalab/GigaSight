package cmu.servercommunication;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Random;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ArrayBlockingQueue;

import android.content.Context;
import android.os.Environment;
import android.util.Log;
import cmu.capture.GPSstream;
import cmu.capture.MP4Stream;
import cmu.capture.Segment;
import cmu.capture.Stream;
import cmu.capture.Segment.Type;
import cmu.gigasight.GigasightActivity;

public class ChunkUploader implements Runnable {

	private static final int STATUS_OK = 0;
	private static final int STATUS_ERROR = 1;
	private static final String TAG = "Chunkploader";
	private static final int BUFFER_SIZE = 10240;
	
	private boolean isRunning = false;
	private ArrayBlockingQueue<File> uploadQ;
	private Timer timer;
	private int chunkSize;
	private int startDelay;
	private File stopFile;
	private Socket uploadSocket;
	private DataInputStream in;
	private DataOutputStream out;
	private Context ctxt;
	private GigasightActivity act;
	private File videoDir;
	
	public ChunkUploader(GigasightActivity act, int chunkSize, String resolution) {
		this.ctxt = act.getApplicationContext();
		this.act = act;
		uploadQ = new ArrayBlockingQueue<File>(300);
		timer = new Timer();
		this.chunkSize = chunkSize;
		this.stopFile = new File("STOPFILE");
		if (ServerSettings.uploadType == ServerSettings.SLOTTEDUPLOAD) {
			startDelay = (int) (ServerSettings.devNo * (chunkSize * 1000.0 / ServerSettings.totalDev));			
		}
		else{
			startDelay = new Random(System.currentTimeMillis()).nextInt(chunkSize*1000-500);
			
		}
		// todo: set videoDir
		
		videoDir = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES)+ File.separator + resolution+"_"+chunkSize+"s");
		act.showMessage("Looking for video in "+videoDir);
	}

	public void run() {
		isRunning = true;
		UploadTask upTask = new UploadTask();		
		if (ServerSettings.uploadType == ServerSettings.SLOTTEDUPLOAD) {
			act.showMessage("Starting slotted upload with offset "+startDelay);
		}
		else{
			act.showMessage("Starting random upload with start delay "+startDelay);
		}		
		timer.scheduleAtFixedRate(upTask, this.startDelay,this.chunkSize * 1000);

		while (isRunning) {

			File f;
			try {
				f = uploadQ.take();
				act.showMessage("Registering "+f.getName());
				// will block until a file is available
				if (f == this.stopFile) {
					act.showMessage("Upload stopped");
					isRunning = false;
					break;
				}
				// register segment and stream
				Segment seg = new Segment(Type.RECORDED);
				MP4Stream ms = new MP4Stream(f);
				seg.addStream(ms);
				RESTClient.post(seg, false);
				RESTClient.post((MP4Stream) seg.getStream(Stream.Container.MP4),false);
				// hack: we do a new put, should not be necessary because all
				// information is already available in the post
				RESTClient.put((MP4Stream) seg.getStream(Stream.Container.MP4),false);

				//now start the upload
				act.showMessage("Uploading "+f.getName());
				uploadSocket = new Socket(ServerSettings.serverIP,Integer.parseInt(ServerSettings.uploadPort));
				in = new DataInputStream(uploadSocket.getInputStream());
				out = new DataOutputStream(uploadSocket.getOutputStream());

				// send length of ID and ID (e.g. /segment/200/400)
				int length = ms.getServerLocation().getBytes().length;
				out.writeInt(length);
				out.write(ms.getServerLocation().getBytes());
				out.flush();
				
				// wait for OK from server before proceeding
				if (!serverOK()) {
					Log.i(TAG, "Aborting upload after error from server");
					isRunning = false;
					break;
				}

				// send fileSize
				int fileSize = ms.getFileSize();
				out.writeInt(fileSize);
				out.flush();

				// wait for OK
				if (!serverOK()) {
					Log.i(TAG, "Aborting upload after error from server");
					isRunning = false;
					break;
				}
				
				// now do the actual upload
				BufferedInputStream bin = new BufferedInputStream(
						new FileInputStream(ms.getFile()));
				byte[] buffer = new byte[BUFFER_SIZE];
				int bytes_read = 0;
				
				while ((bytes_read = bin.read(buffer)) != -1) {
					if (bytes_read > 0) {
						out.write(buffer, 0, bytes_read);
						out.flush();					
					}
				}
				bin.close();

				// wait for ok
				if (!serverOK()) {
					isRunning = false;
					break;
				}
				
				Log.d(TAG,"Upload finished of file "+ms.getFile().getName()+" to "+ms.getServerLocation());		
				
				//close socket
				in.close();
				out.close();
				uploadSocket.close();
				
			} catch (InterruptedException e) {

				e.printStackTrace();
				isRunning = false;
				break;
			} catch (NumberFormatException e) {
				e.printStackTrace();
				isRunning = false;
				break;
			} catch (UnknownHostException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				isRunning = false;
				break;
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				isRunning = false;
				break;
			}

		}
		cleanUp();
		act.showMessage("Ready");
	}

	public void stop() {
		isRunning = false;
		act.showMessage("Will stop after current upload");
		uploadQ.clear();
		try {
			uploadQ.put(stopFile);
		} catch (InterruptedException e) {			
			e.printStackTrace();
		}
		cleanUp();
	}

	private void cleanUp() {
		if (timer != null) {
			timer.cancel();
		}
		
			try {
				if (in != null) {
				in.close();
				}
				if (out != null) {
					out.close();
				}
				if (uploadSocket != null){
					uploadSocket.close();
				}
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		
		
	}

	public boolean isRunning() {
		return isRunning;
	}

	private class UploadTask extends TimerTask {
		private File[] listFiles;
		private int counter;

		UploadTask() {
			listFiles = videoDir.listFiles();
			counter = 0;
			act.showMessage("Will upload "+listFiles.length + "files ");
		}

		public void run() {
			try {
				act.showMessage("File "+listFiles[counter]+ " queued for upload");
				uploadQ.put(listFiles[counter]);
				counter++;				
				if (counter == listFiles.length) {
					uploadQ.put(stopFile);
					timer.cancel();
				}
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}
	
	private boolean serverOK() {
		int reply;
		try {
			reply = in.readInt();
			if (reply == STATUS_ERROR)
				return false;
			else if (reply == STATUS_OK)
				return true;
		} catch (IOException e) {
			Log.e(TAG, ""+e.getMessage());
			return false;
		}
		return false;
	}
}
