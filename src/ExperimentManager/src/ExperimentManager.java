import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.channels.Channel;
import java.nio.file.DirectoryStream;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.ButtonGroup;
import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.SwingConstants;
import javax.swing.SwingUtilities;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.text.BadLocationException;

import com.jcraft.jsch.JSch;
import com.jcraft.jsch.JSchException;
import com.jcraft.jsch.Session;
import com.jcraft.jsch.UserInfo;
import com.jcraft.jsch.ChannelExec;

public class ExperimentManager {

	private ArrayList<MobileNode> connectedNodes = new ArrayList<MobileNode>(); // all
																				// nodes,
																				// possibly
																				// added
																				// after
																				// start
																				// of
																				// experiment
	private ArrayList<MobileNode> target = new ArrayList<MobileNode>(); // the
																		// nodes
																		// participating
																		// in
																		// the
	// experiment
	private int currentIteration = 0;

	private final GUIFrame gui = new GUIFrame();

	private static ChannelExec energyChannel = null; //needed to keep channel open
	private static ExperimentManager instance = null;
	private static CommunicationServer commServ;
	private static UploadServerCommunicator uploadComm;
	private static String[] _480pUploads = { "480p_5_Pittsburgh",
			"480p_7_Walnut" };
	private static String[] _1080pUploads = { "1080p_4_Pittsburgh",
			"1080p_6_Pittsburgh" };
	private static String[] _480pUploads_1fps = { "480p_5_Pittsburgh_1fps",
			"480p_7_Walnut_1fps" };
	private static String[] _1080pUploads_1fps = { "1080p_4_Pittsburgh_1fps",
			"1080p_6_Pittsburgh_1fps" };

	private static int uploadCounter = 0;

	public static ExperimentManager getInstance() {
		if (instance == null) {
			instance = new ExperimentManager();
		}
		return instance;
	}

