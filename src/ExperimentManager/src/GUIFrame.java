import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.ButtonGroup;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.text.BadLocationException;

public class GUIFrame extends JFrame {

	private final static int MAX_NODES = 16;
	private JTextArea log = new JTextArea();

	private ResolutionListener resListener = new ResolutionListener();
	private ChunkListener chunkListener = new ChunkListener();
	private ControlListener controlListener = new ControlListener();
	private StatusPanel[] nodePanels = new StatusPanel[MAX_NODES];

	public GUIFrame() {
		super("GigaSight Experiment Controller");
	}

	public void createAndShow() {
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		// create the panel with the status windows
		JPanel statusPanel = new JPanel(new GridLayout(4, 4, 5, 5));
		for (int i = 0; i < MAX_NODES; i++) {
			nodePanels[i] = new StatusPanel(i);
			statusPanel.add(nodePanels[i]);
		}

		// create the panel with the experiment details
		JPanel experimentPanel = new JPanel();
		experimentPanel.setLayout(new BoxLayout(experimentPanel,
				BoxLayout.Y_AXIS));
		experimentPanel.setPreferredSize(new Dimension(300, 600));

		// subpanel 1: parameters
		JPanel parameterPanel = new JPanel();
		parameterPanel
				.setLayout(new BoxLayout(parameterPanel, BoxLayout.Y_AXIS));
		parameterPanel.setBorder(BorderFactory
				.createTitledBorder("Experiment parameters"));
		/*
		 * // 1.1: resolution JLabel resLabel = new JLabel("Resolution: ");
		 * resLabel.setAlignmentX(Component.LEFT_ALIGNMENT);
		 * parameterPanel.add(resLabel); JPanel resPanel = new JPanel(new
		 * FlowLayout(FlowLayout.LEFT)); ButtonGroup resGroup = new
		 * ButtonGroup(); for (String s : ExperimentConfiguration.availableRes)
		 * { JRadioButton button = new JRadioButton(s);
		 * button.setActionCommand(s); button.addActionListener(resListener);
		 * resGroup.add(button); resPanel.add(button); if (s ==
		 * ExperimentConfiguration.availableRes[0]) { button.setSelected(true);
		 * ExperimentConfiguration.resolution = s; } }
		 * 
		 * resPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		 * parameterPanel.add(resPanel);
		 */
		// 1.0: experiment type
		JLabel typeLabel = new JLabel("Experiment type: ");
		typeLabel.setAlignmentX(Component.LEFT_ALIGNMENT);
		parameterPanel.add(typeLabel);
		JPanel typePanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		JComboBox<String> typeList = new JComboBox<String>(
				ExperimentConfiguration.experimentDescription);
		// init
		ExperimentConfiguration.experimentType = ExperimentConfiguration.defExpType;
		typeList.setSelectedIndex(ExperimentConfiguration.experimentType);		
		typeList.addItemListener(new TypeListener());
		typePanel.add(typeList);
		typePanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		parameterPanel.add(typePanel);

		// 1.1: resolution/files to be used
		JLabel fileLabel = new JLabel("File: ");
		fileLabel.setAlignmentX(Component.LEFT_ALIGNMENT);
		parameterPanel.add(fileLabel);
		JPanel filePanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		JComboBox<String> fileList = new JComboBox<String>(
				ExperimentConfiguration.availableVideos);
		// init
		ExperimentConfiguration.video = ExperimentConfiguration.availableVideos[ExperimentConfiguration.defvideo];
		fileList.setSelectedIndex(ExperimentConfiguration.defvideo);
		fileList.addActionListener(new FileListener());
		filePanel.add(fileList);
		filePanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		parameterPanel.add(filePanel);

		// 1.2: chunk size
		JLabel chunkLabel = new JLabel("Chunk time: ");
		chunkLabel.setAlignmentX(Component.LEFT_ALIGNMENT);
		parameterPanel.add(chunkLabel);
		JPanel chunkPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		ButtonGroup chunkGroup = new ButtonGroup();
		for (int i = 0; i < ExperimentConfiguration.availableChunk.length; i++) {
			JRadioButton button = new JRadioButton(
					ExperimentConfiguration.availableChunk[i]);
			button.setActionCommand(""
					+ ExperimentConfiguration.availableChunkSec[i]);
			button.addActionListener(chunkListener);
			chunkGroup.add(button);
			chunkPanel.add(button);
			if (i == 0) {
				button.setSelected(true);
				ExperimentConfiguration.chunk_sec = ExperimentConfiguration.availableChunkSec[0];
			}
		}
		chunkPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		parameterPanel.add(chunkPanel);

		// 1.3: no iterations
		JPanel iterPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		iterPanel.add(new JLabel("No iterations: "));
		JTextField iterTextField = new JTextField(3);
		iterTextField.getDocument().addDocumentListener(
				new IterationListener(iterTextField));
		// iterTextField.setMinimumSize(new Dimension(40,1));
		ExperimentConfiguration.no_iters = 5;
		iterTextField.setText(""+ExperimentConfiguration.no_iters);
		iterPanel.add(iterTextField);
		iterPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		parameterPanel.add(iterPanel);

		// 1.4: no nodes to involve in the experiment
		JPanel MaxNoNodesPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		MaxNoNodesPanel.add(new JLabel("Max no nodes in experiment: "));
		JTextField MaxNoNodesTextField = new JTextField(3);
		MaxNoNodesTextField.getDocument().addDocumentListener(
				new MaxNoNodesListener(MaxNoNodesTextField));
		// iterTextField.setMinimumSize(new Dimension(40,1));
		MaxNoNodesTextField.setText("1");
		MaxNoNodesPanel.add(MaxNoNodesTextField);
		MaxNoNodesPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		
		JPanel MinNoNodesPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		MinNoNodesPanel.add(new JLabel("Min no nodes in experiment: "));
		JTextField MinNoNodesTextField = new JTextField(3);
		MinNoNodesTextField.getDocument().addDocumentListener(
				new MinNoNodesListener(MinNoNodesTextField));
		// iterTextField.setMinimumSize(new Dimension(40,1));
		MinNoNodesTextField.setText("1");
		MinNoNodesPanel.add(MinNoNodesTextField);
		MinNoNodesPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		
		parameterPanel.add(MaxNoNodesPanel);
		parameterPanel.add(MinNoNodesPanel);
		
		// 1.4: duration of the experiment
		JPanel durationPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		durationPanel.add(new JLabel("Duration (s): "));
		JTextField durationTextField = new JTextField(5);
		durationTextField.getDocument().addDocumentListener(
				new DurationListener(durationTextField));
		// iterTextField.setMinimumSize(new Dimension(40,1));
		durationTextField.setText("600");
		durationPanel.add(durationTextField);
		durationPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		parameterPanel.add(durationPanel);

		// 1.5: do upload of files again?
		JPanel uploadPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		JCheckBox uploadButton = new JCheckBox(
				"Upload files during initialization");
		uploadButton.addItemListener(new UploadCheckListener());
		uploadPanel.add(uploadButton);
		uploadPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		parameterPanel.add(uploadPanel);

		// 1.6: loop uploads?
		JPanel loopPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		JCheckBox loopButton = new JCheckBox("Loop over files?");
		loopButton.addItemListener(new LoopCheckListener());
		loopPanel.add(loopButton);
		loopPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		parameterPanel.add(loopPanel);

		// subpanel 2: controls
		JPanel controlPanel = new JPanel();
		controlPanel.setBorder(BorderFactory
				.createTitledBorder("Experiment control"));
		JButton init = new JButton("INIT");
		init.setEnabled(true);
		init.setActionCommand("INIT");
		init.addActionListener(controlListener);
		JButton start = new JButton("START");
		start.setActionCommand("START");
		start.addActionListener(controlListener);
		JButton stop = new JButton("STOP");
		stop.setActionCommand("STOP");
		stop.addActionListener(controlListener);
		controlPanel.add(init);
		controlPanel.add(start);
		controlPanel.add(stop);

		// subpanel 3: Status window with logging output
		JPanel logPanel = new JPanel(new BorderLayout());
		logPanel.setBorder(BorderFactory.createTitledBorder("Log"));
		// JTextArea log = new JTextArea(10,10);
		log.setBorder(BorderFactory.createLineBorder(Color.black));
		log.setOpaque(true);
		log.setBackground(Color.white);
		log.setEditable(false);
		JScrollPane scrollPane = new JScrollPane(log);
		logPanel.add(scrollPane, BorderLayout.CENTER);

		// set the complete panel
		parameterPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		experimentPanel.add(parameterPanel);
		controlPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		experimentPanel.add(controlPanel);
		logPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
		experimentPanel.add(logPanel);

		// experimentPanel.add(statusPanel)
		// set up the global content pane
		JPanel contentPane = new JPanel(new BorderLayout());
		contentPane.add(experimentPanel, BorderLayout.WEST);
		contentPane.add(statusPanel, BorderLayout.CENTER);
		setContentPane(contentPane);

		// display the window
		setTitle("GigaSight experiment controller");
		// frame.setPreferredSize(new Dimension(300,400));
		pack();
		setLocationRelativeTo(null); // center on screen
		setVisible(true);
	}

