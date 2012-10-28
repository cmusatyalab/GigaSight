package cmu.gigasight;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import java.util.List;

import cmu.location.PrivacyItemizedOverlay;
import cmu.location.PrivacyRange;
import cmu.privacy.Action;
import cmu.privacy.ContentCondition;
import cmu.privacy.LocationCondition;
import cmu.privacy.Privacy;
import cmu.privacy.Rule;
import cmu.servercommunication.ServerSettings;

import com.google.android.maps.GeoPoint;
import com.google.android.maps.MapActivity;
import com.google.android.maps.MapController;
import com.google.android.maps.MapView;
import com.google.android.maps.Overlay;

import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.support.v4.app.NavUtils;
import android.text.Editable;
import android.text.TextWatcher;

public class LocationRuleActivity extends MapActivity {

	private MapView mapView;
	private PrivacyItemizedOverlay itemOverlay;
	private MapController mc;
	private EditText radiusView;
	private SharedPreferences settings;
	private static final String TAG = "MainActivity";
	private static final String PREFS_NAME = "LocationRuleActivityConfig";
	private static final String SETTING_RADIUS = "RADIUS";
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		//Log.d(TAG,"onCreate");
		setContentView(R.layout.activity_location_rule);

		mapView = (MapView) findViewById(R.id.mapview);
		mc = mapView.getController();
		mapView.setBuiltInZoomControls(true);
		
		List<Overlay> mapOverlays = mapView.getOverlays();
		radiusView = (EditText) findViewById(R.id.radius);
		Drawable drawable = this.getResources().getDrawable(R.drawable.marker);
		itemOverlay = new PrivacyItemizedOverlay(drawable, radiusView);
		mapOverlays.add(itemOverlay);

		//zoom to Gates Hillman		
		mc.setCenter(new GeoPoint(40444178,-79944635));
		mc.setZoom(18);
		
		Button mButton = (Button) findViewById(R.id.button_locationdone);
		mButton.setOnClickListener(new OnClickListener(){
			public void onClick(View v) {
				saveRules();
				Intent intent = new Intent(LocationRuleActivity.this,PrivacyActivity.class);
				startActivity(intent);
			}			
		});		
		
	}
	
	public void onResume(){
		super.onResume();
		SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
		radiusView.setText(settings.getString(SETTING_RADIUS, getString(R.string.default_radius)));		
		mapView.requestFocus();
		
	}
	
	public void onPause(){
		super.onPause();
		SharedPreferences.Editor editor = getSharedPreferences(PREFS_NAME, 0).edit();
		editor.putString(SETTING_RADIUS, ""+radiusView.getText());
		editor.commit();	
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.activity_location_rule, menu);
		return true;
	}

	@Override
	protected boolean isRouteDisplayed() {
		// TODO Auto-generated method stub
		return false;
	}
	
	private void saveRules(){
		int noRanges = itemOverlay.size();
		for(int r = 0; r < noRanges; r++){
			Rule rule = new Rule(Action.BLANK); //one new rule per GPS area
			PrivacyRange pr = (PrivacyRange) itemOverlay.getItem(r);
			LocationCondition lc = new LocationCondition(pr.getPoint().getLatitudeE6(),pr.getPoint().getLongitudeE6(),pr.getRadiusMeter());
			rule.addCondition(lc);
			//add rule to Privacy (where it will be registered with the server)
			Privacy.getInstance().addRule(rule);
		}	

	}
}


