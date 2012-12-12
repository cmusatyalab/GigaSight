package com.example.cmu.experimentclient;

import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.app.Activity;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.graphics.Color;
import android.text.Editable;
import android.text.TextWatcher;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends Activity {

	private TextView nodeIP;
	private TextView nodeName;
	private TextView uploadIP;
	private TextView uploadPort;
	private EditText controllerIP;
	private EditText controllerPort;
	private TextView statusView;
	private TextView colorView;
	
	private ControllerClientv2 controllerClient;
	private Handler mHandler;
	
	public final static int SET_NODENAME = 0;
	public final static int UPDATE_STATUS = 1;
	public final static int SET_UPLOAD_IP = 2;
	public final static int SET_UPLOAD_PORT = 3;
	public final static int INIT_COMPLETE = 4;
	public final static int DISCONNECTED = 5;
	private final static String TAG = "MainActivity";
	private final static String PREFS_SETTINGS = "controllersettings";
	private final static String SET_CONTROLIP = "controllerip";
	private final static String SET_CONTROLPORT = "controllerport";
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        nodeIP = (TextView) findViewById(R.id.nodeip);
        nodeName = (TextView) findViewById(R.id.nodename);
        uploadIP = (TextView) findViewById(R.id.upload_ip);
        uploadPort = (TextView) findViewById(R.id.upload_port);
        controllerIP = (EditText) findViewById(R.id.expman_ip);
        controllerPort = (EditText) findViewById(R.id.expman_port);
        statusView = (TextView) findViewById(R.id.status);       
        colorView = (TextView) findViewById(R.id.colorView);
        
        SharedPreferences prefs = this.getSharedPreferences(PREFS_SETTINGS, 0);    	    	
    	controllerIP.setText(prefs.getString(SET_CONTROLIP, getString(R.string.def_controlip)));    	
    	controllerPort.setText(prefs.getString(SET_CONTROLPORT, getString(R.string.def_controlport)));
    	
        controllerIP.addTextChangedListener(new myTextWatcher());
        controllerPort.addTextChangedListener(new myTextWatcher());
        
        statusView.setMovementMethod(new ScrollingMovementMethod());
        mHandler = new Handler(){
        	public void handleMessage(Message msg){
        		switch(msg.what){
        		case SET_NODENAME:
        			nodeName.setText((String)msg.obj);
        			break;        		
        		case UPDATE_STATUS:
        			updateStatus((String)msg.obj);
        			break;        		
        		case SET_UPLOAD_IP:
        			uploadIP.setText((String) msg.obj);
        			break;
        		case SET_UPLOAD_PORT:
        			uploadPort.setText((String) msg.obj);
        			break;        		
        		case INIT_COMPLETE:
        			colorView.setBackgroundColor(Color.GREEN);
        			break;
        		case DISCONNECTED:{
        			colorView.setBackgroundColor(Color.RED);
        			nodeName.setText(getString(R.string.unknown));
        			uploadIP.setText(getString(R.string.unknown));
        			uploadPort.setText(getString(R.string.unknown));
        			//statusView.setText("");
        			updateStatus("ExperimentController disconnected");
        			break;
        		}
    		}	
        	}
        };
        
        
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.activity_main, menu);
        return true;
    }
    
    public void onPause(){
    	
    	controllerClient.stop();
    	controllerClient = null; //we cannot reuse Runnable objects
    	saveSettings();
    	super.onPause();
    	
    }
    
    public void onResume(){
    	super.onResume();
    	nodeIP.setText(getLocalIpAddress());
    	
    	//we must do the following to avoid that controllerPort opens with the IME keyboard because
    	//we just called setText
    	//findViewById(R.id.buttonConnect).requestFocus();
    	
    	controllerClient = new ControllerClientv2(this,mHandler);
    }
    
    private String getLocalIpAddress() {
		WifiManager wifiManager = (WifiManager) getSystemService(WIFI_SERVICE);
		WifiInfo wifiInfo = wifiManager.getConnectionInfo();
		int ipAddress = wifiInfo.getIpAddress();
		return String.format("%d.%d.%d.%d", (ipAddress & 0xff),
				(ipAddress >> 8 & 0xff), (ipAddress >> 16 & 0xff),
				(ipAddress >> 24 & 0xff));
	}
    
    public void connectToExpController(View v){
    	
    	EditText expConIP = (EditText) findViewById(R.id.expman_ip);
    	EditText expConPort = (EditText) findViewById(R.id.expman_port);
    	
    	if(controllerClient != null && !controllerClient.isRunning()){
    		controllerClient.setServer(""+expConIP.getText(), Integer.parseInt(""+expConPort.getText()));    		
    		new Thread(controllerClient).start();
    		
    	}
    	else
    		Log.d(TAG,"ControllerClient is already running!");
    }
    
    public void quit(View v){
    	this.finish();
    }
    private void updateStatus(String status){
    		statusView.setText(statusView.getText() + "\n" + status);
    		if(statusView.getLineCount() > 15){
    			String completeText = statusView.getText().toString();
    			for(int i = 0; i < 5; i++){
    				completeText = completeText.substring(completeText.indexOf("\n"));
    			}
    			statusView.setText(completeText);	
    		}
    }
    
    private class myTextWatcher implements TextWatcher{

		public void afterTextChanged(Editable arg0) {	
			//Log.d(TAG,"afterTextChanged");
		}

		public void beforeTextChanged(CharSequence arg0, int arg1, int arg2,
				int arg3) {			
			
		}

		public void onTextChanged(CharSequence arg0, int arg1, int arg2,
				int arg3) {	
			//Log.d(TAG,"onTextChanged");
			saveSettings();
			
		}
    	
    }
    
    private void saveSettings(){
    	SharedPreferences prefs = this.getSharedPreferences(PREFS_SETTINGS, 0);
    	Editor edit = prefs.edit();
    	Log.d(TAG,"Saved controllerIP: "+controllerIP.getText()+"");
    	edit.putString(SET_CONTROLIP, controllerIP.getText()+"");
    	Log.d(TAG,"Saved controllerport: "+controllerPort.getText()+"");
    	edit.putString(SET_CONTROLPORT, controllerPort.getText()+"");
    	if(!edit.commit())
    		Log.d(TAG,"WARNING: settings were not saved to disk!");
    	
    }
}
