import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.Writer;
import java.util.ArrayList;
import java.util.Random;

public class GarbageWriter {

	public static void main(String[] args) throws IOException {
		
		double amount = 10;	//In (unit)s
		int unit = (int) Math.pow(2, 20);	//The unit to use. 2^20 is a MB.
		double size = amount * unit;
		String[] directories = {"C:\\Users\\Josh\\Desktop\\"};
		Random rnd = new Random();
		Writer output = null;
		
		ArrayList<Long> ctimes = new ArrayList<Long>();
		ArrayList<Long> ztimes = new ArrayList<Long>();
		
		int desired_writes = 10;
		
		for(int i = 0; i < directories.length; i++) {
			for(int j = 0; j < desired_writes; j++) {
				long start = System.nanoTime();
				
				System.out.println("Writing a new file to: " + directories[i]);
				File file = new File(directories[i],"STaxFile.pdf");
				if(file.exists()) {
					file.delete();
					file.createNewFile();
				}
				output = new BufferedWriter(new FileWriter(file));
				byte[] random = new byte[(int) size];
				rnd.nextBytes(random);
				String converter = new String(random,"UTF-8");
				char[] chars = converter.toCharArray();
				output.write(chars);
				long end = System.nanoTime();
				long result = (end-start)/1000000;
				if(i == 0) {
					ctimes.add(result);
				}
				else if(i == 1) {
					ztimes.add(result);
				}
				System.out.println("The process took: " + result + " milliseconds.\n");
			}
		}
		long caverage = 0;
		long zaverage = 0;
		for(int k = 0; k < desired_writes; k++) {
			caverage += ctimes.get(k);
		}
		caverage /= desired_writes;
		zaverage /= desired_writes;
		
		System.out.println("The average C write time was: " + caverage);
		System.out.println("The average Z write time was: " + zaverage);
		System.out.println("The Z writing was: " + ((float) zaverage / (float) caverage) + " times slower.");
		
		output.close();

	}

}
