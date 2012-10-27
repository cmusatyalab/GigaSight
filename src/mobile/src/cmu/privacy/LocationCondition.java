package cmu.privacy;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

public class LocationCondition extends Condition {

	private String centre_lat;
	private String centre_long;
	private String radius_m;
	
	public LocationCondition(int centre_lat, int centre_long, float radius_m) {
		super(ConditionType.LOCATION);
		this.centre_lat = ""+centre_lat;
		this.centre_long = ""+centre_long;
		this.radius_m = ""+radius_m;
	}

	@Override
	public String createFormattedText() {
		StringBuilder dataString = new StringBuilder();
		dataString.append("\ttype: LOCATION\n");
		dataString.append("\tlatitude: "+centre_lat+"\n");
		dataString.append("\tlongitude: "+centre_long+"\n");
		dataString.append("\tradius: "+radius_m+"\n");
		return dataString.toString();
	}

	@Override
	protected String createDataString() {
		StringBuilder dataString = new StringBuilder();
		dataString.append("{");
		dataString.append("\"centre_lat\" = "+ centre_lat + ",");
		dataString.append("\"centre_long\" = "+ centre_long + ",");
		dataString.append("\"radius\" = \""+radius_m);
		dataString.append("}");
		return dataString.toString();
	}

}
