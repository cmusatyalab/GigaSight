import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JLabel;
import javax.swing.JPanel;

public class StatusPanel extends JPanel {

	private int number;
	public boolean assigned;
	private JLabel ipLabel;
	private JLabel messageLabel;
	
	public StatusPanel(int number){
	
		this.number = number;
		this.assigned = false;
		
		setOpaque(true);
		setPreferredSize(new Dimension(150,150));
		setAlignmentY(Component.CENTER_ALIGNMENT);
		
		ipLabel = new JLabel(""+number,JLabel.CENTER);
		ipLabel.setAlignmentX(Component.CENTER_ALIGNMENT);
		messageLabel = new JLabel("disconnected");
		messageLabel.setAlignmentX(Component.CENTER_ALIGNMENT);
		
		setLayout(new BoxLayout(this,BoxLayout.Y_AXIS));
		add(Box.createVerticalStrut(20));
		add(ipLabel);
		add(Box.createVerticalGlue());
		add(messageLabel);
		add(Box.createVerticalStrut(20));
			
		setDisconnected();
	}
	
	
	public void setMessage(String text){
		messageLabel.setText(text);
	}
	
	//when node is disconnected
	public void setDisconnected(){
		this.assigned = false;
		ipLabel.setText(this.number+"");
		messageLabel.setText("disconnected");
		setBackground(Color.RED);
	}
	
	public void setConnected(String ip){
		ipLabel.setText(ip);
		messageLabel.setText("Connected");
		setBackground(Color.ORANGE);
	}
	public void setInitializing(){
		this.assigned = true;
		messageLabel.setText("initializing");
		setBackground(Color.magenta);		
	}
	
	public void setInitialized(){
		this.assigned = true;
		messageLabel.setText("Ready");
		setBackground(Color.GREEN);
	}
	
	public void setRunning(){
		this.assigned = true;
		messageLabel.setText("Running");
		setBackground(Color.GREEN);
	}
	
	public void setStopped(){
		setInitialized();
	}
}
