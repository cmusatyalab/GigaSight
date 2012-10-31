package cmu.privacy;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

public class LocationCondition extends Condition {

	private long centre_lat_e6;
	private long centre_long_e6;
	private String radius_m;
	
	public LocationCondition(int centre_lat, int centre_long, float radius_m) {
		super(ConditionType.LOCATION);
		this.centre_lat_e6 = centre_lat;
		this.centre_long_e6 = centre_long;
		this.radius_m = ""+radius_m;
	}

	@Override
	public String createFormattedText() {
		StringBuilder dataString = new StringBuilder();
		dataString.append("\ttype: LOCATION\n");
		dataString.append("\tlatitude: "+(centre_lat_e6*1.0)/1000000+"\n");
		dataString.append("\tlongitude: "+(centre_long_e6*1.0)/1000000+"\n");
		dataString.append("\tradius: "+radius_m+"\n");
		return dataString.toString();
	}

	@Override
	protected String createDataString() {
		StringBuilder dataString = new StringBuilder();
		dataString.append("{");
		dataString.append("\"centre_lat\" = "+ (centre_lat_e6*1.0)/1000000 + ",");
		dataString.append("\"centre_long\" = "+ (centre_long_e6*1.0)/1000000 + ",");
		dataString.append("\"radius\" = "+radius_m);
		dataString.append("}");
		return dataString.toString();
	}

}
