package cmu.gigasight;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
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
	public static final String VIDEOPREFIX = "GigaSight";
	public static final String GPSPREFIX = "GigaSight";

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_capture);
		getActionBar().setDisplayHomeAsUpEnabled(true);
		// Camera initialization is done in onResume, as the camera is released
		// in the onPause state!

		cleanMediaFiles(); // clean the directory from previous sessions
		fileUploader = new FileUploader();
		fileUploader.start();

		// check if GPS is enabled and prompt user if necessary
		locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
		if (!locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)) {
			buildAlertMessageNoGps();
		}

		gpsListener = new GPSStreamListener();

		bCapture = (Button) findViewById(R.id.button_start_capture);
		bCapture.setEnabled(true);
		bStop = (Button) findViewById(R.id.button_stop_capture);
		bStop.setEnabled(false);

		// can be moved to onCreate??
		bCapture.setOnClickListener(new OnClickListener() {

			public void onClick(View arg0) {
				Log.d(TAG, "Capture button pressed");
				// create new segment and streams each time the button is
				// pressed
				segment = new Segment(Segment.Type.RECORDED);
				if (locationManager
						.isProviderEnabled(LocationManager.GPS_PROVIDER)) {
					Log.d(TAG, "Start GPS capturing");
					gpsstream = new GPSstream(getOutputGPSFile());
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
						gpsstream.add(last);
					}
					gpsCapturing = true;
				}
				
				mp4stream = new MP4Stream(getOutputMediaFile());
				mp4stream.open();
				segment.addStream(mp4stream);
				
				registerSegmentAndStream();

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
		RESTClient.put(mp4stream, true);
		fileUploader.upload(mp4stream);
		mp4stream = null;
		
		if (gpsCapturing) {
			locationManager.removeUpdates(gpsListener);
			gpsstream.close();
			gpsCapturing = false;
			RESTClient.put(gpsstream, true);
			fileUploader.upload(gpsstream);
			gpsstream = null;
		}
		
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

	public void registerSegmentAndStream() {
		new AsyncTask<Void, Void, Void>() {
			@Override
			protected Void doInBackground(Void... params) {
				// synchronously, we must be sure that the registration
				// succeeded before registering all streams in this segment
				RESTClient.post(segment, false);

				//hack for Yu: first send the GPS
				if (gpsCapturing)
					RESTClient.post(gpsstream, false);				
				/* now, register the mp4stream(s) */
				RESTClient.post(mp4stream, false);
				return null;
			}

			@Override
			protected void onPostExecute(Void result) {
				Toast.makeText(CaptureActivity.this,
						"Segments and streams registered on server",
						Toast.LENGTH_SHORT).show();
			}
		}.execute();
	}

	private void cleanMediaFiles() {
		File mediaStorageDir = new File(
				Environment
						.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES),
				VIDEOPREFIX);
		File[] files = mediaStorageDir.listFiles();

		for (File f : files)
			f.delete();
	}

	private static File getOutputMediaFile() {
		// To be safe, you should check that the SDCard is mounted
		// using Environment.getExternalStorageState() before doing this.

		Log.d(TAG, "sd card state: " + Environment.getExternalStorageState());
		File mediaStorageDir = new File(
				Environment
						.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES),
				VIDEOPREFIX);

		// This location works best if you want the created images to be shared
		// between applications and persist after your app has been uninstalled.
		// There is also a location for videos private to your app: then do
		// getExternalFilesDir instead

		// Create the storage directory if it does not exist
		if (!mediaStorageDir.exists()) {
			if (!mediaStorageDir.mkdirs()) {
				Log.d(TAG, "failed to create directory");
				return null;
			}
		}

		// Create a media file name
		String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss")
				.format(new Date());
		File mediaFile = new File(mediaStorageDir.getPath() + File.separator
				+ "VID_" + timeStamp + ".mp4");
		return mediaFile;
	}

	private File getOutputGPSFile() {
		// To be safe, you should check that the SDCard is mounted
		// using Environment.getExternalStorageState() before doing this.

		//File mediaStorageDir = new File(Environment.getExternalStorageDirectory(),GPSPREFIX);
		File mediaStorageDir = new File(getFilesDir() + File.separator + GPSPREFIX);
		// This location works best if you want the created images to be shared
		// between applications and persist after your app has been uninstalled.
		// There is also a location for videos private to your app: then do
		// getExternalFilesDir instead

		// Create the storage directory if it does not exist
		if (!mediaStorageDir.exists()) {
			if (!mediaStorageDir.mkdirs()) {
				Log.d(TAG, "failed to create directory");
				return null;
			}
		}

		// Create a media file name
		String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss")
				.format(new Date());
		File gpsFile = new File(mediaStorageDir.getPath() + File.separator
				+ "GPS_" + timeStamp + ".csv");
		return gpsFile;
		
	}

	private void buildAlertMessageNoGps() {
		final AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setMessage(
				"GPS is currently disabled, do you want to enable it?")
				.setCancelable(false)
				.setPositiveButton("Yes",
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog,
									final int id) {
								startActivityForResult(
										new Intent(
												Settings.ACTION_LOCATION_SOURCE_SETTINGS),
										REQ_ENABLELOC);
							}
						})
				.setNegativeButton("No", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, final int id) {
						Toast.makeText(CaptureActivity.this,
								"GPS not enabled, we will only send video",
								Toast.LENGTH_SHORT).show();
						dialog.cancel();
					}
				});
		final AlertDialog alert = builder.create();
		alert.show();
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
}
