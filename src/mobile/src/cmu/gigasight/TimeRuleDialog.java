package cmu.gigasight;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import cmu.privacy.Action;
import cmu.privacy.ContentCondition;
import cmu.privacy.Privacy;
import cmu.privacy.Rule;
import cmu.privacy.TimeCondition;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnShowListener;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

public class TimeRuleDialog extends DialogFragment {
	private static final String TAG = "TimeRuleDialog";
	private View v;
	EditText startHour;
	EditText startMin;
	EditText stopHour;
	EditText stopMin;


	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState) {
		Log.d(TAG, "onCreateDialog");
		// Use the Builder class for convenient dialog construction
		AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
		// Get the layout inflater
		LayoutInflater inflater = getActivity().getLayoutInflater();
		// Inflate and set the layout for the dialog
		v = inflater.inflate(R.layout.dialog_timeconstraintdialog, null);
		// Pass null as the parent view because its going in the dialog layout
		builder.setView(v);

		startHour = (EditText) v
				.findViewById(R.id.edit_start_hour);
		startMin = (EditText) v
				.findViewById(R.id.edit_start_min);
		stopHour = (EditText) v
				.findViewById(R.id.edit_stop_hour);
		stopMin = (EditText) v.findViewById(R.id.edit_stop_min);

		startHour.addTextChangedListener(new JumpFocusToView(startMin));
		startMin.addTextChangedListener(new JumpFocusToView(stopHour));
		stopHour.addTextChangedListener(new JumpFocusToView(stopMin));
		startHour.requestFocus();

		builder.setTitle(R.string.dialog_settime)
				.setPositiveButton(R.string.blank,
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int id) {
								if(!validateInput())
									return; //invalid input
								Rule r = new Rule(Action.BLANK);
								String startTime = startHour.getText().toString() + ":" + startMin.getText().toString();
								String stopTime = stopHour.getText().toString()+ ":" + stopMin.getText().toString(); 
								TimeCondition tc = new TimeCondition(startTime,stopTime);
								r.addCondition(tc);
								// add rule to Privacy (where it will be
								// registered with the server
								Privacy.getInstance().addRule(r);
								// callback to update GUI
								((PrivacyActivity) getActivity()).updatePrivacyView();
							}
						})
				.setNegativeButton(R.string.cancel,
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int id) {
								// User cancelled the dialog, no changes to
								// persist
							}
						});

		// return the dialog
		return builder.create();
	}
	
	private boolean validateInput(){
		int startH = Integer.parseInt(startHour.getText().toString());
		int stopH = Integer.parseInt(stopHour.getText().toString());
		int startM = Integer.parseInt(startMin.getText().toString());
		int stopM = Integer.parseInt(stopMin.getText().toString());

		String errorDescription = "";
		if(stopH > startH)
			return true;
		if((stopH == startH) && (stopM >= startM))
			return true;
		
		errorDescription = "End time is before start time";
		
		AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
		builder.setMessage("Invalid input: "+errorDescription)
			   .setPositiveButton("OK", new DialogInterface.OnClickListener() {			
           public void onClick(DialogInterface dialog, int id) {
        	   dialog.dismiss();
           }
       });
		AlertDialog dialog = builder.create();
		dialog.show();
		return false;
	}

	class JumpFocusToView implements TextWatcher {
		View v; // the view to jump to when 2 characters have been filled
		int noToJump = 2;

		JumpFocusToView(View v) {
			this.v = v;
		}

		public void afterTextChanged(Editable s) {
			// TODO Auto-generated method stub

		}

		public void beforeTextChanged(CharSequence s, int start, int count,
				int after) {
			// TODO Auto-generated method stub

		}

		public void onTextChanged(CharSequence s, int start, int before,
				int count) {
			if (s.length() == noToJump)
				v.requestFocus();
		}
	}
}
