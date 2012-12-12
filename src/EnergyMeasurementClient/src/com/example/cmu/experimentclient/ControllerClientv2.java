package com.example.cmu.experimentclient;

import java.io.File;
import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.ClosedChannelException;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.nio.channels.spi.SelectorProvider;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class ControllerClientv2 implements Runnable {

	private SocketChannel expControllerChannel = null;
	private Context ctxt;
	private Handler mHandler;
	private String controllerIP;
	private int controllerPort;
	private final static String TAG = "ControllerClient";

	private Selector selector;
	private List pendingChanges = new LinkedList();
	private ByteBuffer readBuffer = ByteBuffer.allocate(200);
	// Maps a SocketChannel to a list of ByteBuffer instances
	private Map pendingData = new HashMap();

	private boolean isRunning;
	/* data fields and experiment configuration */
	private String nodeName = null;
	private String uploadIP = null;
	private int uploadPort = -1;
	private int chunkSec = 5;
	private FileReceiver fr = null;
	private FileUploader fu = null;
	private File videoStorageDir;
	private boolean loopUpload = false;
	private boolean startRandomChunk = false;
	private boolean startRandomInterval = false;
	private boolean startFixedDelay = true;
	private int startDelay = -1;
	public ControllerClientv2(Context ctxt, Handler mHandler) {
		this.mHandler = mHandler;
		this.ctxt = ctxt;
		this.videoStorageDir = new File(ctxt.getExternalFilesDir(null)
				+ File.separator + "GSExperiment");
		if (!this.videoStorageDir.exists()) {
			this.videoStorageDir.mkdirs();
		}
	}

	private boolean handleRequest() {
		Log.d(TAG, "handleRequest");
		Log.d(TAG,
				"bytes in buffer before handling request: "
						+ (readBuffer.limit() - readBuffer.position()));
		if (readBuffer.limit() - readBuffer.position() == 0) { // this should
																// never
																// happen...
			Log.w(TAG, "Empty readbuffer!");
			return false;
		}

		// read in opcode
		byte opcode = readBuffer.get();
		Log.d(TAG, "opcode: " + opcode);
		boolean result = true;
		switch (opcode) {
		case ControllerProtocol.SET_NAME:
			result = setName();
			break;

		case ControllerProtocol.SET_UPLOAD_IP: {
			result = setUploadIP();
			break;
		}
		case ControllerProtocol.SET_UPLOAD_PORT: {
			result = setUploadPort();
			break;
		}
		case ControllerProtocol.OPEN_RECEIVE_PORT: {
			result = openReceivePort();
			break;
		}
		case ControllerProtocol.CLOSE_RECEIVE_PORT: {
			result = closeReceivePort();
			break;
		}
		case ControllerProtocol.DELETE_FILES: {
			result = deleteFiles();
			break;
		}
		case ControllerProtocol.START_EXPERIMENT: {
			result = startExperiment();
			break;
		}
		case ControllerProtocol.STOP_EXPERIMENT: {
			result = stopExperiment();
			break;
		}
		case ControllerProtocol.SET_CHUNKINTERVAL: {
			result = setChunkInterval();
			break;
		}
		case ControllerProtocol.SET_STARTDELAY: {
			result = setStartDelay();
			break;
		}
		case ControllerProtocol.SET_LOOPUPLOAD: {
			result = setLoopUpload();
			break;
		}
		case ControllerProtocol.START_RANDOM_CHUNK: {
			result = startRandomChunk();
			break;
		}
		case ControllerProtocol.START_RANDOM_INTERVAL: {
			result = startRandomInterval();
			break;
		}
		
		default:
			Log.w(TAG, "Unknown opcode!");
			sendMessage(MainActivity.UPDATE_STATUS, "Unknown opcode received!"
					+ opcode);
			result = false;
		}

		return result;

	}

	public void run() {
		isRunning = true;
		readBuffer.clear();

		if (!init())
			isRunning = false;

		while (isRunning) {
			try {
				// Process any pending changes
				synchronized (this.pendingChanges) {
					Iterator changes = this.pendingChanges.iterator();
					while (changes.hasNext()) {
						ChangeRequest change = (ChangeRequest) changes.next();
						switch (change.type) {
						case ChangeRequest.CHANGEOPS:
							SelectionKey key = change.socket
									.keyFor(this.selector);
							key.interestOps(change.ops);
							break;
						case ChangeRequest.REGISTER:
							change.socket.register(this.selector, change.ops);
							break;
						}
					}
					this.pendingChanges.clear();
				}
					// Wait for an event one of the registered channels
					this.selector.select();

					// Iterate over the set of keys for which events are
					// available
					Iterator<SelectionKey> selectedKeys = this.selector
							.selectedKeys().iterator();
					while (selectedKeys.hasNext()) {
						SelectionKey key = (SelectionKey) selectedKeys.next();
						selectedKeys.remove();

						if (!key.isValid()) {
							Log.d(TAG, "Invalid key!");
							continue;
						}

						// Check what event is available and deal with it
						if (key.isReadable()) {
							this.read(key);
						} else if (key.isWritable()) {
							this.write(key);
						}
					}
				
			} catch (IOException e) {
				Log.w(TAG, e.getMessage());
				sendMessage(MainActivity.UPDATE_STATUS, e.getMessage());
				isRunning = false;
			}
		}

		cleanUp();

	}

	public void setServer(String serverIP, int serverPort) {
		this.controllerIP = serverIP;
		this.controllerPort = serverPort;
	}

	public boolean init() {
		try {
			this.selector = SelectorProvider.provider().openSelector();
		} catch (IOException e) {
			Log.w(TAG, e.getMessage());
			cleanUp();
			isRunning = false;
			return false;
		}

		try {
			expControllerChannel = SocketChannel.open();
			expControllerChannel.configureBlocking(false);
			// kick of connection establishment
			expControllerChannel.connect(new InetSocketAddress(controllerIP,
					controllerPort));

			// loop until connected
			int noRetries = 0;
			while (!expControllerChannel.finishConnect() && noRetries < 10) {
				sendMessage(MainActivity.UPDATE_STATUS, "Trying to connect...");
				try {
					Log.d(TAG, "Waiting until finishConnect returns true..");
					Thread.sleep(500);
					noRetries++;
				} catch (InterruptedException e) {
					Log.w(TAG, e.getMessage());
					cleanUp();
					isRunning = false;
					return false;
				}
			}

			if (noRetries == 10) {
				sendMessage(MainActivity.UPDATE_STATUS,
						"Could not connect, wrong IP?");
				cleanUp();
				isRunning = false;
				return false;
			}

			Log.d(TAG, "Connected to ExpController");
			sendMessage(MainActivity.UPDATE_STATUS,
					"Connected to ExpController");

			// register an interest in reading from this channel
			try {
				expControllerChannel.register(this.selector,
						SelectionKey.OP_READ);
			} catch (ClosedChannelException e) {
				Log.w(TAG, e.getMessage());
				isRunning = false;
				return false;
			}

			requestParameters();

		} catch (IOException e) {
			Log.w(TAG, e.getMessage());
			sendMessage(MainActivity.UPDATE_STATUS, e.getMessage());
			return false;
		}

		return true;
	}

	private void cleanUp() {
		try {
			if (fr != null) {
				Log.d(TAG, "Stopping fileReceiver thread");
				sendMessage(MainActivity.UPDATE_STATUS, "Receive port "
						+ fr.listenPort + " closed");
				fr.stop();
				fr = null;
			}
			if (fu != null) {
				Log.d(TAG, "Stopping fileUpload thread");
				fu.stop();
				fu = null;
			}

			if (expControllerChannel != null && expControllerChannel.isOpen())
				expControllerChannel.close();

			if (selector.isOpen())
				selector.close();
		} catch (IOException e) {
			Log.w(TAG, e.getMessage());
		}
	}

	private void read(SelectionKey key) throws IOException {
		SocketChannel socketChannel = (SocketChannel) key.channel();

		if (socketChannel != expControllerChannel) {
			Log.w(TAG,
					"DATA received on different channel than expControllerChannel!");
		}

		// clear out read buffer
		// this.readBuffer.clear();

		// Attempt to read off the channel
		int numRead;
		try {
			numRead = socketChannel.read(this.readBuffer);
		} catch (IOException e) {
			// The remote forcibly closed the connection, cancel
			// the selection key and close the channel.
			// if (knownMobile)
			// onDisconnect(socketChannel);
			sendMessage(MainActivity.DISCONNECTED,
					"Server forcibly closed connection");
			Log.w(TAG, "Server forcibly closed connection");
			key.cancel();
			socketChannel.close();
			cleanUp();
			isRunning = false;
			return;
		}

		if (numRead == 0) {
			Log.w(TAG, "read 0 although socket is readable...");
			return;
		}

		if (numRead == -1) {
			// Remote entity shut the socket down cleanly. Do the
			// same from our end and cancel the channel.
			// if (knownMobile)
			sendMessage(MainActivity.DISCONNECTED, "Server closed connection");
			Log.w(TAG, "Server closed connection");
			key.channel().close();
			key.cancel();
			isRunning = false;
			cleanUp();
			return;
		}

		// keep handling requests as long as there is enough data
		// note that the first byte in the buffer will always be an opcode!
		// readBuffer.flip(); //set readBuffer to read state, position = 0
		// while(readBuffer.remaining() > 0){ //note that buffer is in read
		// state, or we escaped from the while loop
		// if(handleRequest()){
		// readBuffer.compact(); //clear a complete request from the buffer
		// readBuffer.flip(); //set position = 0
		// } else{ //the data was not from a complete request, so set the
		// position to make sure bytes are not overwritten
		// readBuffer.position(0);
		// readBuffer.compact(); //to set the position just after the last byte,
		// and limit to capacity
		// break;
		// }
		// }
		readBuffer.flip(); // set buffer to read state, position = 0
		while (readBuffer.remaining() > 0) {
			readBuffer.mark(); // mark the beginning of the next opcode
			if (!handleRequest()) {
				readBuffer.reset(); // request was not completely in buffer, so
									// reset position to beginning of request
				break;
			}
		}
		readBuffer.compact(); // clear all bytes of completed requests

	}

	private boolean setName() {
		int nameLength = 0;
		if (readBuffer.remaining() >= 4) {
			nameLength = readBuffer.getInt();
		} else {
			return false; // we do not have enough data
		}

		if (readBuffer.remaining() >= nameLength) {
			byte[] bName = new byte[nameLength];
			readBuffer.get(bName);
			nodeName = new String(bName);
			Log.d(TAG, "Received name: " + nodeName);
			sendMessage(MainActivity.SET_NODENAME, nodeName);
			sendMessage(MainActivity.UPDATE_STATUS, "Node name configured");
		} else {

			return false;
		}

		return true;
	}

	private boolean setUploadIP() {
		int ipLength = 0;
		if (readBuffer.remaining() >= 4) {
			ipLength = readBuffer.getInt();
			Log.d(TAG, "iplength: " + ipLength);

		} else {
			return false; // we do not have enough data
		}

		if (readBuffer.remaining() >= ipLength) {
			byte[] bIP = new byte[ipLength];
			readBuffer.get(bIP);
			uploadIP = new String(bIP);
			Log.d(TAG, "Received IP: " + uploadIP);
			sendMessage(MainActivity.SET_UPLOAD_IP, uploadIP);
			sendMessage(MainActivity.UPDATE_STATUS, "Upload IP configured");
		} else {
			return false;
		}

		return true;
	}

	private boolean setUploadPort() {
		if (readBuffer.remaining() >= 4) {
			uploadPort = readBuffer.getInt();
			Log.d(TAG, "Upload port: " + uploadPort);
			sendMessage(MainActivity.SET_UPLOAD_PORT, "" + uploadPort);
			sendMessage(MainActivity.UPDATE_STATUS, "Upload port configured");
		} else {
			return false; // we do not have enough data
		}

		if (isInitialized()) {
			sendMessage(MainActivity.INIT_COMPLETE, "Initialization completed");
		}
		return true;
	}
	
	private boolean setLoopUpload(){
		if (readBuffer.remaining() >= 4) {
			this.loopUpload = (readBuffer.getInt() > 0);
			if(this.loopUpload){
				Log.d(TAG,"Looping over files");
				sendMessage(MainActivity.UPDATE_STATUS,"Looping over files");
			}
			else{
				Log.d(TAG,"Only upload video once");
				sendMessage(MainActivity.UPDATE_STATUS,"Only upload video once");
			}
			return true;
		} else //not enough data
			return false;
	}
	
	private boolean setStartDelay(){
		if (readBuffer.remaining() >= 4) {
			this.startFixedDelay = true;
			this.startDelay = readBuffer.getInt();
			sendMessage(MainActivity.UPDATE_STATUS,"Start delay set to: "+this.startDelay);
			return true;
		} else //not enough data
			return false;
	}

	private boolean openReceivePort() {
		// the ExperimentManager wants us to send files for upload experiments
		if (fr != null) {
			fr.stop();
		}
		fr = new FileReceiver(this.videoStorageDir);
		int listenPort = fr.init();
		if (listenPort > 0) {
			new Thread(fr).start();
			ByteBuffer b = ByteBuffer.allocate(1 + 4);
			b.put(ControllerProtocol.RECEIVE_PORT_OPENED);
			b.putInt(listenPort);
			this.send(expControllerChannel, b.array());
			Log.d(TAG, "Receive port " + listenPort + " opened");
			sendMessage(MainActivity.UPDATE_STATUS, "Receive port "
					+ listenPort + " opened ");
		} else {
			Log.d(TAG, "Could not open receive port");
			sendMessage(MainActivity.UPDATE_STATUS,
					"Could not open receive port");
		}
		return true; // we handled all readable data
	}

	private boolean closeReceivePort() {
		fr.stop();
		byte[] data = { ControllerProtocol.RECEIVE_PORT_CLOSED };
		this.send(expControllerChannel, data);
		sendMessage(MainActivity.UPDATE_STATUS, "Receive port " + fr.listenPort
				+ " closed ");
		fr = null;
		return true;

	}

	private boolean startRandomChunk(){
		this.startRandomChunk = true;
		sendMessage(MainActivity.UPDATE_STATUS,"Will start at random chunk in video stream");
		return true;
	}
	
	private boolean startRandomInterval(){
		this.startRandomInterval = true;
		sendMessage(MainActivity.UPDATE_STATUS,"Will start with random offset");
		return true;
	}
	private boolean startExperiment() {
		// the ExperimentManager wants us to send files for upload experiments
		if (fu != null) {
			fu.stop();
		}
		
		if (this.startRandomInterval){
			fu = new RandomFileUploader(this,this.uploadIP,this.uploadPort,this.chunkSec,this.videoStorageDir,this.loopUpload,this.startRandomChunk);
		}
		else if (this.startFixedDelay){
			fu = new SlottedFileUploader(this,this.uploadIP,this.uploadPort,this.chunkSec,this.videoStorageDir,this.loopUpload,this.startRandomChunk,this.startDelay);
		}
		else {
			fu = new SynchroFileUploader(this,this.uploadIP,this.uploadPort,this.chunkSec,this.videoStorageDir,this.loopUpload,this.startRandomChunk);
		}
		new  Thread(fu).start();
		sendMessage(MainActivity.UPDATE_STATUS,"EXPERIMENT STARTED");
		return true;
	}
	
	public void notifyUploadFinished(){
		Log.d(TAG,"NotifyUploadFinished");
		ByteBuffer b = ByteBuffer.allocate(1);
		b.put(ControllerProtocol.UPLOAD_FINISHED);
		this.send(expControllerChannel, b.array());
		return;
	}

	private boolean stopExperiment(){
		if( fr!= null){
			fr.stop();
		}
		fr = null;
		
		if (fu!= null){
			fu.stop();
		}
		fu = null;
		sendMessage(MainActivity.UPDATE_STATUS,"EXPERIMENT STOPPED");
		return true;
	}
	
	private boolean setChunkInterval() {
		
		if (readBuffer.remaining() >= 4) {
			this.chunkSec = readBuffer.getInt();
			Log.d(TAG, "chunk sec: "+this.chunkSec);
			sendMessage(MainActivity.UPDATE_STATUS,"Chunk size: "+chunkSec);
			return true;
		} else {
			return false; // we do not have enough data
		}
	}
	private boolean deleteFiles() {
		Log.d(TAG,"Deleting files");
		sendMessage(MainActivity.UPDATE_STATUS,"Deleting files of previous experiment");
		File[] list = videoStorageDir.listFiles();
		for (File f : list) {
			f.delete();
		}
		return true;
	}

	public void sendMessage(int code, String message) {
		Message msg = mHandler.obtainMessage(code);
		msg.what = code;
		msg.obj = message;
		mHandler.sendMessage(msg);
	}

	public void stop() {
		isRunning = false;
	}

	public boolean isRunning() {
		return isRunning;
	}

	private int requestParameters() {
		byte[] data = new byte[3];
		data[0] = ControllerProtocol.REQUEST_NAME;
		data[1] = ControllerProtocol.REQUEST_UPLOAD_IP;
		data[2] = ControllerProtocol.REQUEST_UPLOAD_PORT;

		this.send(this.expControllerChannel, data);
		Log.d(TAG, "Parameter requests sent");
		sendMessage(MainActivity.UPDATE_STATUS, "Parameter requests sent");
		return 1;

	}

	public void send(SocketChannel socket, byte[] data) {
		//Log.d(TAG,"send "+data.length);
		synchronized (this.pendingChanges) {
			// Indicate we want the interest ops set changed
			//Log.d(TAG,"Adding new change request");
			this.pendingChanges.add(new ChangeRequest(socket,
					ChangeRequest.CHANGEOPS, SelectionKey.OP_WRITE));

			// And queue the data we want written
			synchronized (this.pendingData) {
				//Log.d(TAG,"queue data");
				List queue = (List) this.pendingData.get(socket);
				if (queue == null) {
					queue = new ArrayList();
					this.pendingData.put(socket, queue);
				}
				Log.d(TAG, "Bytes queued for writing: " + data.length);
				queue.add(ByteBuffer.wrap(data));
			}
		}

		// Finally, wake up our selecting thread so it can make the required
		// changes
		this.selector.wakeup();
	}

	private void write(SelectionKey key) throws IOException {
		SocketChannel socketChannel = (SocketChannel) key.channel();

		synchronized (this.pendingData) {
			List queue = (List) this.pendingData.get(socketChannel);

			// Write until there's not more data ...
			while (!queue.isEmpty()) {
				ByteBuffer buf = (ByteBuffer) queue.get(0);
				//Log.d(TAG,"Writing data to socket");
				socketChannel.write(buf);
				if (buf.remaining() > 0) {
					System.out.println("did not write all data!");
					// ... or the socket's buffer fills up
					break;
				}
				queue.remove(0);
			}

			if (queue.isEmpty()) {
				// We wrote away all data, so we're no longer interested
				// in writing on this socket. Switch back to waiting for
				// data.
				key.interestOps(SelectionKey.OP_READ);
			}
		}
	}

	private boolean isInitialized() {
		return (nodeName != null) && (uploadIP != null) && (uploadPort > 0);
	}

}