package cmu.servercommunication;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

public abstract class ServerResource {

	protected String serverLocation;
	protected String serverCollection;
	protected boolean registered;
	
	public void register(String loc){
		registered = true;
		serverLocation = loc;
	}
	
	public String getServerLocation(){
		return serverLocation;
	}
	
	abstract public String createJSON();
	
	public String getCollection(){
		return serverCollection;
	}
	
	public boolean isRegistered(){
		return registered;
	}
}
