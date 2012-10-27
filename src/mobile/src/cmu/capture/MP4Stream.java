package cmu.capture;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import org.json.JSONException;
import org.json.JSONObject;

import android.util.Log;

public class MP4Stream extends FileStream {

	private Date startTime;
	private Date stopTime;
	private long duration; // duration in seconds
	
	private final static String TAG = "MP4Stream";

	public MP4Stream(File f) {
		super(Stream.Container.MP4,f);
		this.startTime = null;
		this.duration = -1;
		
	}

	public void setStartTime(Date start) {
		this.startTime = start;
	}

	public void setStopTime(Date stop) {
		this.stopTime = stop;
		// calculate duration
		if (startTime != null) {
			duration = stopTime.getTime() - startTime.getTime();
		}
	}

	protected void fill(JSONObject obj) {
		super.fill(obj);
		try {
			if (startTime != null)
				obj.put("startTime", sdf.format(startTime));
			if (duration >= 0)
				obj.put("duration", duration);
		} catch (JSONException e) {
			Log.w(TAG, "JSONException");
		}

	}

	public String createJSON() {
		JSONObject obj = new JSONObject();
		fill(obj);
		return obj.toString();
	}

	@Override
	public boolean open() {
		return true;
	}

	@Override
	public boolean close() {
		return true;
	}


}
