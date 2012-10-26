package cmu.capture;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

import android.content.Context;
import android.location.Location;
import android.util.Log;

public class GPSstream extends FileStream implements Runnable {

	private static final String TAG = "GPSstream";
	private static final int TWO_MIN = 1000 * 60 * 2;	
	private Location lastLoc;	
	private FileOutputStream fos;
	private BlockingQueue<String> queue; // queue to insert files
	private final String STOP = "STOP"; // when GPS stream can be closed

	public GPSstream(File f) {
		super(Container.GPS,f);
		this.queue = new ArrayBlockingQueue<String>(20);
	}

	public boolean open() {
		try {
			fos = new FileOutputStream(f);
			new Thread(this).start();
			return true;
		} catch (FileNotFoundException e) {
			Log.w(TAG, "File not found!" + e.getMessage());
			return false;
		}

	}

	public boolean close() {
		queue.add(STOP);
		return true;
	}

	public void add(Location l) {
		if (isBetterLocation(l, lastLoc)) {
			String newLoc = sdf.format(l.getTime()) + ";" + l.getLatitude()
					+ ";" + l.getLongitude() + "\n";
			queue.add(newLoc);
		}
	}

	/**
	 * Determines whether one Location reading is better than the current
	 * Location fix
	 * 
	 * @param location
	 *            The new Location that you want to evaluate
	 * @param currentBestLocation
	 *            The current Location fix, to which you want to compare the new
	 *            one
	 */
	protected boolean isBetterLocation(Location location,
			Location currentBestLocation) {
		if (currentBestLocation == null) {
			// A new location is always better than no location
			return true;
		}

		// Check whether the new location fix is newer or older
		long timeDelta = location.getTime() - currentBestLocation.getTime();
		boolean isSignificantlyNewer = timeDelta > TWO_MIN;
		boolean isSignificantlyOlder = timeDelta < -TWO_MIN;
		boolean isNewer = timeDelta > 0;

		// If it's been more than two minutes since the current location, use
		// the new location
		// because the user has likely moved
		if (isSignificantlyNewer) {
			return true;
			// If the new location is more than two minutes older, it must be
			// worse
		} else if (isSignificantlyOlder) {
			return false;
		}

		// Check whether the new location fix is more or less accurate
		int accuracyDelta = (int) (location.getAccuracy() - currentBestLocation
				.getAccuracy());
		boolean isLessAccurate = accuracyDelta > 0;
		boolean isMoreAccurate = accuracyDelta < 0;
		boolean isSignificantlyLessAccurate = accuracyDelta > 200;

		// Check if the old and new location are from the same provider
		boolean isFromSameProvider = isSameProvider(location.getProvider(),
				currentBestLocation.getProvider());

		// Determine location quality using a combination of timeliness and
		// accuracy
		if (isMoreAccurate) {
			return true;
		} else if (isNewer && !isLessAccurate) {
			return true;
		} else if (isNewer && !isSignificantlyLessAccurate
				&& isFromSameProvider) {
			return true;
		}
		return false;
	}

	/** Checks whether two providers are the same */
	private boolean isSameProvider(String provider1, String provider2) {
		if (provider1 == null) {
			return provider2 == null;
		}
		return provider1.equals(provider2);
	}

	public void run() {

		while (true) {
			try {
				String element = queue.take();
				if (element.equals(STOP)) {
					Log.d(TAG, "End of GPSstream file write thread");
					break; // out of while loop
				} else {
					fos.write(element.getBytes());

				}
			} catch (InterruptedException e) {
				Log.w(TAG, "InterruptedException " + e.getMessage());
				break;
			} catch (IOException e) {
				Log.w(TAG, "IOException " + e.getMessage());
				break;
			}
		}
		try {
			fos.close();
		} catch (IOException e) {
			Log.w(TAG, "IOException " + e.getMessage());
		}

	}
}
