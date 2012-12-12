package cmu.gigasight;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import cmu.servercommunication.RESTClient;
import cmu.servercommunication.ServerSettings;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.text.InputFilter;
import android.text.Spanned;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

public class GigasightActivity extends Activity {

	private final static String TAG = "GigasightActivity";
	private static final int REQ_ENABLEWIFI = 222;
	private static final int REQ_ENABLELOC = 333;
	public static final String PREFS_NAME = "PersonalvmConfig";
	public static final String STATE_IP = "STATE_IP";
	public static final String STATE_RESTPORT = "STATE_RESTPORT";
	public static final String STATE_UPLOADPORT = "STATE_UPLOADPORT";
	public static Handler handler;

	private EditText mIPEdit;
	private EditText mRestPortEdit;
	private EditText mUploadPortEdit;
	private boolean mWifiAsked;
	private boolean mLocAsked;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_gigasight);

		mIPEdit = (EditText) findViewById(R.id.personalvm_ip);
		mRestPortEdit = (EditText) findViewById(R.id.personalvm_restport);
		mUploadPortEdit = (EditText) findViewById(R.id.personalvm_uploadport);
		initializePersonalVMAddress();

		mWifiAsked = false;
		mLocAsked = false;

		handler = new Handler() {
			@Override
			public void handleMessage(Message msg) {
				Toast.makeText(GigasightActivity.this, (String) (msg.obj),
						Toast.LENGTH_LONG).show();
			}
		};

	}

	public void onResume() {
		super.onResume();

		// check if GPS is enabled and prompt user if necessary
		LocationManager locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
		if (!locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER) && !mLocAsked) {
			mLocAsked = true;
			buildAlertMessageNoGps();
		}

		ConnectivityManager connManager = (ConnectivityManager) getSystemService(CONNECTIVITY_SERVICE);
		NetworkInfo mWifi = connManager
				.getNetworkInfo(ConnectivityManager.TYPE_WIFI);

		if (!mWifi.isConnected() && !mWifiAsked) {
			mWifiAsked = true;
			buildAlertMessageNoWifi();
		}
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.activity_gigasight, menu);
		return true;
	}

	public void startActivity(View v) {
		if (v.getId() == R.id.button_privacy) {
			Intent intent = new Intent(this, PrivacyActivity.class);
			startActivity(intent);
			return;
		}
		if (v.getId() == R.id.button_capture_video) {
			Intent intent = new Intent(this, CaptureActivity.class);
			startActivity(intent);
			return;
		}
	}

	private void saveSettings() {

		// We need an Editor object to make preference changes.
		// All objects are from android.context.Context
		SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
		SharedPreferences.Editor editor = settings.edit();

		String newIP = mIPEdit.getText().toString();
		String newRESTPort = mRestPortEdit.getText().toString();
		String newUploadPort = mUploadPortEdit.getText().toString();
		if (!newIP.isEmpty()) {
			editor.putString(STATE_IP, newIP);
			ServerSettings.serverIP = newIP;
		}
		if (!newRESTPort.isEmpty()) {
			editor.putString(STATE_RESTPORT, newRESTPort);
			ServerSettings.restPort = newRESTPort;
		}
		if (!newUploadPort.isEmpty()) {
			editor.putString(STATE_UPLOADPORT, newUploadPort);
			ServerSettings.uploadPort = newUploadPort;
		}

		// do all necessary update actions
		ServerSettings.update();
		// Commit the edits!
		Log.d(TAG, "Saved new settings!");
		editor.commit();
	}

	private void initializePersonalVMAddress() {
		// load from settings or use default
		SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
		String mPersonalvmIP = settings.getString(STATE_IP,
				getString(R.string.personalvm_default_ip));
		String mPersonalvmRESTPort = settings.getString(STATE_RESTPORT,
				getString(R.string.personalvm_default_restport));
		String mPersonalvmUploadPort = settings.getString(STATE_UPLOADPORT,
				getString(R.string.personalvm_default_uploadport));

		ServerSettings.serverIP = mPersonalvmIP;
		ServerSettings.restPort = mPersonalvmRESTPort;
		ServerSettings.uploadPort = mPersonalvmUploadPort;
		mIPEdit.setText(mPersonalvmIP);
		mRestPortEdit.setText(mPersonalvmRESTPort);
		mUploadPortEdit.setText(mPersonalvmUploadPort);

		// attach the appropriate listeners
		AddressListener addressListener = new AddressListener();
		mIPEdit.setOnEditorActionListener(addressListener);
		mRestPortEdit.setOnEditorActionListener(addressListener);
		mUploadPortEdit.setOnEditorActionListener(addressListener);
		// add a filter to check if you are entering a good IP address
		InputFilter[] filters = new InputFilter[1];
		filters[0] = new InputFilter() {
			public CharSequence filter(CharSequence source, int start, int end,
					Spanned dest, int dstart, int dend) {
				if (end > start) {
					String destTxt = dest.toString();
					String resultingTxt = destTxt.substring(0, dstart)
							+ source.subSequence(start, end)
							+ destTxt.substring(dend);
					if (!resultingTxt
							.matches("^\\d{1,3}(\\.(\\d{1,3}(\\.(\\d{1,3}(\\.(\\d{1,3})?)?)?)?)?)?")) {
						return "";
					} else {
						String[] splits = resultingTxt.split("\\.");
						for (int i = 0; i < splits.length; i++) {
							if (Integer.valueOf(splits[i]) > 255) {
								return "";
							}
						}
					}
				}

				return null;
			}
		};
		mIPEdit.setFilters(filters);
	}

	private class AddressListener implements TextView.OnEditorActionListener {

		public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {

			if (actionId == EditorInfo.IME_ACTION_DONE) {
				saveSettings();
				return false; // we still return false (and not true) to
								// have the keyboard disappear
								// if we return true, we have to remove the
								// keyboard ourselves
			}
			return false;

		}

	}

	private void buildAlertMessageNoWifi() {
		final AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setMessage(
				"WiFi is currently disabled, do you want to enable it?")
				.setCancelable(false)
				.setPositiveButton("Yes",
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog,
									final int id) {
								startActivityForResult(new Intent(
										Settings.ACTION_WIFI_SETTINGS),
										REQ_ENABLEWIFI);
								dialog.dismiss();
							}
						})
				.setNegativeButton("No", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, final int id) {
						Toast.makeText(
								GigasightActivity.this,
								"WiFi not enabled, your files will be uploaded when back online",
								Toast.LENGTH_SHORT).show();
						dialog.dismiss();
					}
				});
		final AlertDialog alert = builder.create();
		alert.show();
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
						Toast.makeText(
								GigasightActivity.this,
								"GPS not enabled, only video will be captured!",
								Toast.LENGTH_SHORT).show();
						dialog.cancel();
					}
				});
		final AlertDialog alert = builder.create();
		alert.show();
	}

	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == REQ_ENABLEWIFI && resultCode == 0) {
			ConnectivityManager connManager = (ConnectivityManager) getSystemService(CONNECTIVITY_SERVICE);
			NetworkInfo mWifi = connManager
					.getNetworkInfo(ConnectivityManager.TYPE_WIFI);

			if (mWifi.isConnectedOrConnecting()) {
				Toast.makeText(this, "WiFi enabled", Toast.LENGTH_SHORT).show();
			} else {
				// Users did not switch on the WiFi
				Toast.makeText(
						GigasightActivity.this,
						"WiFi not enabled, your files will be uploaded when back online",
						Toast.LENGTH_LONG).show();
				// GigasightActivity.this.finish();
			}

			return;
		}

		if (requestCode == REQ_ENABLELOC && resultCode == 0) {
			String provider = Settings.Secure.getString(getContentResolver(),
					Settings.Secure.LOCATION_PROVIDERS_ALLOWED);
			if (!provider.isEmpty()) {
				Toast.makeText(this, "GPS enabled", Toast.LENGTH_LONG).show();
			} else {
				// Users did not switch on the GPS
				Toast.makeText(this,
						"GPS not enabled, we will only capture video",
						Toast.LENGTH_LONG).show();
			}
			return;
		}

	}

}
