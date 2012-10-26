package cmu.privacy;

public abstract class Condition {

	String type;
	
	protected Condition(String type){
		this.type = type;
	}
	
	//create a nicely formatted text representation
	abstract public String createFormattedText();
	//create a data representation
	abstract protected String createDataString();
	
	public String createJSON(){
		String dataString = createDataString();
		StringBuilder b = new StringBuilder("{");
		b.append("\"type\": \""+type+"\",");
		b.append("\"data\": ["+ dataString + "]");
		b.append("}");
		
		return b.toString();
	}
	
	public String getType(){
		return type;
	}
}