	public static void main(String[] args) {

		final ExperimentManager expManager = getInstance();
		// start GUI
		SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				expManager.gui.createAndShow();
			}
		});

		// start Communication Server
		CommunicationServer serv = null;
		try {
			serv = new CommunicationServer(null, 9090);
		} catch (IOException e1) {
			e1.printStackTrace();
		}
		new Thread(serv).start();

		// start communication with upload server
		try {
			uploadComm = new UploadServerCommunicator();
			new Thread(uploadComm).start();
		} catch (IOException e1) {
			e1.printStackTrace();
		}
		/*
		 * System.out.println("Waiting for nodes to connect before proceeding");
		 * while (expManager.connectedNodes.size() < 1) {
		 * System.out.println("No nodes connected!"); try { Thread.sleep(1500);
		 * } catch (InterruptedException e) { e.printStackTrace(); } }
		 * 
		 * BufferedReader bufferRead = new BufferedReader(new InputStreamReader(
		 * System.in)); try {
		 * System.out.println("Press enter to start experiment");
		 * bufferRead.readLine(); } catch (IOException e) { // TODO
		 * Auto-generated catch block e.printStackTrace(); } Experiment exp =
		 * new Experiment(serv); System.out.println("Initializing all nodes");
		 * 
		 * if (exp.initAllNodes())
		 * System.out.println("Starting to send files to all nodes"); else
		 * System.out.println("Init of nodes failed!");
		 */
	}

	public void register(MobileNode mn) {
		// request status panel from GUI
		mn.setGUIPanel(gui.reservePanel());
		synchronized (connectedNodes) {
			connectedNodes.add(mn);
		}
	}

	public void deregister(MobileNode mn) {
		synchronized (connectedNodes) {
			if (connectedNodes.contains(mn)) {
				connectedNodes.remove(mn);
				gui.freePanel(mn.getGUIPanel());
			}
		}
	}

	public void notifyFinished(MobileNode mn) {
		if (ExperimentConfiguration.experimentType != ExperimentConfiguration.ALL_IDENTICAL) {
			if (mn.ip.equals(ExperimentConfiguration.REFERENCE_NODE)) {
				// stop the experiment if the reference node has finished its
				// uploads
				stop();
			}
		}
	}

	public void init() {
		currentIteration = 0;
		new Thread(new InitExperiment()).start();
	}

	public void start() {
		new Thread(new StartExperiment()).start();
	}

	public void stop() {
		new Thread(new StopExperiment()).start();
	}

	protected ExperimentManager() {
		// Exists only to defeat instantiation.
	}

	public void notifyDisconnected(Runnable r) {
		if (r instanceof UploadServerCommunicator) {
			gui.log("Communication with UploadServer disconnected!");
			((UploadServerCommunicator) r).stop();
		}
	}

	private String[] getFileList(MobileNode mn) {

		ArrayList<String> files = new ArrayList<String>();

		// ugly hack since we added possibility of not uploading later in the
		// code development process
		if (!ExperimentConfiguration.bDoUpload) {
			String[] empty = new String[0];
			return empty;
		}
		String video = "";

		if (ExperimentConfiguration.experimentType == ExperimentConfiguration.ALL_IDENTICAL) {
			video = ExperimentConfiguration.video;

		} else if (ExperimentConfiguration.experimentType == ExperimentConfiguration.ALLBUTONE_480p
				|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_480p
				|| ExperimentConfiguration.experimentType == ExperimentConfiguration.SLOTTED_ALLBUTONE_480p) {
			if (mn.ip.equals(ExperimentConfiguration.REFERENCE_NODE)) {
				video = ExperimentConfiguration.video;
			} else { // loop over all other videos of the same resolution
				video = _480pUploads[uploadCounter];
				uploadCounter = (uploadCounter + 1) % _480pUploads.length;
			}
		} else if (ExperimentConfiguration.experimentType == ExperimentConfiguration.ALLBUTONE_1080p
				|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_1080p
				|| ExperimentConfiguration.experimentType == ExperimentConfiguration.SLOTTED_ALLBUTONE_1080p) {
			if (mn.ip.equals(ExperimentConfiguration.REFERENCE_NODE)) {
				video = ExperimentConfiguration.video;
			} else { // loop over all other videos of the same resolution
				video = _1080pUploads[uploadCounter];
				uploadCounter = (uploadCounter + 1) % _1080pUploads.length;

			}
		} else if (ExperimentConfiguration.experimentType == ExperimentConfiguration.ALLBUTONE_480p_1fps
				|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_480p_1fps
				|| ExperimentConfiguration.experimentType == ExperimentConfiguration.SLOTTED_ALLBUTONE_480p_1fps) {
			if (mn.ip.equals(ExperimentConfiguration.REFERENCE_NODE)) {
				video = ExperimentConfiguration.video;
			} else { // loop over all other videos
				video = _480pUploads_1fps[uploadCounter];
				uploadCounter = (uploadCounter + 1) % _480pUploads_1fps.length;
			}
		} else if (ExperimentConfiguration.experimentType == ExperimentConfiguration.ALLBUTONE_1080p_1fps
				|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_1080p_1fps
				|| ExperimentConfiguration.experimentType == ExperimentConfiguration.SLOTTED_ALLBUTONE_1080p_1fps) {
			System.out.println("we are here!");
			if (mn.ip.equals(ExperimentConfiguration.REFERENCE_NODE)) {
				video = ExperimentConfiguration.video;
			} else { // loop over all other videos
				video = _1080pUploads_1fps[uploadCounter];
				uploadCounter = (uploadCounter + 1) % _1080pUploads_1fps.length;
			}
		}

		System.out.println("Sending video " + video + "_"
				+ ExperimentConfiguration.chunk_sec + "s to node " + mn.ip);
		try (DirectoryStream<Path> stream = Files
				.newDirectoryStream(FileSystems.getDefault().getPath(
						ExperimentConfiguration.baseDir, video,
						ExperimentConfiguration.chunk_sec + "s"))) {

			for (Path file : stream) {
				files.add("" + file.toString());

			}
		} catch (IOException e) {
			e.printStackTrace();
			return null;
		}

		Collections.sort(files, new Comparator<String>() {
			public int compare(String a, String b) {
				// System.out.println("comparing "+a+" with "+b);
				return getNumber(a) - getNumber(b);
			}

			private int getNumber(String s) {
				return Integer.parseInt(s.substring(s.lastIndexOf("_") + 1,
						s.indexOf(".")));
			}
		});

		// now reduce the list to avoid unnecessary uploads
		int noChunks = Math.min(files.size(),
				ExperimentConfiguration.duration_sec
						/ ExperimentConfiguration.chunk_sec);
		String[] array = new String[noChunks];
		for (int i = 0; i < noChunks; i++)
			array[i] = files.get(i);
		return array;
	}

	private class InitExperiment implements Runnable {
		@Override
		public void run() {
			ArrayList<MobileNode> candidates = new ArrayList<MobileNode>();
			currentIteration = 0;
			uploadCounter = 0;

			synchronized (connectedNodes) {
				candidates = (ArrayList<MobileNode>) connectedNodes.clone();
			}
			target.clear();
			ExperimentConfiguration.no_nodes = ExperimentConfiguration.no_nodes_max;
			if (candidates.size() >= ExperimentConfiguration.no_nodes) {
				for (int i = 0; i < ExperimentConfiguration.no_nodes; i++) {
					target.add(candidates.get(i));
				}
			} else {
				gui.log("Not enough nodes connected!");
				return;
			}

			gui.log("Initializing " + target.size() + " nodes");

			gui.log("Uploading chunks of " + ExperimentConfiguration.chunk_sec
					+ " sec");

			// upload - sequentially
			int noNodes = target.size();
			int counter = 0;
			int interval = (int)((ExperimentConfiguration.chunk_sec*1000.0)/(1.0*noNodes));
			for (MobileNode mn : target) {				
				// set chunk size
				mn.sendChunkSize(ExperimentConfiguration.chunk_sec);
				// set random start interval or not
				if (ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_480p
						|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_1080p
						|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_480p_1fps
						|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_1080p_1fps) {
					mn.sendRandomStartInterval();
				} else if (ExperimentConfiguration.experimentType == ExperimentConfiguration.SLOTTED_ALLBUTONE_1080p ||
						ExperimentConfiguration.experimentType == ExperimentConfiguration.SLOTTED_ALLBUTONE_480p ||
						ExperimentConfiguration.experimentType == ExperimentConfiguration.SLOTTED_ALLBUTONE_480p_1fps ||
						ExperimentConfiguration.experimentType == ExperimentConfiguration.ALLBUTONE_1080p_1fps){
					gui.log("Node "+mn.ip+" will start with delay "+interval*counter);
					mn.sendStartDelay(interval*counter);
				}
				// set random start chunk if needed
				if (ExperimentConfiguration.experimentType != ExperimentConfiguration.ALL_IDENTICAL) {
					if (!mn.ip.equals(ExperimentConfiguration.REFERENCE_NODE)) {
						mn.sendLoopUploads(true);
						mn.sendStartRandomChunk();
					} else {
						mn.sendLoopUploads(false);
					}
				} else {
					// set looping over uploads as configured in gui
					mn.sendLoopUploads(ExperimentConfiguration.bLoopUpload);
				}
				final String[] fileList = getFileList(mn);
				if (fileList != null) {
					mn.sendFiles(fileList, true); // for now: always erase all
													// previous files on
													// MobileNode
				}

				counter++;
			}	
			
			gui.log("Initialization completed!");

		}
	}

	private class StartExperiment implements Runnable {
		@Override
		public void run() {

			// synchronized (connectedNodes) {
			// target = (ArrayList<MobileNode>) connectedNodes.clone();
			// }

			String expDescription = ExperimentConfiguration.resolution + "_"
					+ ExperimentConfiguration.chunk_sec + "_" + "chop.mp4"
					+ "_" + ExperimentConfiguration.no_nodes + "_"
					+ currentIteration;
			uploadComm.sendString(expDescription);
			gui.log(expDescription);

			uploadComm.sendCommand('S');
			gui.log("UploadServer started");

			// clean the log at the upload server
			try {
				JSch jsch = new JSch();
				gui.log("Clearing log of uploadserver");
				Session session = jsch.getSession("cloudlet",
						ExperimentConfiguration.DEFAULT_VIDEO_UPLOADIP);
				session.setUserInfo(new MyUserInfo());
				session.connect();
				ChannelExec channel = (ChannelExec) session.openChannel("exec");
				channel.setCommand("cat /dev/null > /tmp/cloud_server.log");
				InputStream in = channel.getInputStream();
				channel.connect();
				byte[] tmp = new byte[1024];
				while (true) {
					while (in.available() > 0) {
						int i = in.read(tmp, 0, 1024);
						if (i < 0)
							break;
						System.out.print(new String(tmp, 0, i));
					}
					if (channel.isClosed()) {
						System.out.println("Exit status: "
								+ channel.getExitStatus());
						break;
					}
					try {
						Thread.sleep(100);
					} catch (Exception ee) {
					}
					;
				}
			} catch (JSchException e) {
				gui.log(e.getMessage());
				e.printStackTrace();
			} catch (IOException e) {
				gui.log(e.getMessage());
			}

			// start tshark log
			try {
				JSch jsch = new JSch();
				gui.log("Starting Tshark");
				Session session = jsch.getSession("cloudlet",
						ExperimentConfiguration.DEFAULT_VIDEO_UPLOADIP);
				session.setUserInfo(new MyUserInfo());
				session.connect();
				ChannelExec channel = (ChannelExec) session.openChannel("exec");
				String fileName = ExperimentConfiguration.video + "_"
						+ ExperimentConfiguration.chunk_sec + "_"
						+ ExperimentConfiguration.no_nodes + "_"
						+ currentIteration + "_tshark.log";

				channel.setCommand("nohup bash ./startTshark.sh "+fileName+" > /dev/null 2>&1 &");
				gui.log("Saving tshark log to "+fileName);
	
				InputStream in = channel.getInputStream();
				channel.connect();
				byte[] tmp = new byte[1024];
				while (true) {
					while (in.available() > 0) {
						int i = in.read(tmp, 0, 1024);
						if (i < 0)
							break;
						System.out.print(new String(tmp, 0, i));
					}
					if (channel.isClosed()) {
						System.out.println("Exit status: "
								+ channel.getExitStatus());
						break;
					}
					try {
						Thread.sleep(100);
					} catch (Exception ee) {
					}
					;
				}
			} catch (JSchException e) {
				gui.log(e.getMessage());
				e.printStackTrace();
			} catch (IOException e) {
				gui.log(e.getMessage());
			}

			//start energy measurement
			try {
				JSch jsch = new JSch();
				jsch.addIdentity("/home/pieter/.ssh/id_rsa");
				Session session = jsch.getSession("cloudlet", ExperimentConfiguration.ENERGYMON_IP);
				session.setUserInfo(new MyUserInfo());
				session.connect();
				energyChannel = (ChannelExec) session.openChannel("exec");
				String logName = "energy_" + ExperimentConfiguration.video + "_"
						+ ExperimentConfiguration.chunk_sec + "_"
						+ ExperimentConfiguration.no_nodes + "_"
						+ currentIteration ;
				
				String command = "\"C:\\Program Files (x86)\\Monsoon Solutions Inc\\Power Monitor" +
					"\\PowerToolCmd\" /TRIGGER=ATYD"+(ExperimentConfiguration.duration_sec - 15)+"A " +
					"/SAVEFILE="+ logName + ".pt4 " +
					"/KEEPPOWER /VOUT=3.89 /NOEXITWAIT";
				energyChannel.setCommand(command);
				InputStream in = energyChannel.getInputStream();
				energyChannel.connect();
				gui.log("Energy measurement started");
				//byte[] tmp = new byte[1024];
				/*int noretries = 0;
				while (noretries < 10) {
					while (in.available() > 0) {
						int i = in.read(tmp, 0, 1024);
						if (i < 0)
							break;
						String temp = new String(tmp, 0, i);
						System.out.println(temp);
						if(temp.contains("Waiting For Completion")){
							System.out.println("Energy measurement started"); //won't be visible in output since we start in background
							noretries = 999;
							break;
						}
					}
					try {
						Thread.sleep(100);
					} catch (Exception ee) {
					}
					noretries++;
				}
				if(noretries == 1000)
					gui.log("Energy measurement started");
				else if (noretries == 10)
					gui.log("No energy measurement!!!");
					*/
			} catch (JSchException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			
			
			
			//experiment ready for take off!
			gui.log("Experiment started, iteration no: " + currentIteration
					+ "/" + (ExperimentConfiguration.no_iters - 1));
			for (MobileNode mn : target) {
				mn.startExperiment();
			}

		}
	}

	private class StopExperiment implements Runnable {
		@Override
		public void run() {
			// uploadComm.sendCommand('X');
			// uploadComm.sendString("Experiment stopped");
			// gui.log("UploadServer stopped");

			// synchronized (connectedNodes) {
			// target = (ArrayList<MobileNode>) connectedNodes.clone();
			// }
			
			gui.log("Waiting one more chunk sec to stop experiment...");
			try {
				Thread.sleep(ExperimentConfiguration.chunk_sec*1000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			gui.log("Experiment stopped");
			
			for (MobileNode mn : target) {
				mn.stopExperiment();
			}

			// rename tshark log

			try {
				JSch jsch = new JSch();
				Session session = jsch.getSession("cloudlet",
						ExperimentConfiguration.DEFAULT_VIDEO_UPLOADIP);
				session.setUserInfo(new MyUserInfo());
				session.connect();
				ChannelExec channel = (ChannelExec) session.openChannel("exec");
				channel.setCommand("sudo pkill tshark");			
				InputStream in = channel.getInputStream();
				channel.connect();
				byte[] tmp = new byte[1024];
				while (true) {
					while (in.available() > 0) {
						int i = in.read(tmp, 0, 1024);
						if (i < 0)
							break;
						System.out.print(new String(tmp, 0, i));
					}
					if (channel.isClosed()) {
						System.out.println("Exit status: "
								+ channel.getExitStatus());
						break;
					}
					try {
						Thread.sleep(100);
					} catch (Exception ee) {
					}
					;
				}
				gui.log("Tshark log saved");
			} catch (JSchException e) {
				gui.log(e.getMessage());
				e.printStackTrace();
			} catch (IOException e) {
				gui.log(e.getMessage());
			}

			// rename log
			try {
				JSch jsch = new JSch();
				String fileName = ExperimentConfiguration.video + "_"
						+ ExperimentConfiguration.chunk_sec + "_"
						+ ExperimentConfiguration.no_nodes + "_"
						+ currentIteration + ".log";

				Session session = jsch.getSession("cloudlet",
						ExperimentConfiguration.DEFAULT_VIDEO_UPLOADIP);
				session.setUserInfo(new MyUserInfo());
				session.connect();
				ChannelExec channel = (ChannelExec) session.openChannel("exec");
				channel.setCommand("scp /tmp/cloud_server.log " + "/tmp/"
						+ fileName);
				InputStream in = channel.getInputStream();
				channel.connect();
				byte[] tmp = new byte[1024];
				while (true) {
					while (in.available() > 0) {
						int i = in.read(tmp, 0, 1024);
						if (i < 0)
							break;
						System.out.print(new String(tmp, 0, i));
					}
					if (channel.isClosed()) {
						System.out.println("Exit status: "
								+ channel.getExitStatus());
						break;
					}
					try {
						Thread.sleep(100);
					} catch (Exception ee) {
					}
					;
				}
				gui.log("Output log saved to " + fileName);
			} catch (JSchException e) {
				gui.log(e.getMessage());
				e.printStackTrace();
			} catch (IOException e) {
				gui.log(e.getMessage());
			}
			
			// start next iteration if needed
			currentIteration++;
			if (currentIteration < ExperimentConfiguration.no_iters) {
				
				ExperimentManager.this.start();
			} else /*if (ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_480p
					|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_1080p
					|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_480p_1fps
					|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_1080p_1fps
					|| ExperimentConfiguration.experimentType == ExperimentConfiguration.ALLBUTONE_1080p_1fps
					|| ExperimentConfiguration.experimentType == ExperimentConfiguration.ALLBUTONE_480p_1fps)*/
			{
				// reduce the number of nodes participating, but do not delete
				// the reference node
				if (target.size() > ExperimentConfiguration.no_nodes_min) {
					int i = target.size() - 1;
					while (i >= 0
							&& target.get(i).ip
									.equals(ExperimentConfiguration.REFERENCE_NODE)) {
						i--;
					}
					gui.log("Removing node " + target.get(i).ip
							+ " from experiment");
					target.remove(i);
					ExperimentConfiguration.no_nodes--;
					int counter = 0;					
					int interval = (int)((ExperimentConfiguration.chunk_sec*1000.0)/(1.0*ExperimentConfiguration.no_nodes));
					for (MobileNode mn : target) {				
						// set chunk size
						mn.sendChunkSize(ExperimentConfiguration.chunk_sec);
						// set random start interval or not
						if (ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_480p
								|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_1080p
								|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_480p_1fps
								|| ExperimentConfiguration.experimentType == ExperimentConfiguration.RANDOM_ALLBUTONE_1080p_1fps) {
							mn.sendRandomStartInterval();
						} else if (ExperimentConfiguration.experimentType == ExperimentConfiguration.SLOTTED_ALLBUTONE_1080p ||
								ExperimentConfiguration.experimentType == ExperimentConfiguration.SLOTTED_ALLBUTONE_480p ||
								ExperimentConfiguration.experimentType == ExperimentConfiguration.SLOTTED_ALLBUTONE_480p_1fps ||
								ExperimentConfiguration.experimentType == ExperimentConfiguration.ALLBUTONE_1080p_1fps){
							gui.log("Node "+mn.ip+" will start with delay "+interval*counter);
							mn.sendStartDelay(interval*counter);
						}
						// set random start chunk if needed
						if (ExperimentConfiguration.experimentType != ExperimentConfiguration.ALL_IDENTICAL) {
							if (!mn.ip.equals(ExperimentConfiguration.REFERENCE_NODE)) {
								mn.sendLoopUploads(true);
								mn.sendStartRandomChunk();
							} else {
								mn.sendLoopUploads(false);
							}
						} else {
							// set looping over uploads as configured in gui
							mn.sendLoopUploads(ExperimentConfiguration.bLoopUpload);
						}
						counter++;
					}					
					
					currentIteration = 0;
					ExperimentManager.this.start();
				}
			}

		}
	}

	private class MyUserInfo implements UserInfo {

		@Override
		public String getPassphrase() {
			return "";
		}

		@Override
		public String getPassword() {
			return "gigasight";
		}

		@Override
		public boolean promptPassphrase(String arg0) {
			return true;
		}

		@Override
		public boolean promptPassword(String arg0) {
			return true;
		}

		@Override
		public boolean promptYesNo(String arg0) {
			return true;
		}

		@Override
		public void showMessage(String arg0) {

		}

	}

}
