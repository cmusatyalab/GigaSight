package cmu.location;

import java.util.ArrayList;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Point;
import android.graphics.drawable.Drawable;
import android.util.FloatMath;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.EditText;
import android.widget.Toast;

import com.google.android.maps.GeoPoint;
import com.google.android.maps.ItemizedOverlay;
import com.google.android.maps.MapView;
import com.google.android.maps.OverlayItem;
import com.google.android.maps.Projection;

public class PrivacyItemizedOverlay extends ItemizedOverlay {
	private static final String TAG = "PrivacyItemizedOverlay";
	//private ArrayList<OverlayItem> mOverlays = new ArrayList<OverlayItem>();
	private ArrayList<PrivacyRange> mOverlays = new ArrayList<PrivacyRange>();
	private Context mContext;
	private EditText mRadiusText;
	private Paint paintFill;
	private Paint paintLine;
	
	
	public PrivacyItemizedOverlay(Drawable defaultMarker, EditText mRadiusText) {
		super(boundCenterBottom(defaultMarker));
		this.mRadiusText = mRadiusText;
		this.mContext = mRadiusText.getContext();
		populate(); // hack needed to initiate the overlay without any items
					// http://code.google.com/p/android/issues/detail?id=2035

		paintFill = new Paint(Paint.ANTI_ALIAS_FLAG);
        paintFill.setColor(0x30000000);
        paintFill.setStyle(Style.FILL_AND_STROKE);

        paintLine = new Paint(Paint.ANTI_ALIAS_FLAG);
        paintLine.setColor(0x99000000);
        paintLine.setStyle(Style.STROKE);    
        
	}

	//public void addOverlay(OverlayItem overlay) {
	//	mOverlays.add(overlay);
	//	populate();
	//}
	
	public void addOverlay(PrivacyRange r){
		mOverlays.add(r);
		populate();
	}

	@Override
	protected OverlayItem createItem(int i) {
		return mOverlays.get(i);
	}

	@Override
	public int size() {
		return mOverlays.size();
	}

	// @Override
	public boolean onTap(GeoPoint p, MapView mapView) {
		//Log.d(TAG, "onTap");
		// let the superclass check if we hit an existing marker
		// if so, it will call onTap(int index)
		if (super.onTap(p, mapView))
			return true;

		// the event was not handled as a tap on an existing marker, so let's
		// add a new marker
		String text = mRadiusText.getText().toString();
		if(text.isEmpty())
			Toast.makeText(mContext,"empty radius!", Toast.LENGTH_SHORT).show();
		else{ 
			float radius = Float.parseFloat(mRadiusText.getText().toString());
			PrivacyRange r = new PrivacyRange(p, radius);
			addOverlay(r);
			mapView.invalidate();			
		}
		return true;
	}

	@Override
	// an existing marker was tapped
	public boolean onTap(int i) {
		Log.i(TAG, "marker tapped!");
		return true;
	}

	@Override
	public void draw(Canvas canvas, MapView mapView, boolean shadow) {
		//super.draw(canvas, mapView, shadow);
		
		//we do not need shadows
		if(!shadow){
			
			//Log.d(TAG,"draw");
			for(PrivacyRange r : mOverlays){
				//Log.d(TAG,"overlay");
				Point myPoint = new Point();
				Projection projection = mapView.getProjection();
				projection.toPixels(r.getPoint(),myPoint);				
				int radiusPixel = (int) projection.metersToEquatorPixels(r.getRadiusMeter());
				canvas.drawCircle(myPoint.x, myPoint.y, radiusPixel, paintFill);
				canvas.drawCircle(myPoint.x, myPoint.y, radiusPixel, paintLine);
	
			}
			super.draw(canvas, mapView, shadow); //draw the pins on top of the circles
		}		
	}

	//this should give more accurate radius of the circle...
	//http://stackoverflow.com/questions/2077054/how-to-compute-a-radius-around-a-point-in-an-android-mapview
	//however, this does not seem to work? Also check which order of latitude is needed (*1e6 or not?)
	public static int metersToRadius(float meters, Projection p, double latitude) {
		return (int) (p.metersToEquatorPixels(meters) * (1 / FloatMath.cos((float)Math.toRadians(latitude))));
	}
}
