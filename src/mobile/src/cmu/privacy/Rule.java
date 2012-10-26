package cmu.privacy;

import java.util.ArrayList;

public class Rule {

	private Action action;
	protected ArrayList<Condition> conditions;
	
	public Rule(Action action) {
		this.action = action;
		this.conditions = new ArrayList<Condition>();
	}
	
	public void addCondition(Condition cond){
		conditions.add(cond);
	}
	
	public Action getAction(){
		return action;
	}
	public String createJSON(){
		StringBuilder b = new StringBuilder();
		b.append("{");
		b.append("\"action\": \""+action.toString() +"\",");
		b.append("\"conditions\": [");
		for(int i = 0; i < conditions.size()-1; i++)
			b.append(conditions.get(i).createJSON()+",");
		b.append(conditions.get(conditions.size()-1).createJSON());
		b.append("]");
		b.append("}");
		
		return b.toString();
	}

}
