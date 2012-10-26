package cmu.servercommunication;

import java.io.IOException;
import java.io.UnsupportedEncodingException;

import org.apache.http.Header;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.message.BasicHeader;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;
import org.apache.http.protocol.HTTP;


import cmu.gigasight.GigasightActivity;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.http.AndroidHttpClient;
import android.os.AsyncTask;
import android.os.Message;
import android.util.Log;

public class RESTClient {

	private static final String TAG = "RESTClient";
	
	public static void post(ServerResource res, boolean aSync) {
		if(res.getCollection() == null){
			Log.w(TAG,"Trying to create a resource without valid collection!");
			return;
		}
		String url = "http://"+ServerSettings.serverIP+":"+ServerSettings.restPort+res.getCollection();
		if(aSync){
			PostTask pt = new PostTask(res);
		    pt.execute(new String[] { url });
		}
		else
			doPost(res,url);
	}
	
	public static void put(ServerResource res, boolean aSync){
		if(res.getServerLocation() == null){
			Log.w(TAG,"ServerResource has no valid ServerLocation!"+res.createJSON());
			return;
		}
		String url = "http://"+ServerSettings.serverIP+":"+ServerSettings.restPort+res.getServerLocation();
		if(aSync){
			PutTask pt = new PutTask(res);
			pt.execute(new String[] { url });
		}
		else
			doPut(res,url);
	}
	
	
	private static HttpResponse doPost(ServerResource res, String url){
		AndroidHttpClient client = AndroidHttpClient.newInstance("Android");
		try {
			HttpPost request = new HttpPost(url);
			String jsonText = res.createJSON();
			Log.d(TAG,request.getRequestLine().toString());
							
			if(jsonText!= null){
				StringEntity se = new StringEntity(jsonText);
				se.setContentEncoding(new BasicHeader(HTTP.CONTENT_TYPE," application/json"));
				request.setEntity(se);
				Log.d(TAG,jsonText);
			}
			
			HttpResponse response = client.execute(request);
			if (response != null) {
				Log.d(TAG,response.getStatusLine().toString());
				// save the location in the ServerResource
				if (response.getStatusLine().getStatusCode() == HttpStatus.SC_CREATED) {
					Header[] headers = response.getHeaders("Location");
					String location = headers[0].getValue();
					res.register(location);
					Log.d(TAG,"Location: "+location);
				}
			}
			return response;
			
		} catch (IOException e) {
			Message m = GigasightActivity.handler.obtainMessage(0,"Server problem: " + e.getMessage());
			GigasightActivity.handler.sendMessage(m);			
			e.printStackTrace();
			return null;
		} finally {
			client.close();
		}
	}

	private static HttpResponse doPut(ServerResource res, String url){
		AndroidHttpClient client = AndroidHttpClient.newInstance("Android");
		try {
			HttpPut request = new HttpPut(url);
			String jsonText = res.createJSON();
			StringEntity se = new StringEntity(jsonText);
			se.setContentEncoding(new BasicHeader(HTTP.CONTENT_TYPE," application/json"));
			request.setEntity(se);
			
			Log.d(TAG,request.getRequestLine().toString());
			Log.d(TAG,jsonText);
			HttpResponse response = client.execute(request);
			if (response != null) {
				Log.d(TAG,response.getStatusLine().toString());
			}
			return response;
		} catch (IOException e) {
			Message m = GigasightActivity.handler.obtainMessage(0,"Server problem: " + e.getMessage());
			GigasightActivity.handler.sendMessage(m);	
			e.printStackTrace();
			return null;
		} finally {
			client.close();
		}
	
	}
	private static class PostTask extends AsyncTask<String, Void, HttpResponse> {
		ServerResource res;

		protected PostTask(ServerResource res) {
			this.res = res;
		}

		@Override
		protected HttpResponse doInBackground(String... params) {
			String link = params[0];
			return doPost(res, link);
		}

		@Override
		protected void onPostExecute(HttpResponse result) {
			// Do something with result on UI Thread here if needed			
		}
	}
	
	private static class PutTask extends AsyncTask<String, Void, HttpResponse> {
		ServerResource res;

		protected PutTask(ServerResource res) {
			this.res = res;
		}

		@Override
		protected HttpResponse doInBackground(String... params) {
			String link = params[0];
			return doPut(res,link);
		}

		@Override
		protected void onPostExecute(HttpResponse result) {
			// Do something with result on UI Thread here if needed
			
		}
	}

}
