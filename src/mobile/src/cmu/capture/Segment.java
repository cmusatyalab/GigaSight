package cmu.capture;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import java.io.StringWriter;
import java.util.ArrayList;

import org.json.JSONException;
import org.json.JSONObject;

import android.util.Log;

import cmu.capture.Stream.Container;
import cmu.servercommunication.ServerResource;



public class Segment extends ServerResource {
	
	private final static String TAG = "Segment";
	public enum Type {
		RECORDED{
			public String toString(){
				return "recorded";
			}
		},
		LIVESTREAM{
			public String toString(){
				return "livestream";
			}
		};
	}
	
	private Type type;
	private ArrayList<Stream> streams;
	
	public Segment(Type type){
		this.type = type;
		this.serverCollection = "/segment";
		streams = new ArrayList<Stream>();
	}
	
	public void addStream(Stream s){
		streams.add(s);
		s.setSegment(this);
	}
	
	public String createJSON(){
		JSONObject obj = new JSONObject();
		try {
			obj.put("type",type.toString());
		} catch (JSONException e) {
			Log.e(TAG,e.getMessage());
		}
		return obj.toString();
	}
	
	public Stream getStream(Container c){
		for(Stream s : streams){
			if(s.getContainer() == c)
				return s;
		}
		return null;
	}


}
