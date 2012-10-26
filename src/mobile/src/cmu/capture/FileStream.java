package cmu.capture;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Locale;

import org.json.JSONObject;

public abstract class FileStream extends Stream {
	protected static final SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ", Locale.US);
	protected File f;
	protected boolean uploaded; // uploaded and no longer available on file
								// system!
	
	public FileStream(Container c, File f) {
		super(c);		
		this.f = f;
		uploaded = false;
	}
	
	public void setUploaded() {
		uploaded = true;
		f.delete();
		f = null;
	}
	
	public File getFile() {
		return f;
	}

	public int getFileSize() {
		if (f != null)
			return (int) f.length();

		return 0;
	}
	
	abstract public boolean open();
	abstract public boolean close();

}
