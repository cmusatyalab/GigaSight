package com.example.cmu.experimentclient;

public class ControllerProtocol {

	public static final byte REQUEST_NAME = 1;
	public static final byte SET_NAME = 2;
	public static final byte REQUEST_UPLOAD_IP = 3;
	public static final byte SET_UPLOAD_IP = 4;
	public static final byte REQUEST_UPLOAD_PORT = 5;
	public static final byte SET_UPLOAD_PORT = 6;
	public static final byte OPEN_RECEIVE_PORT = 7;
	public static final byte RECEIVE_PORT_OPENED = 8;
	public static final byte CLOSE_RECEIVE_PORT = 9;
	public static final byte RECEIVE_PORT_CLOSED = 10;
	public static final byte DELETE_FILES = 11;
	public static final byte START_EXPERIMENT = 12;
	public static final byte STOP_EXPERIMENT = 13;
	public static final byte SET_CHUNKINTERVAL = 14;
	public static final byte SET_LOOPUPLOAD = 15;
	public static final byte UPLOAD_FINISHED = 16;
	public static final byte START_RANDOM_CHUNK = 17;
	public static final byte START_RANDOM_INTERVAL = 18;
	public static final byte SET_STARTDELAY = 19;
}
 