package cmu.privacy;

public class TimeCondition extends Condition {

	String timeStart;
	String timeStop;
	public TimeCondition(String start, String stop) {
		super(ConditionType.TIMEFRAME);		
		timeStart = start;
		timeStop = stop;
	}

	@Override
	public String createFormattedText() {
		StringBuilder dataString = new StringBuilder();
		dataString.append("\ttype: TIMEFRAME\n");
		dataString.append("\tfrom: "+timeStart+"\n");
		dataString.append("\tuntil: "+timeStop+"\n");
		return dataString.toString();
	}

	@Override
	protected String createDataString() {
		StringBuilder dataString = new StringBuilder();
		dataString.append("{");
		dataString.append("\"time_start\" = \""+ timeStart + "\",");
		dataString.append("\"time_stop\" = \""+ timeStop + "\"");
		dataString.append("}");
		return dataString.toString();
	
	}

}
