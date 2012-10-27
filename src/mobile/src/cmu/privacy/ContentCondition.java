package cmu.privacy;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

import java.util.ArrayList;

public class ContentCondition extends Condition {

	public static final String [] names = {"Satya", "Pieter", "Yu", "Babu", "Kiryong"};
	ArrayList<String> data;
	
	public ContentCondition(){
		super(ConditionType.CONTENT);
	}
	
	public ContentCondition(ArrayList<String> users){
		super(ConditionType.CONTENT);
		data = users;
	}
	
	public ArrayList<String> getSelectedContent(){
		return data;
	}
	public void setSelected(ArrayList<String> newSelection){
		data = newSelection;
	}

	@Override
	protected String createDataString() {
		StringBuilder dataString = new StringBuilder();
		for(int i = 0; i < data.size() - 1; i++)
			dataString.append("\""+data.get(i)+"\",");
		dataString.append("\""+data.get(data.size()-1)+"\"");
		return dataString.toString();
	}

	@Override
	public String createFormattedText() {
		StringBuilder dataString = new StringBuilder();
		dataString.append("\ttype: CONTENT\n");
		dataString.append("\tobjects: ");
		for(int i = 0; i < data.size() - 1; i++)
			dataString.append(data.get(i)+", ");
		dataString.append(data.get(data.size()-1)+"\n");
		return dataString.toString();
	}
}
