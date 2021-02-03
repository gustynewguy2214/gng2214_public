
import javax.swing.JOptionPane;

public class Errors {

	//show_Error Function
	public static void show_Error (String msg, String sound) {
		if(sound == null || sound == "") {
			audio.playSound("chord.wav",null);
			JOptionPane.showMessageDialog(null, msg, null, JOptionPane.ERROR_MESSAGE);
		}
		else {
			audio.playSound(sound,null);
			JOptionPane.showMessageDialog(null, msg, null, JOptionPane.ERROR_MESSAGE);
		}
	}
	
	//Shows a notice / warning.
	public static void show_Notice (String msg, String sound) {
		if(sound == null || sound == "") {
			audio.playSound("Windows Default.wav",null);
			JOptionPane.showMessageDialog(null, msg);
		}
		else {
			audio.playSound(sound,null);
			JOptionPane.showMessageDialog(null, msg);
		}
	}
}
