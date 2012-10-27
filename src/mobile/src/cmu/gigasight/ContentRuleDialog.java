package cmu.gigasight;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import java.io.IOException;
import java.util.ArrayList;

import org.apache.http.Header;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.methods.HttpPost;

import cmu.privacy.Action;
import cmu.privacy.ContentCondition;
import cmu.privacy.Privacy;
import cmu.privacy.Rule;
import cmu.servercommunication.ServerResource;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.net.http.AndroidHttpClient;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;

public class ContentRuleDialog extends DialogFragment {
	private final static String TAG = "ContentRuleDialog";	
	private ArrayList<String> mSelected = new ArrayList<String>();
	
	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState) {
	
		// Use the Builder class for convenient dialog construction
		AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
		Log.d(TAG,"onCreateDialog");        
		builder.setTitle(R.string.dialog_pick_faces)
				.setPositiveButton(R.string.blur,
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int id) {
								Rule r = new Rule(Action.BLUR);
								ContentCondition cc = new ContentCondition(mSelected);
								r.addCondition(cc);
						    	//add rule to Privacy (where it will be registered with the server
						    	Privacy.getInstance().addRule(r);
						    	//callback to update GUI
								((PrivacyActivity) getActivity()).updatePrivacyView();								
							}
						})
				.setNegativeButton(R.string.cancel,
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int id) {
								// User cancelled the dialog, no changes to persist
							}
						})
			    .setMultiChoiceItems(ContentCondition.names, null, 
			    		new DialogInterface.OnMultiChoiceClickListener() {		    	
			    			public void onClick(DialogInterface dialog, int which, boolean isChecked) {
			    				if(isChecked){
			    					mSelected.add(ContentCondition.names[which]);
			    				} else if(mSelected.contains(ContentCondition.names[which]))
			    					mSelected.remove(ContentCondition.names[which]);
			    			}
			    		})
       			;
		// Create the AlertDialog object and return it
		return builder.create();
	}
	
	}

