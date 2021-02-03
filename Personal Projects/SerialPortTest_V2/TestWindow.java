import java.awt.EventQueue;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JTextField;

import com.fazecast.jSerialComm.SerialPort;

public class TestWindow {

	private JFrame frame;

	/**
	 * Launch the application.
	 */
	public static void main(String[] args) {
		EventQueue.invokeLater(new Runnable() {
			public void run() {
				try {
					TestWindow window = new TestWindow();
					window.frame.setVisible(true);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		});
	}

	/**
	 * Create the application.
	 */
	public TestWindow() {
		initialize();
	}

	public static JButton ConnectButton_reference = null;
	
	public static SerialPort chosenPort = null;
																		  /*
																		   * Reminder: COM11 on Dad's computer
																		   * does not even exist until you plug a device in, thus voiding any checks
																		   * because the list is never filled. COM11 is completely virtual
																		   * on Dad's computer, and the only reason COM3 works out of the box
																		   * on Josh's end is because of a built in COM port.
																		   */
	
	static ArrayList <JFrame> window_reference = new ArrayList <JFrame>();

	public static JComboBox<String> PortList_reference;

	public static Thread thread1;

	
	private void initialize() {
		frame = new JFrame();
		frame.setBounds(100, 100, 450, 300);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.setLayout(null);
		
		JComboBox<String> DEBUG_PortList = new JComboBox<String>();
		DEBUG_PortList.setBounds(55, 71, 103, 20);
		frame.getContentPane().add(DEBUG_PortList);
		PortList_reference = DEBUG_PortList;
		
		SerialPort[] portNames = SerialPort.getCommPorts();
		for(int i = 0; i < portNames.length; i++)
			DEBUG_PortList.addItem(portNames[i].getSystemPortName());
		
		JButton DEBUG_PortActivateBtn = new JButton("Enable Port");
		DEBUG_PortActivateBtn.setBounds(55, 102, 125, 20);
		frame.getContentPane().add(DEBUG_PortActivateBtn);
		DEBUG_PortActivateBtn.addActionListener(new ActionListener(){
			@Override public void actionPerformed(ActionEvent e) {
				if(DEBUG_PortActivateBtn.getText().equals("Enable Port")) {
					
					stellaris.connect();
					
				} else {
					
					if(chosenPort != null) {

						//  from the serial port
						chosenPort.closePort();
						DEBUG_PortList.setEnabled(true);
						DEBUG_PortActivateBtn.setText("Enable Port");
						stellaris.disconnect = true;
						thread1 = null;
						stellaris.successful_auto = false;	//Disable the successful auto flag to allow for normal operation.
						
					}
				}
			}
		});
		ConnectButton_reference = DEBUG_PortActivateBtn;
		
		JTextField field = new JTextField();
		field.setBounds(0,0,200,20);
		frame.getContentPane().add(field);
		
		JButton btnSend = new JButton("Send");
		btnSend.setBounds(190,102,125,20);
		btnSend.addMouseListener(new MouseAdapter() {
			@Override
			public void mouseClicked(MouseEvent e) {
				if(chosenPort != null) {
					stellaris.display_set(0, field.getText());
					field.setText("");
				}
			}
		});
		frame.getContentPane().add(btnSend);
	}
}
