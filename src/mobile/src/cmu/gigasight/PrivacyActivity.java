package cmu.gigasight;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import cmu.privacy.Action;
import cmu.privacy.Condition;
import cmu.privacy.Privacy;
import cmu.privacy.Rule;
import android.os.Bundle;
import android.app.Activity;
import android.app.FragmentManager;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.RadioButton;
import android.widget.TextView;
import android.support.v4.app.NavUtils;

public class PrivacyActivity extends Activity {

	private TextView rulesView;
	private Privacy p;
	private final static String TAG = "PrivacyActivity"; 
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_privacy_settings);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        rulesView = (TextView) findViewById(R.id.rules_view);
        p = Privacy.getInstance();
        
    }
    
    public void onResume(){
    	super.onResume();
    	updatePrivacyView();
    	
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.activity_privacy_settings, menu);
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

    public void startConstraintDialog(View v){
    	
    	if(v.getId() == R.id.button_addcontentrule){
    		
    		FragmentManager fm = getFragmentManager();
    		ContentRuleDialog ccd = new ContentRuleDialog();
    		ccd.show(fm,"contentrule");
    	}
    	if(v.getId() == R.id.button_addtimerule){
    		FragmentManager fm = getFragmentManager();
    		TimeRuleDialog dcd = new TimeRuleDialog();
    		dcd.show(fm,"timerule");    		
		    
    	}
    	if(v.getId() == R.id.button_addlocationrule){
    		Log.d(TAG,"Starting location rule");
    		Intent intent = new Intent(this, LocationRuleActivity.class);
			startActivity(intent);
    	}
    }
    
    public void deleteAllRules(View v){
    	p.clear();
    	updatePrivacyView();
    }
    
    public void changeDefaultPolicy(View v){
    	if(v.getId() == R.id.radio_publish)
    		p.setPolicy(Action.PUBLISH);
    	else if(v.getId() == R.id.radio_blank)
    		p.setPolicy(Action.BLANK);
    	else
    		Log.d(TAG,"Unknown view called changeDefaultPolicy");
    	updatePrivacyView();
    	
    }
    void updatePrivacyView(){
    	//update the view to show the new rule to the user
    	rulesView.setText(p.createFormattedText());   	
    }    
}
