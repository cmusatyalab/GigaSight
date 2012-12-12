import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;
import java.util.LinkedList;
import java.util.List;

public class MobileWorker implements Runnable {

	private List<MobileDataEvent> queue = new LinkedList<MobileDataEvent>();

	private CommunicationServer server;
	private SocketChannel socketChannel;
	private MobileNode mn; // datastructure
	private ByteBuffer inputData;
	

	public MobileWorker(CommunicationServer server, SocketChannel socketChannel) {
		this.server = server;
		this.socketChannel = socketChannel;
		this.mn = new MobileNode(socketChannel.socket().getInetAddress().getHostAddress(), this);
		inputData = ByteBuffer.allocate(500);
		mn.connected();
	}


	public void processData(byte[] data, int count) {
		byte[] dataCopy = new byte[count];
		System.arraycopy(data, 0, dataCopy, 0, count);
		System.out.println("Added bytes: "+count);
		synchronized (queue) {
			queue.add(new MobileDataEvent(dataCopy));
			queue.notify();
		}
	}

	@Override
	public void run() {
		MobileDataEvent dataEvent;

		while (true) {
			// Wait for data to become available
			synchronized (queue) {
				while (queue.isEmpty()) {
					try {
						queue.wait();
					} catch (InterruptedException e) {
					}
				}
				dataEvent = queue.remove(0);
			}

			processData(dataEvent);

		}

	}

	/* we received some data from the mobile node */
	private void processData(MobileDataEvent dataEvent) {
		//add data to input buffer
		System.out.println("received bytes: "+dataEvent.data.length);
		System.out.println("inputData position: "+inputData.position()+" inputData.limit: "+inputData.limit());
		inputData.put(dataEvent.data);
		
		
		inputData.flip(); // set buffer to read state, position = 0
		while (inputData.remaining() > 0) {
			inputData.mark(); // mark the beginning of the next opcode
			if (!processRequest()) {
				inputData.reset(); // request was not completely in buffer, so
									// reset position to beginning of request
				break;
			}
		}
		inputData.compact(); // clear all bytes of completed requests
		

	}

	public void notifyDisconnected() {
		mn.disconnected();
		this.mn = null;
	}

	private boolean processRequest() {
		
		int requestCode = inputData.get();
		System.out.println("Received opcode "+requestCode+" from " + mn.name);
		boolean result = false;
		switch (requestCode) {
		case ConfigurationProtocol.REQUEST_NAME: {
			result = sendRequestName();
			break;
		}
		case ConfigurationProtocol.REQUEST_UPLOAD_IP: {
			result = sendUploadIP();
			break;
		}
		case ConfigurationProtocol.REQUEST_UPLOAD_PORT: {
			result = sendUploadPort();
			break;
		}
		case ConfigurationProtocol.RECEIVE_PORT_OPENED: {
			result = handleReceivePortOpened();
			break;
		}
		case ConfigurationProtocol.RECEIVE_PORT_CLOSED: {
			result = handleReceivePortClosed();
			break;
		}
		case ConfigurationProtocol.UPLOAD_FINISHED: {
			result = handleUploadFinished();
			break;
		}
		default:
			System.out.println("Unknown requestCode!");
			result = false;
		}
		
		return result;
	}

	private boolean sendRequestName() {

		ByteBuffer outputData = ByteBuffer
				.allocate(1 + 4 + mn.name.getBytes().length);
		outputData.put(ConfigurationProtocol.SET_NAME);
		outputData.putInt(mn.name.getBytes().length);
		outputData.put(mn.name.getBytes());
		server.send(socketChannel, outputData.array());
		System.out.println("Assigned node name " + mn.name);
		
		return true;
	}

	private boolean sendUploadIP() {

		String uploadIP = mn.getVideoUploadIP();
		ByteBuffer outputData = ByteBuffer
				.allocate(1 + 4 + uploadIP.getBytes().length);
		outputData.put(ConfigurationProtocol.SET_UPLOAD_IP);
		outputData.putInt(uploadIP.getBytes().length);
		outputData.put(uploadIP.getBytes());
		server.send(socketChannel, outputData.array());
		//System.out.println("Transferred uploadIP " + uploadIP + " to " + mn.name);
		
		return true;
	}

	private boolean sendUploadPort() {

		ByteBuffer outputData = ByteBuffer.allocate(1 + 4);
		outputData.put(ConfigurationProtocol.SET_UPLOAD_PORT);
		outputData.putInt(mn.getVideoUploadPort());
		server.send(socketChannel, outputData.array());
		//System.out.println("Transferred uploadPort " + uploadPort + " to " + mn.name);
		
		//by definition of the protocol, we are now configured
		mn.configured();
		return true;
	}

	private boolean handleReceivePortOpened() {
		//System.out.println("handleReceivePortOpen");
		if(inputData.remaining() >= 4){
			mn.fileReceivePort = inputData.getInt();
			return true;
		}
		else
			return false;

	}

	private boolean handleReceivePortClosed(){
		mn.fileReceivePort = -1;
		return true;
	}
	
	private boolean handleUploadFinished(){
		mn.ready();
		return true;
	}
	
	public void send(byte[] data) {
		//System.out.println("MobileWorker is sending "+data[0]);
		server.send(socketChannel, data);
	}
}
