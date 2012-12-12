import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.nio.channels.spi.SelectorProvider;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public class CommunicationServer implements Runnable {
	// The host:port combination to listen on
	private InetAddress hostAddress;
	private int port;	

	// The channel on which we'll accept connections
	private ServerSocketChannel serverChannel;

	// The selector we'll be monitoring
	private Selector selector;

	// The buffer into which we'll read data when it's available
	private ByteBuffer readBuffer = ByteBuffer.allocate(8192);

	// A list of ChangeRequest instances
	private List changeRequests = new LinkedList();
	
	// Maps a SocketChannel to a list of ByteBuffer instances
	private Map pendingData = new HashMap();
	 
	//list of connected mobile nodes
	private HashMap<Integer,MobileWorker> connectedNodes = new HashMap<Integer,MobileWorker>();

	public CommunicationServer(InetAddress hostAddress, int port)
			throws IOException {
		this.hostAddress = hostAddress;
		this.port = port;
		this.selector = this.initSelector();	
	}

	private Selector initSelector() throws IOException {
		// Create a new selector
		Selector socketSelector = SelectorProvider.provider().openSelector();

		// Create a new non-blocking server socket channel
		this.serverChannel = ServerSocketChannel.open();
		serverChannel.configureBlocking(false);

		// Bind the server socket to the specified address and port
		InetSocketAddress isa = new InetSocketAddress(this.hostAddress,
				this.port);
		serverChannel.socket().bind(isa);

		// Register the server socket channel, indicating an interest in
		// accepting new connections
		serverChannel.register(socketSelector, SelectionKey.OP_ACCEPT);

		return socketSelector;
	}

	@Override
	public void run() {
		System.out.println("ExperimentServer started");
		while (true) {
			try {
				
				// Process any pending changes
		        synchronized(this.changeRequests) {
		          Iterator changes = this.changeRequests.iterator();
		          while (changes.hasNext()) {
		            ChangeRequest change = (ChangeRequest) changes.next();
		            switch(change.type) {
		            case ChangeRequest.CHANGEOPS:
		              SelectionKey key = change.socket.keyFor(this.selector);
		              key.interestOps(change.ops);
		            }
		          }
		          this.changeRequests.clear();
		        }
		        
				// Wait for an event one of the registered channels
				this.selector.select();

				// Iterate over the set of keys for which events are available
				Iterator<SelectionKey> selectedKeys = this.selector
						.selectedKeys().iterator();
				while (selectedKeys.hasNext()) {
					SelectionKey key = (SelectionKey) selectedKeys.next();
					selectedKeys.remove();

					if (!key.isValid()) {
						continue;
					}

					// Check what event is available and deal with it
					if (key.isAcceptable()) {
						this.accept(key);
					} else if (key.isReadable()) {
						this.read(key);
					} else if (key.isWritable()) {
			            this.write(key);
			          }
				}
			} catch (Exception e) {
				System.out.println("Exception "+e.getMessage());
				e.printStackTrace();
			}
		}

	}

	private void accept(SelectionKey key) throws IOException {
		// For an accept to be pending the channel must be a server socket
		// channel.
		ServerSocketChannel serverSocketChannel = (ServerSocketChannel) key
				.channel();

		// Accept the connection and make it non-blocking
		SocketChannel socketChannel = serverSocketChannel.accept();
		socketChannel.configureBlocking(false);

		// Register the new SocketChannel with our Selector, indicating
		// we'd like to be notified when there's data waiting to be read
		socketChannel.register(this.selector, SelectionKey.OP_READ);
		
		//we have a new mobile node connected
		System.out.println("New mobile node connected from "+socketChannel.socket().getInetAddress().getHostAddress());
		MobileWorker mobWorker = new MobileWorker(this,socketChannel);
		connectedNodes.put(socketChannel.hashCode(),mobWorker);
		new Thread(mobWorker).start();
	}

	private void read(SelectionKey key) throws IOException {
		SocketChannel socketChannel = (SocketChannel) key.channel();

		//from which mobile node comes this data?
		int hash = socketChannel.hashCode();
		boolean knownMobile = connectedNodes.containsKey(hash);
		
		if(!knownMobile){
			System.out.println("Unknown mobile node!!!");
		}
		
		// Clear out our read buffer so it's ready for new data
		this.readBuffer.clear();

		// Attempt to read off the channel
		int numRead;
		try {
			numRead = socketChannel.read(this.readBuffer);
		} catch (IOException e) {
			// The remote forcibly closed the connection, cancel
			// the selection key and close the channel.
			System.out.println("Channel closed");
			if(knownMobile)
				onDisconnect(socketChannel);
			key.cancel();
			socketChannel.close();				
			return;
		}

		if (numRead == -1) {
			// Remote entity shut the socket down cleanly. Do the
			// same from our end and cancel the channel.
			System.out.println("Channel closed");
			if(knownMobile)
				onDisconnect(socketChannel);
			key.channel().close();
			key.cancel();
			
			return;
		}

		// Hand the data off to the appropriate MobileWorker thread
		if(knownMobile){
			connectedNodes.get(hash).processData(this.readBuffer.array(), numRead);
		}		
	}

	public void send(SocketChannel socket, byte[] data) {
		synchronized (this.changeRequests) {
			// Indicate we want the interest ops set changed
			this.changeRequests.add(new ChangeRequest(socket,
					ChangeRequest.CHANGEOPS, SelectionKey.OP_WRITE));

			// And queue the data we want written
			synchronized (this.pendingData) {
				List queue = (List) this.pendingData.get(socket);
				if (queue == null) {
					queue = new ArrayList();
					this.pendingData.put(socket, queue);
				}
				System.out.println("Writing data queued "+data[0]);
				queue.add(ByteBuffer.wrap(data));
			}
		}

		// Finally, wake up our selecting thread so it can make the required
		// changes
		this.selector.wakeup();
	}
	
	private void write(SelectionKey key) throws IOException {
	    SocketChannel socketChannel = (SocketChannel) key.channel();

	    synchronized (this.pendingData) {
	      List queue = (List) this.pendingData.get(socketChannel);
	      
	      // Write until there's not more data ...
	      while (!queue.isEmpty()) {
	        ByteBuffer buf = (ByteBuffer) queue.get(0);
	        //System.out.println("Writing data to socket");
	        socketChannel.write(buf);	        
	        if (buf.remaining() > 0) {
	        	System.out.println("did not write all data!");
	          // ... or the socket's buffer fills up
	          break;
	        }
	        queue.remove(0);
	      }
	      
	      if (queue.isEmpty()) {
	        // We wrote away all data, so we're no longer interested
	        // in writing on this socket. Switch back to waiting for
	        // data.
	        key.interestOps(SelectionKey.OP_READ);
	      }
	    }
	  }
	
	private void onDisconnect(SocketChannel socketChannel){
		MobileWorker mw = connectedNodes.get(socketChannel.hashCode());
		System.out.println("disconnected node!");
		mw.notifyDisconnected();
		connectedNodes.remove(socketChannel.hashCode());		
	}	
}