	public StatusPanel reservePanel() {
		synchronized (nodePanels) {
			for (int i = 0; i < MAX_NODES; i++) {
				if (!nodePanels[i].assigned) {
					nodePanels[i].assigned = true;
					return nodePanels[i];
				}
			}
		}
		return null;
	}

	public void freePanel(StatusPanel sp) {
		synchronized (nodePanels) {
			sp.assigned = false;
			sp.setDisconnected();
		}
	}

	public void log(String text) {
		if (log.getLineCount() > 15) {
			try {
				log.replaceRange(null, log.getLineStartOffset(0),
						log.getLineEndOffset(0));
			} catch (BadLocationException e) {
				e.printStackTrace();
				return;
			}
		}

		log.append(text + "\n");
		log.repaint();

	}

	private class ResolutionListener implements ActionListener {

		@Override
		public void actionPerformed(ActionEvent arg0) {
			ExperimentConfiguration.resolution = arg0.getActionCommand();
			System.out.println("Resolution set to "
					+ ExperimentConfiguration.resolution);
		}

	}

		private class ChunkListener implements ActionListener {
		@Override
		public void actionPerformed(ActionEvent arg0) {
			ExperimentConfiguration.chunk_sec = Integer.parseInt(arg0
					.getActionCommand());
			System.out.println("Chunk sec set to "
					+ ExperimentConfiguration.chunk_sec);
		}
	}

