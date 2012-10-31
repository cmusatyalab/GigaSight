package cmu.gigasight;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.support.v4.app.NavUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.Toast;
import cmu.capture.CameraRecorder;
import cmu.capture.GPSstream;
import cmu.capture.MP4Stream;
import cmu.capture.Segment;
import cmu.capture.Stream;
import cmu.capture.Segment.Type;
import cmu.servercommunication.FileUploader;
import cmu.servercommunication.RESTClient;

public class CaptureActivity extends Activity {

	private static final String TAG = "CaptureActivity";
	private CameraRecorder mCamRec;
	private Button bStop;
	private Button bCapture;
	private Segment segment;
	private MP4Stream mp4stream;
	private GPSstream gpsstream;
	private boolean gpsCapturing;
	private FileUploader fileUploader;
	private LocationManager locationManager;
	private GPSStreamListener gpsListener;
	private static final int REQ_ENABLELOC = 123;
	public static final String MEDIA_DIR = "GigaSight";

	
	private ProgressDialog progressDialog;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_capture);
		getActionBar().setDisplayHomeAsUpEnabled(true);
		// Camera initialization is done in onResume, as the camera is released
		// in the onPause state!

		progressDialog = new ProgressDialog(this);
		fileUploader = new FileUploader();
		fileUploader.start();

		bCapture = (Button) findViewById(R.id.button_start_capture);
		bCapture.setEnabled(true);
		bStop = (Button) findViewById(R.id.button_stop_capture);
		bStop.setEnabled(false);

		
		locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
		gpsListener = new GPSStreamListener();

		bCapture.setOnClickListener(new OnClickListener() {

			public void onClick(View arg0) {
				Log.d(TAG, "Capture button pressed");

				// create new segment and streams each time the button is
				// pressed				
				segment = new Segment(Segment.Type.RECORDED);
				String [] streamNames = createStreamNames(new String [] {"VID", "GPS"}, new String [] {"mp4","csv"});

				if (locationManager
						.isProviderEnabled(LocationManager.GPS_PROVIDER)) {
					Log.d(TAG, "Start GPS capturing, results are written in "+ streamNames[1]);			
					
					gpsstream = new GPSstream(new File(streamNames[1]));
					gpsstream.open();
					segment.addStream(gpsstream);

					locationManager.requestLocationUpdates(
							LocationManager.GPS_PROVIDER, 0, 0, gpsListener);
					locationManager
							.requestLocationUpdates(
									LocationManager.NETWORK_PROVIDER, 0, 0,
									gpsListener);
					Location last = locationManager
							.getLastKnownLocation(LocationManager.GPS_PROVIDER);
					if (last == null) {
						last = locationManager
								.getLastKnownLocation(LocationManager.NETWORK_PROVIDER);
					}
					if (last != null) {
						Log.w(TAG,"Setting timestamp of cached location to current time");
						last.setTime(new Date().getTime());
						gpsstream.add(last);
					}
					gpsCapturing = true;
				}

				mp4stream = new MP4Stream(new File(streamNames[0]));
				Log.d(TAG,"Start video capturing, results are written in "+streamNames[0]);
				mp4stream.open();
				segment.addStream(mp4stream);			

				Date startTime = mCamRec.startRecording(mp4stream.getFile());
				if (startTime != null) { // recording started
					mp4stream.setStartTime(startTime);
					bCapture.setEnabled(false);
					bStop.setEnabled(true);
				}
			}

		});

		bStop.setOnClickListener(new OnClickListener() {
			public void onClick(View arg0) {
				Log.d(TAG, "Stop button pressed");
				stopCapturing();
			}
		});
	}

	public void onDestroy() {
		Log.d(TAG, "onDestroy");
		super.onDestroy();
		fileUploader.stop();
	}

	public void onResume() {
		Log.v(TAG, "onResume");

		if(isConnected()){
			syncToPersonalVM();
		}
		else
			Toast.makeText(this, "WiFi not enabled, offline operation",Toast.LENGTH_LONG).show();
		
		// initialize recording
		mCamRec = new CameraRecorder(this);
		mCamRec.initCameraRecorder();

		FrameLayout previewFL = (FrameLayout) findViewById(R.id.camera_preview);
		previewFL.addView(mCamRec.getPreview());

		super.onResume();
	}

	public void onPause() {
		Log.d(TAG, "onPause");
		if (mCamRec.recording) {
			stopCapturing();
		}
		mCamRec.releaseCameraAndPreview();

		super.onPause();
	}

	private void stopCapturing() {
		Date stopTime = mCamRec.stopRecording();
		if (stopTime != null)
			mp4stream.setStopTime(stopTime);
		mp4stream.close();
		
		if (gpsCapturing) { 
			locationManager.removeUpdates(gpsListener);
			gpsstream.close();			 
		}
				
		if(isConnected())
			syncSegment(segment);

		gpsCapturing = false;
		mp4stream = null;
		gpsstream = null;
		segment = null;
		
		bCapture.setEnabled(true);
		bStop.setEnabled(false);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.activity_capture, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case android.R.id.home:
			NavUtils.navigateUpFromSameTask(this);
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	private void syncToPersonalVM(){
		File mediaStorageDir = new File(getExternalFilesDir(null)+ File.separator + MEDIA_DIR);
		File [] segmentList = mediaStorageDir.listFiles();
		if(segmentList == null)
			return;
		
		if(segmentList.length > 0){
			progressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			progressDialog.setMessage("Syncing segments to personalVM");
		}
		          
		for(File segDir : segmentList){			
			if(segDir.isDirectory()){
				Segment seg = new Segment(Type.RECORDED);
				
				File [] streams = segDir.listFiles();				
				if(streams.length == 0){
					Log.d(TAG,"Deleting segment directory "+segDir.getAbsolutePath());
					segDir.delete();
					continue;
				}				
				for(File stream : streams){
					if(stream.getName().contains("VID"))
						seg.addStream(new MP4Stream(stream));
					else if(stream.getName().contains("GPS"))
						seg.addStream(new GPSstream(stream));
					else
						Log.d(TAG,"Unknown stream: "+stream.getName());							
				}				
				//register segment and stream and initiate upload			
				syncSegment(seg);
				//files will be deleted after upload by fileUploader
			}
			

		}
		
	}
	private void syncSegment(final Segment seg) {
		new AsyncTask<Void, String, Void>() {
		
			Segment i_segment;
			GPSstream i_gpsstream;
			MP4Stream i_mp4stream;
			
			@Override
            protected void onPreExecute()
            {
                /*
                 * This is executed on UI thread before doInBackground(). It is
                 * the perfect place to show the progress dialog.
                 */				
				i_segment = seg; //internal copy
				
            }
			@Override
			protected Void doInBackground(Void... params) {
				publishProgress("Registering segment...");
				Log.d(TAG,"Registering segment");
				RESTClient.post(i_segment, false);
				
				i_gpsstream = (GPSstream) i_segment.getStream(Stream.Container.GPS);
				i_mp4stream = (MP4Stream) i_segment.getStream(Stream.Container.MP4);
				
				
				// hack for Yu: first send the GPS
				if(i_gpsstream != null){				
					publishProgress("Registering GPS stream...");
					Log.d(TAG,"Registering GPS stream");
					RESTClient.post(i_segment.getStream(Stream.Container.GPS), false);
				}
				
				/* now, register the mp4stream(s) */
				publishProgress("Registering MP4 stream...");
				Log.d(TAG,"Registering mp4 stream");
				RESTClient.post(i_mp4stream, false);
				
							
				//hack: we do a new put, should not be necessary because all information is already available in the post
				RESTClient.put(i_mp4stream, true); 
				
			
				//now start the upload
				Log.d(TAG,"Starting upload of "+i_mp4stream.getFile().getName());
				fileUploader.upload(i_mp4stream);		
				
				if (i_gpsstream != null) {
					//hack: we do a new put, even though it is not necessary
					RESTClient.put(i_gpsstream,true); 
					fileUploader.upload(i_gpsstream);				 
				}
				
					

				return null;
				
			}

			@Override
			protected void onProgressUpdate(String... s) {
		        super.onProgressUpdate();
		        progressDialog.setMessage(s[0]);
		    }
			@Override
			protected void onPostExecute(Void result) {
				progressDialog.dismiss();
				if (!i_segment.isRegistered() || !i_mp4stream.isRegistered()
						|| ((i_gpsstream != null) && !i_gpsstream.isRegistered()))
					Toast.makeText(CaptureActivity.this,"Could not register segments and streams on server!",Toast.LENGTH_LONG).show();
				else
					Toast.makeText(CaptureActivity.this,
							"Segments and streams registered on server",Toast.LENGTH_LONG).show();
			}
		}.execute();
	}

	
	private void cleanMediaFiles() {
		File mediaStorageDir = new File(getExternalFilesDir(null)
				+ File.separator + MEDIA_DIR);
		if (mediaStorageDir.exists())
			deleteRecursive(mediaStorageDir);

	}

	private void deleteRecursive(File fileOrDirectory) {
		if (fileOrDirectory.isDirectory())
			for (File child : fileOrDirectory.listFiles())
				deleteRecursive(child);
		fileOrDirectory.delete();
	}

	private String getNewSegmentDir() {
		// To be safe, you should check that the SDCard is mounted
		// using Environment.getExternalStorageState() before doing this.
		Log.d(TAG, "sd card state: " + Environment.getExternalStorageState());
		File mediaStorageDir = new File(getExternalFilesDir(null) + File.separator + MEDIA_DIR);
		if(!mediaStorageDir.exists()){
			if(!mediaStorageDir.mkdirs()){
				Log.d(TAG,"Failed to create directory "+mediaStorageDir);
				return null;
			}
		}
		
		String segmentDir = null;
		int segmentID = 0;
		while(new File((segmentDir = mediaStorageDir.getAbsolutePath() + File.separator + segmentID)).exists())
			segmentID++;

		File segDir = new File(segmentDir);
		if(!segDir.exists()){ //it should never exist...
			if(!segDir.mkdirs()){
				Log.d(TAG,"Failed to create directory "+segDir);
				return null;
			}
		}
		else{
			Log.e(TAG,"Already existing segment directory created!");
			return null;
		}
		
		
		return segmentDir;		
	}
	
	private String [] createStreamNames(String [] prefix, String [] suffix){
		int noStreams = prefix.length;
		String [] result = new String [noStreams];
		String segmentDir = getNewSegmentDir();
		String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());

		for(int i = 0; i < noStreams; i++ ){			
			result[i] = segmentDir + File.separator + prefix[i] + "_" + timeStamp + "." + suffix[i];
		}
		
		return result;
	}

	
		protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == REQ_ENABLELOC && resultCode == 0) {
			String provider = Settings.Secure.getString(getContentResolver(),
					Settings.Secure.LOCATION_PROVIDERS_ALLOWED);
			if (!provider.isEmpty()) {
				Toast.makeText(this, "GPS enabled", Toast.LENGTH_LONG).show();
			} else {
				// Users did not switch on the GPS
				Toast.makeText(this,
						"GPS not enabled, we will only send video",
						Toast.LENGTH_LONG).show();
			}
		}
	}

	private class GPSStreamListener implements LocationListener {

		public void onLocationChanged(Location loc) {
			if (gpsCapturing)
				gpsstream.add(loc);
		}

		public void onProviderDisabled(String provider) {
			if (provider.equals(LocationManager.GPS_PROVIDER)) {
				Toast.makeText(CaptureActivity.this, "GPS disabled!",
						Toast.LENGTH_SHORT).show();
			}

		}

		public void onProviderEnabled(String provider) {

		}

		public void onStatusChanged(String provider, int status, Bundle extras) {

		}

	}

	private boolean isConnected() {
		ConnectivityManager connManager = (ConnectivityManager) getSystemService(CONNECTIVITY_SERVICE);
		NetworkInfo mWifi = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);

		return mWifi.isConnected();

	}
}
