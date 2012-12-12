import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.nio.channels.spi.SelectorProvider;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

public class UploadServerCommunicator implements Runnable {

	private String ip;
	private int port;
	private Selector selector;
	private SocketChannel socket;
	private List<ChangeRequest> pendingChanges = new LinkedList<ChangeRequest>();
	private boolean keepRunning;
	private ByteBuffer readBuffer = ByteBuffer.allocate(100);
	private ByteBuffer writeBuffer = ByteBuffer.allocate(100);

	public UploadServerCommunicator() throws IOException {
		this.ip = ExperimentConfiguration.DEFAULT_VIDEO_UPLOADIP;
		this.port = ExperimentConfiguration.DEFAULT_CONTROL_UPLOADPORT;
		this.selector = SelectorProvider.provider().openSelector();
		this.socket = this.initiateConnection();
	}

	@Override
	public void run() {
		keepRunning = true;

		while (keepRunning) {
			try {
				
				synchronized (this.pendingChanges) {
					//System.out.println("Pending change requests "+this.pendingChanges.size());
					Iterator<ChangeRequest> changes = this.pendingChanges
							.iterator();
					while (changes.hasNext()) {						
						ChangeRequest change = changes.next();
						switch (change.type) {
						case ChangeRequest.CHANGEOPS:
							SelectionKey key = change.socket.keyFor(this.selector);
							//System.out.println("Changed key" + key);
							key.interestOps(change.ops);
							break;
						case ChangeRequest.REGISTER:
							//System.out.println("REGISTERING KEY");
							change.socket.register(this.selector, change.ops);
							break;
						}
					}
					this.pendingChanges.clear();
				}
				if(this.selector.keys().size() == 0){
					keepRunning = false;
					break;
				}
				//System.out.println("Before select");
				this.selector.select();				
				//System.out.println("After select");
				Iterator<SelectionKey> selectedKeys = this.selector
						.selectedKeys().iterator();
				while (selectedKeys.hasNext()) {
					SelectionKey key = selectedKeys.next();
					if (!key.isValid())
						continue;

					if (key.isConnectable()) {
						//System.out.println("Key is connectable: "+key);
						this.finishConnection(key);
					} else if (key.isReadable()) {
						//System.out.println("Key is readable: "+key);
						this.read(key);
					} else if (key.isWritable()) {
						//System.out.println("Key is writable: "+key);
						this.write(key);
					}

				}
			} catch (Exception e) {
				e.printStackTrace();
				keepRunning = false;
			}
		}
		cleanUp();
		this.onDisconnect();
	}

	public void stop() {
		keepRunning = false;
	}

	public void cleanUp() {
		if(this.selector != null && this.selector.isOpen())
			try {
				this.selector.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
	}

	private SocketChannel initiateConnection() throws IOException {
		SocketChannel sockChannel = SocketChannel.open();
		sockChannel.configureBlocking(false);
		System.out.println("Initiating connection");
		try{
			sockChannel.connect(new InetSocketAddress(this.ip, this.port));
		} catch (IOException e){
			e.printStackTrace();
		}
		
		// queue a channel registration since the caller is not the selecting
		// thread
		synchronized (this.pendingChanges) {
			this.pendingChanges.add(new ChangeRequest(sockChannel,
					ChangeRequest.REGISTER, SelectionKey.OP_CONNECT));
		}

		return sockChannel;

	}

	private void finishConnection(SelectionKey key) {
		SocketChannel sockChannel = (SocketChannel) key.channel();
		try {
			sockChannel.finishConnect();
		} catch (IOException e) {
			e.printStackTrace();
			key.cancel();
			keepRunning = false;
			return;
		}
		
		//System.out.println("Setting key to READABLE in finishConnection"+key);
		key.interestOps(SelectionKey.OP_READ);
	}

	public void sendCommand(char c){
		byte [] data = {(byte) c};
		this.send(data);
	}
	
	public void sendString(String s){
		byte [] data = new byte[s.getBytes().length + 2];
		data[0] = 'C';
		System.arraycopy(s.getBytes(), 0, data, 1, s.getBytes().length);
		data[data.length-1] = '\n';
		this.send(data);
				
	}
	
	private void send(byte[] data) {
		synchronized (this.pendingChanges) {
			//System.out.println("Queueing data and adding ChangeRequest: "+data.length);
			// Indicate we want the interest ops set changed
			this.pendingChanges.add(new ChangeRequest(socket,
					ChangeRequest.CHANGEOPS, SelectionKey.OP_WRITE));
			// And queue the data we want written
			synchronized (this.writeBuffer) {
				writeBuffer.put(data);
			}
		}

		// Finally, wake up our selecting thread so it can make the required
		// changes
		this.selector.wakeup();
	}

	private void read(SelectionKey key) throws IOException {
		SocketChannel socketChannel = (SocketChannel) key.channel();

		// Clear out our read buffer so it's ready for new data
		this.readBuffer.clear();

		// Attempt to read off the channel
		int numRead;
		try {
			numRead = socketChannel.read(this.readBuffer);
		} catch (IOException e) {
			// The remote forcibly closed the connection, cancel
			// the selection key and close the channel.
			System.out.println("Server forcibly closed connection");
			key.cancel();
			socketChannel.close();
			return;
		}

		if (numRead == -1) {
			// Remote entity shut the socket down cleanly. Do the
			// same from our end and cancel the channel.
			System.out.println("Server closed connection");
			key.channel().close();
			key.cancel();
			return;
		}

		// dummy consumption of input data for now...
		processData(this.readBuffer.array(), numRead);

	}

	//There is some bug here, the write is sometimes called with 0 bytes to write...
	//must have something to do with setting the keys at the wrong time (while being blocked in the select call and
	//ChangeRequests are not yet registered ...)
	private void write(SelectionKey key) throws IOException {
		SocketChannel socketChannel = (SocketChannel) key.channel();

		synchronized (this.writeBuffer) {
			writeBuffer.flip();
			//System.out.println("Sending "+writeBuffer.remaining()+ " bytes to uploadserver");
			socketChannel.write(this.writeBuffer);
			if (this.writeBuffer.remaining() > 0) {
				System.out.println("did not write all data!");

			}
			writeBuffer.compact();
		}
			// We wrote away all data, so we're no longer interested
			// in writing on this socket. Switch back to waiting for
			// data.
			key.interestOps(SelectionKey.OP_READ);
	}

	private void onDisconnect() {
		ExperimentManager expMan = ExperimentManager.getInstance();
		expMan.notifyDisconnected(this);
	}

	private void processData(byte[] data, int count) {
		byte[] dataCopy = new byte[count];
		System.arraycopy(data, 0, dataCopy, 0, count);
		System.out.println("Bytes received from UploadServer: " + count);
	}

}
