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

	//this method is called when one of the data fields has been updated 
	public static void update(){
		//reregister all existing resources
		Privacy.recreate();
	}
}