	private class ControlListener implements ActionListener {

		@Override
		public void actionPerformed(ActionEvent arg0) {
			ExperimentManager expManager = ExperimentManager.getInstance();
			// System.out.println("Actionperformed!");
			String actionCommand = arg0.getActionCommand();
			if (actionCommand.equals("INIT")) {
				expManager.init();
			} else if (actionCommand.equals("START")) {
				expManager.start();
			} else if (actionCommand.equals("STOP")) {
				expManager.stop();
			}
		}
	}

	private class IterationListener implements DocumentListener {

		JTextField textField;

		public IterationListener(JTextField textField) {
			this.textField = textField;
		}

		@Override
		public void changedUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.no_iters = Integer.parseInt(text);
				System.out.println("No iterations set to: "+ExperimentConfiguration.no_iters);
			}
			
		}

		@Override
		public void insertUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.no_iters = Integer.parseInt(text);
				System.out.println("No iterations set to: "+ExperimentConfiguration.no_iters);
			}
		}

		@Override
		public void removeUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.no_iters = Integer.parseInt(text);
				System.out.println("No iterations set to: "+ExperimentConfiguration.no_iters);
			}

		}
	}

	private class MaxNoNodesListener implements DocumentListener {

		JTextField textField;

		public MaxNoNodesListener(JTextField textField) {
			this.textField = textField;
		}

		@Override
		public void changedUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.no_nodes_max = Integer.parseInt(text);
				System.out.println("Max nodes set to: "
						+ ExperimentConfiguration.no_nodes_max);
			}
		}

		@Override
		public void insertUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.no_nodes_max = Integer.parseInt(text);
				System.out.println("Max no nodes set to: "
						+ ExperimentConfiguration.no_nodes_max);
			}
		}

		@Override
		public void removeUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.no_nodes_max = Integer.parseInt(textField
						.getText());
				System.out.println("Max no nodes set to: "
						+ ExperimentConfiguration.no_nodes_max);
			}

		}
	}

	private class MinNoNodesListener implements DocumentListener {

		JTextField textField;

		public MinNoNodesListener(JTextField textField) {
			this.textField = textField;
		}

		@Override
		public void changedUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.no_nodes_min = Integer.parseInt(text);
				System.out.println("Min nodes set to: "
						+ ExperimentConfiguration.no_nodes_min);
			}
		}

		@Override
		public void insertUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.no_nodes_min = Integer.parseInt(text);
				System.out.println("Min no nodes set to: "
						+ ExperimentConfiguration.no_nodes_min);
			}
		}

		@Override
		public void removeUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.no_nodes_min = Integer.parseInt(textField
						.getText());
				System.out.println("Min no nodes set to: "
						+ ExperimentConfiguration.no_nodes_min);
			}

		}
	}

	
	
	private class DurationListener implements DocumentListener {

		JTextField textField;

		public DurationListener(JTextField textField) {
			this.textField = textField;
		}

		@Override
		public void changedUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.duration_sec = Integer.parseInt(text);
				System.out.println("Duration set to: "
						+ ExperimentConfiguration.duration_sec);
			}
		}

		@Override
		public void insertUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.duration_sec = Integer.parseInt(text);
				System.out.println("Duration set to: "
						+ ExperimentConfiguration.duration_sec);
			}
		}

		@Override
		public void removeUpdate(DocumentEvent arg0) {
			String text = textField.getText();
			if (!text.isEmpty()) {
				ExperimentConfiguration.duration_sec = Integer.parseInt(text);
				System.out.println("Duration set to: "
						+ ExperimentConfiguration.duration_sec);
			}
		}
	}

	private class UploadCheckListener implements ItemListener {

		@Override
		public void itemStateChanged(ItemEvent e) {
			ExperimentConfiguration.bDoUpload = (e.getStateChange() == ItemEvent.SELECTED);
			System.out.println("Do upload" + ExperimentConfiguration.bDoUpload);

		}

	}

	private class LoopCheckListener implements ItemListener {

		@Override
		public void itemStateChanged(ItemEvent e) {
			ExperimentConfiguration.bLoopUpload = (e.getStateChange() == ItemEvent.SELECTED);
			System.out.println("Clients will loop their uploads: "
					+ ExperimentConfiguration.bDoUpload);

		}

	}

	private class TypeListener implements ItemListener {

		@Override
		public void itemStateChanged(ItemEvent e) {
			if(e.getStateChange() == ItemEvent.SELECTED){
				String textType = (String)e.getItem();
				System.out.println("Selected experiment type: "+textType);
				for(int i = 0; i < ExperimentConfiguration.experimentDescription.length; i++){
					if(textType.equals(ExperimentConfiguration.experimentDescription[i])){
						ExperimentConfiguration.experimentType = i;
						System.out.println("Type no "+i);
						break;
					}
					
				}
				
			}
						
		}

	}

	private class FileListener implements ActionListener {

		public void actionPerformed(ActionEvent e) {
			String selectedFile = (String) ((JComboBox) (e.getSource()))
					.getSelectedItem();
			ExperimentConfiguration.video = selectedFile;
			System.out.println("Selected file: "
					+ ExperimentConfiguration.video);

		}

	}
}
