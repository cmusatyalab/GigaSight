package cmu.privacy;

import java.util.ArrayList;

import cmu.servercommunication.RESTClient;
import cmu.servercommunication.ServerResource;

public class Privacy extends ServerResource {

	private static final Privacy instance = new Privacy();

	Action policy;
	ArrayList<Rule> rules; // note that the ArrayList is ordered!

	private Privacy() {
		policy = Action.PUBLISH;
		this.serverCollection = "/privacy";
		this.rules = new ArrayList<Rule>();
		RESTClient.post(this,true); //do this asynchronously
	}
	


	public static Privacy getInstance() {
		return instance;
	}

	public static void recreate(){
		getInstance().registered = false;
		RESTClient.post(getInstance(), true);
	}
	public void setPolicy(Action policy) {
		this.policy = policy;
		RESTClient.put(this,true);
	}

	// add rule to end of list and register immediately with server
	public void addRule(Rule r) {
		rules.add(r);
		RESTClient.put(this,true);
	}

	public void clear() {
		rules.clear();
		RESTClient.put(this,true);
	}

	@Override
	public String createJSON() {
		StringBuilder b = new StringBuilder();
		b.append("{");
		b.append("\"default\": \"" + policy.toString() + "\",");
		b.append("\"rules\": [");
		if (rules.size() > 0) {
			for (int i = 0; i < rules.size() - 1; i++)
				b.append(rules.get(i).createJSON() + ",");
			b.append(rules.get(rules.size() - 1).createJSON()); // no comma here
		}
		b.append("]");
		b.append("}");
		return b.toString();
	}

	public String createFormattedText() {
		// note: we assume that there is only 1 condition in each rule!
		StringBuilder b = new StringBuilder();
		b.append("DEFAULT POLICY: " + policy.toString().toUpperCase() + "\n");
		if (rules.size() > 0) {
			for (int i = 0; i < rules.size(); i++) {
				b.append("Rule " + i + "\n");
				Condition c = rules.get(i).conditions.get(0);
				b.append("\taction: "
						+ rules.get(i).getAction().toString().toUpperCase()
						+ "\n");
				b.append(c.createFormattedText());
			}
		} else
			b.append("No rules defined\n");
			return b.toString();
	}
}
