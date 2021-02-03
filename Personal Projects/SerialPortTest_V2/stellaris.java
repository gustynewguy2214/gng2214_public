
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.util.Arrays;

import com.fazecast.jSerialComm.SerialPort;

public class stellaris {

	public static boolean disconnect = false;

	static String display = "";
	public static int display_score = 0;
	
	static String zeropad = "";
	
	public static boolean send = false;
	
	public static Thread thread1;
	
	static Thread Debug_ContestantList_Thread = null;
	
	public static boolean successful_auto = false;
	
	public static void connect() {
		
		// attempt to connect to the serial port
		
		try {
		
		if(successful_auto == false) {
			
			TestWindow.chosenPort = SerialPort.getCommPort(TestWindow.PortList_reference.getSelectedItem().toString());
			TestWindow.chosenPort.setComPortTimeouts(SerialPort.TIMEOUT_SCANNER, 0, 0);
			
		}

		boolean open = TestWindow.chosenPort.openPort();
		
		if(open || successful_auto == true) {
			
			stellaris.display_reset();
			
			TestWindow.ConnectButton_reference.setText("Disable Data Send");
			if(successful_auto)
				TestWindow.PortList_reference.setSelectedIndex(1);
			
			TestWindow.PortList_reference.setEnabled(false);
			disconnect = false;

			if(thread1 == null) {
				thread1 = new Thread(){

					@Override public void run() {
						// wait after connecting, so the bootloader can finish
						try {Thread.sleep(100); } catch(Exception e) {}

						while(disconnect==false) {

							if(send == true) {
								stellaris.display_set(1,null);	//Use messages that we build, not the ones already on the board.
								send = false;
							}
							
						}
					}
				};
			}
			try {

				System.out.println(thread1.getState());
				
				if(thread1.getState() == Thread.State.NEW) {
					thread1.start();
				}
				
			}
			catch(java.lang.IllegalThreadStateException f) {
				
				Errors.show_Error("FATAL ERROR: Thread was in illegal state and has crashed the program. Please relaunch.",null);
				
				f.printStackTrace();
				
			}
			
		}
		else {
			
			Errors.show_Error("ERROR: Connection timed out - The connection was forcibly disconnected, hogged by Termite, or hogged by another instance of the program. Please diagnose.", null);
			
		}
		
		}
		catch(java.lang.NullPointerException e) {
			
			//if(frmMain.display_error_shown == false) {
				Errors.show_Error("ERROR: The software has failed to detect any COM ports at all!\nThis prevents the software from sending data to the displays, but is not fatal to the program.\nBy proceeding, you acknowledge that the displays are broken!",null);
				//frmMain.display_error_shown = true;
			//}
				e.printStackTrace();
		}
		
	}
	
	public static void display_set(int mode, String cmd) {
		
		zeropad="";			//reset zeropad
		
		try {

			TestWindow.chosenPort.getOutputStream().flush();
			
			if(mode == 0) {

				/*
				int length = cmd.length();
				
				byte[] bytes = stringToBytesASCII(cmd);
				
				TestWindow.chosenPort.writeBytes(bytes, bytes.length);
				*/
				
				/*
				byte[] bytes = toBytes(cmd.toCharArray());
				
				TestWindow.chosenPort.writeBytes(bytes, bytes.length);
				
				System.out.println(bytes[0]);
				*/
				
				/*
				for(int i = 0; i < length; i++) {
					
					byte ch = cmd.getBytes()[i];
					byte ch_array[] = {ch};
					
					
					TestWindow.chosenPort.writeBytes(ch_array, 1);
					
					System.out.println("Sent: [" + ch + "]");
					
				}
				*/
				
			    //char vIn = cmd.toCharArray()[0];
			    //byte [] vOut = new byte[]{(byte)vIn};
				
			    //eTestWindow.chosenPort.writeBytes(vOut,1);
			    
				TestWindow.chosenPort.writeBytes(cmd.getBytes(),cmd.length());
				//System.out.println("Sent as bytes: [" + Byte.toString(vOut[0]) + "]");
				System.out.println(cmd);
			}
			else if(mode == 1) {
				
				if(display_score < 1000 && display_score >= 100) {
					
					zeropad = "0";
					
				}
				else if(display_score < 100 && display_score >= 10) {
					
					zeropad = "00";
					
				}
				else if(display_score < 10 && display_score >= 0) {
					
					zeropad = "000";
					
				}
				
				String msg = "*" + display + zeropad + display_score + "\n";

				//System.out.println("Sent message: [ " + "*" + display + zeropad + display_score + " ]");
				
				if(display != "" && display != "0") {
					TestWindow.chosenPort.writeBytes(msg.getBytes(), msg.length());
					
				}
				else {
					System.out.println("Attempted to send illegal message!");
					System.out.println("Attempted to send: [" + display + "] [" + zeropad + "] [" + display_score + "]");
				}
			}
		}
		catch(java.lang.NullPointerException e) {
			
			//if(frmMain.display_error_shown == false) {
				Errors.show_Error("ERROR: The software has failed to detect any COM ports at all!\nThis prevents the software from sending data to the displays, but is not fatal to the program.\nBy proceeding, you acknowledge that the displays are broken!",null);
				//frmMain.display_error_shown = true;
				
				e.printStackTrace();
			//}
				
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public static void display_reset() {
		//display_set(0, "G1\n");
		//display_set(0, "E\n");
		
		display = "";
		display_score = 0;
	}
	
	public static void outbound(String display_num, int display_amt) {
	
		send = true; 					//Let Master Debug know that we are sending something.
		display = display_num; 			//Change the necessary variables.
		display_score = display_amt;
		
		//DigibeeIO.update_outputs = true;
		
	}
	
	public static void auto_connect() {
		
		try {
			
			TestWindow.chosenPort = SerialPort.getCommPort(TestWindow.PortList_reference.getItemAt(1));		//Our port will ALWAYS be the second port on the list.
			TestWindow.chosenPort.setComPortTimeouts(SerialPort.TIMEOUT_SCANNER, 0, 0);
			
			if(TestWindow.chosenPort.openPort()) {
				
				Errors.show_Notice("Successful auto-connected to " + TestWindow.PortList_reference.getItemAt(1) + " .",null);		//May happen more on fresh plug ins?
				successful_auto = true;
				connect();
				
			}
			else {
				
				Errors.show_Error("ERROR: Could not automatically detect the defualt COM port. Please connect manually." , null);
				disconnect = true;
				
			}
		}
		catch(com.fazecast.jSerialComm.SerialPortInvalidPortException ipe) {
			
			Errors.show_Error("ERROR: The default COM port was considered invalid. Please connect manually." , null);
			disconnect = true;
			
		}
	}
	
	//X
	static byte[] toBytes(char[] chars) {
		CharBuffer charBuffer = CharBuffer.wrap(chars);
		ByteBuffer byteBuffer = Charset.forName("UTF-8").encode(charBuffer);
		byte[] bytes = Arrays.copyOfRange(byteBuffer.array(),
		          byteBuffer.position(), byteBuffer.limit());
		Arrays.fill(byteBuffer.array(), (byte) 0); // clear sensitive data
		return bytes;
	}
	
	public static byte[] stringToBytesASCII(String str) {
		 char[] buffer = str.toCharArray();
		 byte[] b = new byte[buffer.length];
		 for (int i = 0; i < b.length; i++) {
		  b[i] = (byte) buffer[i];
		 }
		 return b;
		}
	
	
}
