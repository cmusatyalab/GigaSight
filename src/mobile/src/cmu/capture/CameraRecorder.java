package cmu.capture;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.List;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.Size;
import android.media.CamcorderProfile;
import android.media.MediaRecorder;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Environment;
import android.util.Log;

@SuppressLint("NewApi")
public class CameraRecorder {

	public final static String SETTING_CAMERA = "CAMERASETTINGS";
	public final static String PREF_RESOLUTION = "RESOLUTION";
	private final static HashMap<Integer,String> resolutionMap = createResolutionMap();	
	private Context mContext;
	private Camera mCamera;
	private static final String TAG = "CameraRecorder";
	private CameraPreview mCameraPreview;
	private MediaRecorder mRec;
	private int resolution;
	
	public boolean recording;
	
	private boolean recorderInitialized;
	
	
	public CameraRecorder(Context context){
		mContext = context;
		recorderInitialized = false;
		
	}
	public boolean initCameraRecorder(){
		//open the camera and create the preview
		//openCameraAndPreview performs the necessary cleanUp by itself if needed
		recorderInitialized = openCameraAndPreview();
		return recorderInitialized;
	}
	
	public Date startRecording(File outputFile){	
		recording = true;
		//we assume that initCameraRecorder has been called before!
		if(!recorderInitialized){
			Log.e(TAG,"Cannot start recorder, you should call initCameraRecorder first!");
			return null;
		}
		
		//prepare the recorder
		prepareMediaRecorder(outputFile);
		
		//start recording
		Date startTime = new Date();
		mRec.start();
		return startTime;
	}
	
	//this method is also the generic cleanUp method that should be called upon exceptions
	public Date stopRecording(){
		recording = false;
		Date stopTime = null;
		Log.i(TAG,"Stop recording");
			
		/* reset and release the MediaRecorder object */
		if(mRec!= null){
			stopTime = new Date();
			mRec.stop();
			mRec.reset();
			mRec.release();
		}
		mRec = null;	
		return stopTime;
		
	}

	public String getResolution(){
		//possible bug: by default, resolution is set to QUALITY_LOW, but this might not be in the list...
		return resolutionMap.get(resolution);
	}
	
	private void prepareMediaRecorder(File outputFile){
		
        Log.d(TAG,"Prepare recorder");

		mRec = new MediaRecorder();
		mRec.setOnErrorListener(new MediaRecorder.OnErrorListener() {
            public void onError(MediaRecorder mr, int what, int extra) {
            	Log.i(TAG, "Error"+" "+what+" "+extra);
            }
		});
		
		mCamera.unlock(); //not required for Android 4.1?		
		mRec.setCamera(mCamera);
		mRec.setAudioSource(MediaRecorder.AudioSource.CAMCORDER);
		mRec.setVideoSource(MediaRecorder.VideoSource.CAMERA);
		SharedPreferences prefs = mContext.getSharedPreferences(SETTING_CAMERA,0);
		resolution = prefs.getInt(PREF_RESOLUTION, CamcorderProfile.QUALITY_LOW);
		CamcorderProfile mCP = CamcorderProfile.get(resolution);
		//CamcorderProfile mCP = CamcorderProfile.get(CamcorderProfile.QUALITY_1080P);
		//CamcorderProfile mCP = CamcorderProfile.get(CamcorderProfile.QUALITY_HIGH);
		//CamcorderProfile mCP = CamcorderProfile.get(CamcorderProfile.QUALITY_LOW);
		
		//mCP.videoCodec = MediaRecorder.VideoEncoder.H263;
		//mCP.audioCodec = MediaRecorder.AudioEncoder.AMR_NB; //AMR_NB only with 3GPP
		//mCP.fileFormat = MediaRecorder.OutputFormat.THREE_GPP;
		
		mCP.fileFormat = MediaRecorder.OutputFormat.MPEG_4;		
		mRec.setProfile(mCP);
    	mRec.setOutputFile(outputFile.toString());
    	//Log.d(TAG, "Saving video in: "+outputFile.getAbsolutePath().toString());
    	mRec.setPreviewDisplay(mCameraPreview.getHolder().getSurface());
  
		try {
			mRec.prepare();
			Log.d(TAG,"Camera prepared");			
		} catch (IOException ex) {
			Log.e(TAG,"Camera could not be prepared");			
			ex.printStackTrace();
			stopRecording();			
		} catch (Exception e){
			e.printStackTrace();
			stopRecording();
		}
    	
		
	}


	public Camera getCamera(){
		return mCamera;
	}
	
	public CameraPreview getPreview(){
		return mCameraPreview;
	}
	
	public boolean openCameraAndPreview() {
	    boolean qOpened = false;
	    
	    try {
	        releaseCameraAndPreview();
	        mCamera = Camera.open();	        
	        qOpened = (mCamera != null); 
	    } catch (Exception e) {
	        Log.e(TAG, "failed to open Camera");
	        e.printStackTrace();
	    }

	    /* configure camera parameters to prepare it for recording... */ 	    
	    Parameters mParams = mCamera.getParameters();
        mParams.setRecordingHint(true);
        mCamera.setParameters(mParams);
          
      	//queryCameraParameters();    
	    
      	//the preview is set and started in the SurfaceChangedListener of this CameraPreview
	    mCameraPreview = new CameraPreview(mContext,mCamera);
	    return qOpened;    
	}
	
	public void releaseCameraAndPreview() {
		
		if(mCameraPreview!=null){
			//mCameraPreview.setCamera(null);
			mCameraPreview.stopPreview();
			mCameraPreview = null;
		}
	    if (mCamera != null) {
	        mCamera.release();
	        mCamera = null;
	    }
	}	
	
	//camera must be opened before calling this function
	private void queryCameraParameters(){
		List<Size> sizes = mCamera.getParameters().getSupportedPreviewSizes();
		Log.d(TAG,"No of supported sizes: " + sizes.size());
		for(Size s : sizes){
			Log.d(TAG,"Resolution: "+s.width+"x"+s.height);
		}
		
		List<int []> ranges = mCamera.getParameters().getSupportedPreviewFpsRange();
		for(int i = 0; i < ranges.size(); i++){
			Log.d(TAG,"min framerate: "+ranges.get(i)[0]);
			Log.d(TAG,"max framerate: "+ranges.get(i)[1]);

		}
	}
	
	//to do: link this to the Strings used in ResolutionDialog??
	//check if resolutions are supported...
	private static HashMap<Integer,String> createResolutionMap(){
		HashMap<Integer,String>resultMap = new HashMap<Integer,String>();
		resultMap.put(CamcorderProfile.QUALITY_QVGA, "320x240");
		resultMap.put(CamcorderProfile.QUALITY_480P, "720x480");
		resultMap.put(CamcorderProfile.QUALITY_720P, "1280x720");
		resultMap.put(CamcorderProfile.QUALITY_1080P, "1920x1080");
		return resultMap;
		
	}
}	