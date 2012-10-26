package cmu.capture;

import org.json.JSONException;
import org.json.JSONObject;

import android.util.Log;

import cmu.servercommunication.ServerResource;

public class Stream extends ServerResource {

	private static final String TAG = "Stream";
	
	enum Container {
		RTSP_RTP{
			public String toString(){
				return "RTSP_RTP";
			}
		},
		MP4{
			public String toString(){
				return "MP4";
			}
		},
		GPS{
			public String toString(){
				return "GPS";
			}
		}
	}
	
	private Container c;
	private Segment seg;
	
	public Stream(Container c) {
		this.c = c;		
	}

	public void setSegment(Segment segment) {
		this.seg = segment;
		this.serverCollection = seg.getServerLocation();		
	}
	
	@Override
	public String getCollection(){
		this.serverCollection = seg.getServerLocation();
		return serverCollection;
	}
	
	protected void fill(JSONObject obj){
		try {
			obj.put("container", c.toString());
		} catch (JSONException e) {
			Log.e(TAG,e.getMessage());
		}		
	}

	public String createJSON(){
		JSONObject obj = new JSONObject();
		fill(obj);
		return obj.toString();
	}
}
