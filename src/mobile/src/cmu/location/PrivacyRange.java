package cmu.location;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import com.google.android.maps.GeoPoint;
import com.google.android.maps.OverlayItem;

public class PrivacyRange extends OverlayItem {

	private float radius_m;

	public PrivacyRange(GeoPoint p, float radius_m) {
		super(p,"Privacy","range in meters: "+radius_m);
		this.radius_m = radius_m;
	}
	
	public float getRadiusMeter(){
		return radius_m;
	}	
	

}
