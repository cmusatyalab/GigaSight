import java.util.ArrayList;

public class Experiment {

	CommunicationServer expServer;

	public Experiment(CommunicationServer expServer) {
		this.expServer = expServer;
	}

	public boolean initAllNodes() {
		//ArrayList<MobileNode> nodes = (ArrayList<MobileNode>) ExperimentManager.connectedNodes.clone();

		//for (MobileNode mn : nodes) {
		//	System.out.println("Asking to open receive port to " + mn.name);
		//	mn.send(ExperimentProtocol.OPEN_RECEIVE_PORT);
		//}

		int noNodesReady = 0;
		int noRetries = 0;
		while (noRetries < 10) {
			noNodesReady = 0;
			//for (MobileNode mn : nodes) {
			//	if (mn.fileReceivePort > 0) {
			//		System.out.println("Mobile node " + mn.name
			//				+ " will receive files on port: "
			//				+ mn.fileReceivePort);
			//		noNodesReady++;
			//	} 
			//}
			//if(noNodesReady == nodes.size()){
			//	System.out.println("all nodes ready!");
			//	return true;
			//}
			noRetries++;
			try {
				Thread.sleep(200);
			} catch (InterruptedException e) {				
				e.printStackTrace();
			}
		}
		
		return false;
	}
}
