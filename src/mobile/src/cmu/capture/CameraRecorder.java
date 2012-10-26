package cmu.capture;


import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.media.CamcorderProfile;
import android.media.MediaRecorder;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Environment;
import android.util.Log;

@SuppressLint("NewApi")
public class CameraRecorder {

	private Context mContext;
	private Camera mCamera;
	private static final String TAG = "CameraRecorder";
	private CameraPreview mCameraPreview;
	private MediaRecorder mRec;
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
		//CamcorderProfile mCP = CamcorderProfile.get(CamcorderProfile.QUALITY_1080P);
		//CamcorderProfile mCP = CamcorderProfile.get(CamcorderProfile.QUALITY_HIGH);
		CamcorderProfile mCP = CamcorderProfile.get(CamcorderProfile.QUALITY_LOW);
		//mCP.videoCodec = MediaRecorder.VideoEncoder.H263;
		//mCP.audioCodec = MediaRecorder.AudioEncoder.AMR_NB; //AMR_NB only with 3GPP
		//mCP.fileFormat = MediaRecorder.OutputFormat.THREE_GPP;
		mCP.fileFormat = MediaRecorder.OutputFormat.MPEG_4;
		
		mRec.setProfile(mCP);

    	mRec.setOutputFile(outputFile.toString());
    	Log.i(TAG, "video will be saved to: "+outputFile.toString());
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
}	