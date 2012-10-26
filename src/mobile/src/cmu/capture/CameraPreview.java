package cmu.capture;

import java.io.IOException;

import android.content.Context;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/** A basic Camera preview class */
public class CameraPreview extends SurfaceView implements SurfaceHolder.Callback {
	private static final String TAG = "CameraPreview";
    private SurfaceHolder mHolder;
    private Camera mCamera;
    private boolean cameraInitialized;
    
    public CameraPreview(Context context, Camera camera) {
        super(context);
        mCamera = camera;

        // Install a SurfaceHolder.Callback so we get notified when the
        // underlying surface is created and destroyed.
        mHolder = getHolder();
        mHolder.addCallback(this);
        // deprecated setting, but required on Android versions prior to 3.0
        //mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        
        cameraInitialized = false;
        
    }

    public void setCamera(Camera camera) {
        if (mCamera == camera) {
        	cameraInitialized = true;
        	return; }
        
        stopPreviewAndFreeCamera();        
        mCamera = camera;
        
        if (mCamera != null) {            
            try {
                mCamera.setPreviewDisplay(mHolder);
            } catch (IOException e) {
                e.printStackTrace();
            }
          

            /*
            Important: Call startPreview() to start updating the preview surface. Preview must 
            be started before you can take a picture.
            */
            mCamera.startPreview();
            cameraInitialized = true;
        }
    }
    
    public void stopPreviewAndFreeCamera() {

        if (mCamera != null) {
            cameraInitialized = false;            
            mCamera.setPreviewCallback(null);
            /*
              Call stopPreview() to stop updating the preview surface.
            */
            mCamera.stopPreview();
        
            /*
              Important: Call release() to release the camera for use by other applications. 
              Applications should release the camera immediately in onPause() (and re-open() it in
              onResume()).
            */
            mCamera.release();        
            mCamera = null;
        }
    }
    
    public void stopPreview() {

        if (mCamera != null) {
            cameraInitialized = false;            
            mCamera.setPreviewCallback(null);
            /*
              Call stopPreview() to stop updating the preview surface.
            */
            mCamera.stopPreview();
  
            //note: the camera is not released here, just as it was not opened in this class neither
            mCamera = null;
        }
    }
    //note that these surfaceCreated/... listeners seem to be called very early in the resume process.
    //If you do not use the cameraInitialized boolean, things crash when you stop the app in the Preview window by pressing the "home button".
    //It must have something to be done by the fact that there is already a listenered attached, although the camera is not yet initialized.
    //There are issues with this, if you google on "android cameraObj.setPreviewCallback(null);" you will learn that this might be a bug.
    
    //this method seems sometimes be called before the mCamera field has been initialzed...
    //somewhere a listener was not correctly deregistered...
    //Except for the catch-clauses for the nullpointer, all of the code below was taken from tutorials 
    //online, in that code you will see the comment "non-existent surfaceview - ignoring", so there might be an issue here...
    
    public void surfaceCreated(SurfaceHolder holder) {
        // The Surface has been created, now tell the camera where to draw the preview.
        try {
        	
        		mCamera.setPreviewDisplay(holder);
        		mCamera.startPreview();
        	
        } catch (IOException e) {
            Log.d(TAG, "Error setting camera preview: " + e.getMessage());
        } catch (NullPointerException e){
        	Log.d(TAG, "Excepted nullpointerexception, ignoring...");
        }
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        // empty. Take care of releasing the Camera preview in your activity.
    	
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        // If your preview can change or rotate, take care of those events here.
        // Make sure to stop the preview before resizing or reformatting it.
    	Log.d(TAG,"surfaceChanged");
        if (mHolder.getSurface() == null){
          // preview surface does not exist
          return;
        }

        // stop preview before making changes
        try {
        	
            mCamera.stopPreview();
        } catch (Exception e){
          // ignore: tried to stop a non-existent preview
        }

        // set preview size and make any resize, rotate or
        // reformatting changes here

        // start preview with new settings
        try {

        		mCamera.setPreviewDisplay(mHolder);
        		mCamera.startPreview();
        } catch (NullPointerException ne){
        	Log.d(TAG,"Expected nullpointerexception, ignoring...");
        } catch (Exception e){
            Log.d(TAG, "Error starting camera preview: " + e.getMessage());            
        }
    }
}
