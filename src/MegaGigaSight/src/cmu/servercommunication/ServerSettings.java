package cmu.servercommunication;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import cmu.privacy.Privacy;

public class ServerSettings {

	/* helper class so that classes that do not have an ApplicationContext can easily access this data, 
	 * without having access to SharedPreferences
	 */
	public static String serverIP; 
	public static String restPort; 
	public static String uploadPort;
	public static int uploadType;
	public static int devNo = 0;
	public static int totalDev = 1;
	public static final int RANDOMUPLOAD = 0;
	public static final int SLOTTEDUPLOAD = 1;
	
	//this method is called when one of the data fields has been updated 
	public static void update(){
		//reregister all existing resources
		Privacy.recreate();
	}
}
