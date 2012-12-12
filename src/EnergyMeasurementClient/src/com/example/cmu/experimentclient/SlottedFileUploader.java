package com.example.cmu.experimentclient;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.SocketChannel;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Random;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;


import android.text.format.Time;
import android.util.Log;

public class SlottedFileUploader implements FileUploader {

	private ControllerClientv2 controller;
	private BlockingQueue<File> uploadQ;
	private SocketChannel sockChannel;
	private String uploadIP;
	private int uploadPort;
	private boolean keepRunning;
	private boolean bReadBlocked;
	private File stopFile;
	private final static String TAG = "FileUploader";
	private int chunkSec;
	private File videoDir;
	private Timer timer;
	private boolean loopUpload;
	private int startDelay;
	private boolean startRandomChunk;
	public SlottedFileUploader(ControllerClientv2 controller, String uploadIP, int uploadPort, int chunkSec, File videoDir, boolean loopUpload, boolean startRandomChunk, int startDelay) {
		this.uploadIP = uploadIP;
		this.uploadPort = uploadPort;
		this.keepRunning = false;
		this.uploadQ = new ArrayBlockingQueue<File>(300);
		this.stopFile = new File("STOPFILE");
		this.chunkSec = chunkSec;
		this.videoDir = videoDir;
		this.timer = new  Timer();
		this.controller = controller;
		this.loopUpload = loopUpload;
		this.startDelay = startDelay;
		this.startRandomChunk = startRandomChunk;
	}

	public void run() {

		// connect to uploadIP
		try {
			sockChannel = SocketChannel.open();
			controller.sendMessage(MainActivity.UPDATE_STATUS,"Default send buffer: "+sockChannel.socket().getSendBufferSize());
			sockChannel.socket().setSendBufferSize(100*1024);
			controller.sendMessage(MainActivity.UPDATE_STATUS,"New send buffer: "+sockChannel.socket().getSendBufferSize());
			sockChannel.configureBlocking(true);
			sockChannel.connect(new InetSocketAddress(uploadIP, uploadPort));
		} catch (IOException e) {
			controller.sendMessage(MainActivity.UPDATE_STATUS,"Could not connect to upload server "+e.getMessage());
			cleanUp();
			return;
		}
		
		controller.sendMessage(MainActivity.UPDATE_STATUS,"SlottedFileUploader");
		//start filling the queue with files to be uploaded
		controller.sendMessage(MainActivity.UPDATE_STATUS,"delay: "+this.startDelay);
		controller.sendMessage(MainActivity.UPDATE_STATUS,"delay: "+this.startRandomChunk);
		UploadTask upTask = new UploadTask(this.startRandomChunk);
		timer.scheduleAtFixedRate(upTask, this.startDelay, this.chunkSec*1000); //wait startDelay before writing first file
		
		
		//now start the actual uploading
		keepRunning = true;
		ByteBuffer sizeBuffer = ByteBuffer.allocate(4);
		while(keepRunning){
			try {
				File f = uploadQ.take(); //will block until a file becomes available
				if(f == this.stopFile){
					Log.d(TAG,"Stopping uploading thread");
					controller.sendMessage(MainActivity.UPDATE_STATUS,"All uploads finished!");
					keepRunning = false;
					break;
				}
				Log.d(TAG,"Starting upload of "+f.getName()+", "+uploadQ.size()+" queued");
				controller.sendMessage(MainActivity.UPDATE_STATUS, "Starting upload of "+f.getName()+", "+uploadQ.size()+" still queued");
				FileInputStream fin = new FileInputStream(f);
				FileChannel fChannel = fin.getChannel();
				long size = f.length();
				sizeBuffer.putInt((int)size);
				sizeBuffer.flip(); //set buffer in read mode
				sockChannel.write(sizeBuffer);
				sizeBuffer.clear();
				
				long position = 0;
				while(position < size){
					position += fChannel.transferTo(position, 2048*1024, sockChannel);
				}	
				controller.sendMessage(MainActivity.UPDATE_STATUS, "Upload finished");
				Log.d(TAG,"Finished upload of "+f.getName());
				fin.close();
								
			} catch (IOException e) {				
				Log.w(TAG,""+e.getMessage());
				keepRunning = false;
				break;
			} catch (InterruptedException e) {
				Log.w(TAG,""+e.getMessage());
				keepRunning = false;
				break;
			}			
		}
		controller.notifyUploadFinished();
		cleanUp();
		Log.d(TAG,"FileUploader stopped");
	}

	public void stop(){
		Log.d(TAG,"Stopping FileUploader");
		if(bReadBlocked)
			cleanUp(); //to close the socket
		
		//else: we are blocked in the take() method, so give it a Poison Pill
		try {
			uploadQ.put(stopFile);
		} catch (InterruptedException e) {			
			e.printStackTrace();
		}
		
		keepRunning = false;
	}
	
	private void cleanUp() {

		try {
	
			if (sockChannel != null && sockChannel.isOpen())
				sockChannel.close();
			if (timer != null)
				timer.cancel();
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		keepRunning = false;
	}

	
	private class UploadTask extends TimerTask{

		private File [] listFiles;
		private int counter;
		private int wrap;
		private DateFormat df;
		
		UploadTask(boolean startRandom){
			listFiles = videoDir.listFiles();
			if(startRandom){
				Random randomGenerator = new Random();
				counter = randomGenerator.nextInt(listFiles.length);				
			}
			else{
				counter = 0;
			}
			wrap = listFiles.length;
	
			//sdf = (SimpleDateFormat) DateFormat.getTimeInstance();
			df = new SimpleDateFormat("hh:mm:ss");
		}
		
		@Override
		public void run() {
			try {				
				Log.d(TAG,"New segment ready queued at "+df.format(new Date()));
				uploadQ.put(listFiles[counter]);
				counter++;
				if(SlottedFileUploader.this.loopUpload){
					counter = counter % wrap;
				}
				else if(counter == listFiles.length){					
					uploadQ.put(stopFile); //to make the FileUploader thread stop graciously
					Log.d(TAG,"All files uploaded!");
					timer.cancel(); //stop putting files in the queue
				}
			} catch (InterruptedException e) {				
				e.printStackTrace();
			}					
		}
		
	}
}
