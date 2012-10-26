package cmu.servercommunication;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

import cmu.capture.FileStream;
import cmu.capture.MP4Stream;

import android.util.Log;


public class FileUploader implements Runnable {

	private static final String TAG = "FileUploader";
	private static final int STATUS_OK = 0;
	private static final int STATUS_ERROR = 1;
	private static final int BUFFER_SIZE = 10240;

	private BlockingQueue<FileStream> queue;
	private MP4Stream STOP;
	private Socket uploadSocket;
	private DataInputStream in;
	private DataOutputStream out;

	public FileUploader() {
		this.queue = new LinkedBlockingQueue<FileStream>();
		STOP = new MP4Stream(new File("STOP"));

	}

	public void start() {
		new Thread(this).start();
	}

	public void upload(FileStream s) {
		if(s.isRegistered()){
			queue.add(s);
		}
		else{
			Log.w(TAG,"Could not upload "+s.getFile().getName()+" because it is not registered on the server");
		}
	}

	public void stop() {
		queue.add(STOP);
	}

	public void run() {		

		while (true) {
			try {
				FileStream element = queue.take();
				if (element == STOP) {
					Log.d(TAG, "End of fileUpload thread");
					break;
				} else {
					Log.d(TAG, "Upload " + element.getServerLocation());
					send(element);
					element.setUploaded();					
				}
			} catch (InterruptedException e) {
				e.printStackTrace();				
			}
		}
		
	}

	private void send(FileStream s) {
		try {
			Log.d(TAG,"Sending upload to server "+ServerSettings.serverIP+":"+ServerSettings.uploadPort);
			uploadSocket = new Socket(ServerSettings.serverIP,
					Integer.parseInt(ServerSettings.uploadPort));
			in = new DataInputStream(uploadSocket.getInputStream());
			out = new DataOutputStream(uploadSocket.getOutputStream());

			// send length of ID and ID (e.g. /segment/200/400)
			int length = s.getServerLocation().getBytes().length;
			out.writeInt(length);
			out.write(s.getServerLocation().getBytes());
			out.flush();

			// wait for OK from server before proceeding
			if (!serverOK()) {
				Log.i(TAG, "Aborting upload after error from server");
				cleanUp();
				return;
			}

			// send fileSize
			int fileSize = s.getFileSize();
			out.writeInt(fileSize);
			out.flush();

			// wait for OK
			if (!serverOK()) {
				Log.i(TAG, "Aborting upload after error from server");
				cleanUp();
				return;
			}

			// now do the actual upload
			BufferedInputStream bin = new BufferedInputStream(
					new FileInputStream(s.getFile()));
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
				cleanUp();
				return;
			}
			
			Log.d(TAG,"Upload finished of file "+s.getFile().getName()+" to "+s.getServerLocation());
		} catch (NumberFormatException e) {
			
			Log.e(TAG, e.getMessage());
		} catch (UnknownHostException e) {
			
			Log.e(TAG, e.getMessage());
		} catch (IOException e) {
			
			Log.e(TAG, e.getMessage());
		} finally {
			cleanUp();
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
			Log.e(TAG, e.getMessage());
			return false;
		}
		return false;
	}

	private void cleanUp() {
		try {
			if (out != null)
				out.close();
			if (in != null)
				in.close();
			if (uploadSocket != null)
				uploadSocket.close();
		} catch (IOException e) {
			Log.e(TAG, e.getMessage());
		}
	}
}
