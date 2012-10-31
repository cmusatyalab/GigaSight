package cmu.gigasight;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.TreeMap;

import cmu.capture.CameraRecorder;
import cmu.servercommunication.ServerSettings;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.media.CamcorderProfile;
import android.os.Bundle;
import android.util.Log;
import android.util.SparseArray;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 


public class ResolutionDialog extends DialogFragment {
	private final static String TAG = "ResolutionDialog";	
	private TreeMap<Integer,String> mResolutions;
	private Integer [] mResIntArray;
	private String[] mResStringArray;
	private SharedPreferences settings;
	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState) {
	
		mResolutions = new TreeMap<Integer,String>();		
		//first check which camera resolutions are supported					
		if(CamcorderProfile.hasProfile(CamcorderProfile.QUALITY_1080P)){
			mResolutions.put(CamcorderProfile.QUALITY_1080P,"1080p - 1920x1080");
			
		}
		if(CamcorderProfile.hasProfile(CamcorderProfile.QUALITY_720P)){
			mResolutions.put(CamcorderProfile.QUALITY_720P,"720p - 1280x720");		
			
		}
		if(CamcorderProfile.hasProfile(CamcorderProfile.QUALITY_480P)){
			mResolutions.put(CamcorderProfile.QUALITY_480P,"480p - 720x480");
			
		}
		if(CamcorderProfile.hasProfile(CamcorderProfile.QUALITY_QVGA)){
			mResolutions.put(CamcorderProfile.QUALITY_QVGA,"QVGA - 320x240");	
			
		}
		
		mResStringArray = new String[mResolutions.values().size()];
		mResIntArray = new Integer [mResolutions.values().size()];
		mResolutions.values().toArray(mResStringArray); 
		mResolutions.keySet().toArray(mResIntArray);
		
		settings = getActivity().getSharedPreferences(CameraRecorder.SETTING_CAMERA,0);
		final SharedPreferences.Editor editor = settings.edit();
		
		//what is the current resolution - if none exists, chooses lowest resolution
		int currentResolution = settings.getInt(CameraRecorder.PREF_RESOLUTION, mResIntArray[mResIntArray.length - 1]);
		
		int flagged = 0;
		for(int i = 0; i < mResIntArray.length; i++){
			if(currentResolution == mResIntArray[i]){
				flagged = i;
				break;
			}
		}

						
		// Use the Builder class for convenient dialog construction
		AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
		Log.d(TAG,"onCreateDialog");        
		builder.setTitle(R.string.dialog_pick_resolution)
				.setPositiveButton("Close",
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int id) {
														
							}
						})				
			    .setSingleChoiceItems(mResStringArray, flagged, 
			    		new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog,int which) {	
								Log.i(TAG,"Capture resolution is "+mResStringArray[which]);
								editor.putInt(CameraRecorder.PREF_RESOLUTION, mResIntArray[which]);
								editor.commit();
							}		    	
			    			
			    		})
       			;
		// Create the AlertDialog object and return it
		return builder.create();
	}
	
	}

