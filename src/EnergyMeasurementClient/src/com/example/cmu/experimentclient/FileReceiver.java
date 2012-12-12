package com.example.cmu.experimentclient;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.SocketChannel;
import java.util.ArrayList;

import android.content.Context;
import android.util.Log;

public class FileReceiver implements Runnable {

	ServerSocket ss;
	int listenPort;
	private static final String TAG = "FileReceiver";
	private static final int DEFAULTPORT = 4444;
	private ArrayList<myFileWriter> runningFileWriters;
	private boolean bAcceptBlocked;
	private File videoStorageDir;

	public FileReceiver(File videoStorageDir) {
		listenPort = 0;
		ss = null;
		runningFileWriters = new ArrayList<myFileWriter>();
		this.videoStorageDir = videoStorageDir;
	}

	public int init() {

		int tryPort = DEFAULTPORT;
		ss = null;
		while (ss == null && tryPort < 4450) {
			Log.d(TAG, "Trying port " + tryPort);
			try {
				ss = new ServerSocket(tryPort);
				Log.d(TAG, "Port " + tryPort + " succeeded");
			} catch (IOException e) {
				Log.w(TAG, e.getMessage());
				tryPort++;
				continue;
			}
			break;
		}
		if (ss == null) {
			Log.d(TAG, "No port found!");
			return -1;
		}

		// ok, we found a port, so let's start listening
		listenPort = tryPort;
		return listenPort;
	}

	public void run() {

		while (true) {
			try {

				bAcceptBlocked = true;
				Socket helpSocket = ss.accept();
				bAcceptBlocked = false;
				new Thread(new myFileWriter(helpSocket)).start();
			} catch (IOException e) {
				Log.d(TAG, e.getMessage());
				break;
			}
		}

		Log.d(TAG, "Ending fileReceive thread");
		cleanUp();
	}

	public void stop() {
		if (bAcceptBlocked) {
			try {
				ss.close(); // closing the socket will throw an Exception in the
							// while-loop of the run call
			} catch (IOException e) {
				Log.d(TAG, e.getMessage());
			}
		} else
			cleanUp();
	}

	private void cleanUp() {
		// check on running uploads?
		if (ss != null)
			try {
				Log.d(TAG, "Closing socket listening on port " + listenPort);
				ss.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
	}

	private class myFileWriter implements Runnable {
		private Socket sock;
		private DataInputStream din;
		private FileOutputStream fout;

		myFileWriter(Socket sock) {
			this.sock = sock;
			this.din = null;
			this.fout = null;
		}

		public void run() {
			runningFileWriters.add(this);
			String fileName = null;
			try {
				din = new DataInputStream(new BufferedInputStream(
						sock.getInputStream()));
				int fileNameLength = din.readInt();
				byte[] bName = new byte[fileNameLength];
				din.read(bName);
				fileName = new String(bName);
				Log.d(TAG, "Filename: " + fileName);

			} catch (IOException e1) {
				Log.w(TAG, e1.getMessage());
				cleanUp();
				return;
			}

			byte[] fileData = new byte[64 * 1024];
			int totalBytes = 0;
			int bytes_read = 0;

			try {

				fout = new FileOutputStream(new File(videoStorageDir+File.separator+fileName));				
				while ((bytes_read = din.read(fileData)) != -1) {
					fout.write(fileData, 0, bytes_read);
					totalBytes += bytes_read;
				}
				Log.d(TAG, "Bytes received: " + totalBytes);

			} catch (IOException e) {
				cleanUp();
				Log.w(TAG, e.getMessage());
				return;
			}

			cleanUp();			
		}

		private void cleanUp() {
			try {
				if (din != null)
					din.close();
				if (fout != null)
					fout.close();
				sock.close();
			} catch (IOException e) {
				Log.w(TAG, e.getMessage());
			}
			runningFileWriters.remove(this);
		}

	}
}
