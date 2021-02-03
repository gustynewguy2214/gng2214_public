import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.attribute.FileTime;
import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextPane;
import javax.swing.filechooser.FileNameExtensionFilter;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;
import javax.swing.JButton;
import javax.swing.JTextField;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;

public class FileFinder {

	private JFrame frame;
	private JPanel panel_1;
	private Component verticalStrut;
	private static JTextPane textPane;
	private JPanel panel_2;
	private Component horizontalStrut;
	private JPanel panel_3;
	private static JTextPane textPane_1;
	private JScrollPane scrollPane;
	
	static String[] columnNames = {"File", "Last Modified Time:"};
	private static JTable table = new JTable( new DefaultTableModel() );
	
	public static boolean done;

	static ArrayList<String> excluded_dirs = new ArrayList<String>();
	static ArrayList<String> excluded_exts = new ArrayList<String>();
	
	static ArrayList<String> res_dirs = new ArrayList<String>();
	static ArrayList<String> res_files = new ArrayList<String>();
	static ArrayList<String> res_times = new ArrayList<String>();
	
	static ArrayList<String> sel_dirs = new ArrayList<String>();
	
	
	
	JPanel panel;
	
	/**
	 * Launch the application.
	 */
	public static void main(String[] args) {
		
		FileFinder window = new FileFinder();
		window.frame.setVisible(true);
		
	}

	
	
	
	/**
	 * Create the application.
	 */
	public FileFinder() {
		initialize();
	}

	/**
	 * Initialize the contents of the frame.
	 */
	private void initialize() {
		frame = new JFrame();
		frame.setBounds(100, 100, 1000, 529);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		
		panel  = new JPanel();
		frame.getContentPane().add(panel, BorderLayout.CENTER);
		panel.setLayout(new BorderLayout(0, 0));
		table.setFillsViewportHeight(true);

		JScrollPane tableContainer = new JScrollPane(table);
        panel.add(tableContainer);
        frame.getContentPane().add(panel);
        
        panel_1 = new JPanel();
        panel.add(panel_1, BorderLayout.NORTH);
        panel_1.setLayout(new BoxLayout(panel_1, BoxLayout.X_AXIS));
        
        verticalStrut = Box.createVerticalStrut(20);
        panel_1.add(verticalStrut);
        
        textPane = new JTextPane();
        panel_1.add(textPane);
        
        panel_2 = new JPanel();
        panel.add(panel_2, BorderLayout.EAST);
        panel_2.setLayout(new BorderLayout(0, 0));
        
        horizontalStrut = Box.createHorizontalStrut(240);
        panel_2.add(horizontalStrut, BorderLayout.NORTH);
        
        panel_3 = new JPanel();
        panel_2.add(panel_3, BorderLayout.CENTER);
        GridBagLayout gbl_panel_3 = new GridBagLayout();
        gbl_panel_3.columnWidths = new int[]{0, 0};
        gbl_panel_3.rowHeights = new int[]{0, 0};
        gbl_panel_3.columnWeights = new double[]{1.0, Double.MIN_VALUE};
        gbl_panel_3.rowWeights = new double[]{1.0, Double.MIN_VALUE};
        panel_3.setLayout(gbl_panel_3);
        
        scrollPane = new JScrollPane();
        GridBagConstraints gbc_scrollPane = new GridBagConstraints();
        gbc_scrollPane.fill = GridBagConstraints.BOTH;
        gbc_scrollPane.gridx = 0;
        gbc_scrollPane.gridy = 0;
        panel_3.add(scrollPane, gbc_scrollPane);
        
        textPane_1 = new JTextPane();
        textPane_1.setEditable(false);
        scrollPane.setViewportView(textPane_1);
        
        panel_4 = new JPanel();
        panel.add(panel_4, BorderLayout.SOUTH);
        
        verticalStrut_1 = Box.createVerticalStrut(20);
        panel_4.add(verticalStrut_1);
        
        btnNewButton = new JButton("Start Search");
        btnNewButton.addMouseListener(new MouseAdapter() {
        	@Override
        	public void mouseClicked(MouseEvent e) {
        		
        		Thread t = new Thread(){
        			public void run() {
        				
        				

        				sb.setLength(0);
        				sb.append("Folders Explored:\n");
        				sb.append("-----------------\n");
        				
        				Search();
        				
        				done = true;
        				btnNewButton.setEnabled(true);
        				
        			}
        		};
        		t.start();
        		btnNewButton.setEnabled(false);
        		System.out.println("Search started.");
        		
        	}
        });
        panel_4.add(btnNewButton);
        
        horizontalStrut_1 = Box.createHorizontalStrut(20);
        panel_4.add(horizontalStrut_1);
        
        textField = new JTextField();
        panel_4.add(textField);
        textField.setColumns(10);
        
        btnNewButton_1 = new JButton("Add Exclusion Parameter");
        panel_4.add(btnNewButton_1);
        
        btnNewButton_2 = new JButton("Re-Select Exclusion Filters");
        btnNewButton_2.addMouseListener(new MouseAdapter() {
        	@Override
        	public void mouseClicked(MouseEvent e) {
        		
        		selectFilterFiles();
        		
        	}
        });
        panel_4.add(btnNewButton_2);
		
        
        
        
		
		/*
		excluded_dirs.add("$");
		excluded_dirs.add("Apple");
		excluded_dirs.add("Apache");
		excluded_dirs.add("MinGW");
		excluded_dirs.add("MSI");
		excluded_dirs.add("NVIDIA");
		excluded_dirs.add(".");
		excluded_dirs.add("cache");
		excluded_exts.add("dll");
		excluded_exts.add("adb");
		excluded_exts.add("ads");
		excluded_exts.add("ali");
		excluded_exts.add("pm");
		excluded_exts.add("pl");
		excluded_exts.add("hpp");
		excluded_exts.add("sys");
		excluded_exts.add("htm");
		*/
		
		//sel_dirs.add("C:\\");
		//sel_dirs.add("H:\\");
		//sel_dirs.add("J:\\");

		

	}
	
	protected void selectFilterFiles() {
		
		JFileChooser fc = new JFileChooser();
		fc.setFileFilter(new FileNameExtensionFilter("*.txt", "txt"));

		File file1 = null;
		File file2 = null;
		File file3 = null;
		
		File[] filters = {file1,file2,file3};
		String[] instructions = {"starting directories","directory exclusion","extension exclusion"};
		
		for(int i = 0; i < filters.length; i++) {
			fc.setDialogTitle("Choose your " + instructions[i] + " file.");
			int returnVal = fc.showOpenDialog(frame);
			
			if (returnVal == JFileChooser.APPROVE_OPTION) {
				filters[i] = fc.getSelectedFile();
			}
		}
		
		BufferedReader br;
		String line = null;  
		
		ArrayList<ArrayList<String>> options = new ArrayList<ArrayList<String>>();
		options.add(sel_dirs);
		options.add(excluded_exts);
		options.add(excluded_dirs);

		for(int i = 0; i < options.size(); i++) {
			try {
				br = new BufferedReader(new FileReader(filters[i]));
				while ((line = br.readLine()) != null)  
				{  
				   System.out.println("Read line: [" + line + "] into: [" + options.get(i).getClass().getName() + "]");
					options.get(i).add(line);
				}
			} catch (Exception e) {
				
			}
		}
		  
		textPane.setText("Parameters Loaded.");
		
	}




	static void Search() {
		
		System.out.println("Search really did start.");
		System.out.println("sel_dirs.size() is: " + sel_dirs.size());
		
		for(int i = 0; i < sel_dirs.size(); i++) {
			File f = new File(sel_dirs.get(i));
			init_space += f.getTotalSpace();	//in bytes
		}
		
		long start = System.nanoTime();
		for(int i = 0; i < sel_dirs.size(); i++) {
			doDir(sel_dirs.get(i),sel_dirs.get(i));
		}
		long end = System.nanoTime();
		long result = (end-start)/1000000;

		Object[][] data = new Object[res_files.size()][columnNames.length];
		for(int k = 0; k < res_files.size(); k++) {
			data[k][0] = res_files.get(k);
			data[k][1] = res_times.get(k);
		}
		
		TableModel model = new AbstractTableModel() {
			private static final long serialVersionUID = 1L;
			public String getColumnName(int col) {
		        return columnNames[col].toString();
		    }
		    public int getRowCount() { return res_files.size(); }
		    public int getColumnCount() { return columnNames.length; }
		    public Object getValueAt(int row, int col) {
		        return data[row][col];
		    }
		    public boolean isCellEditable(int row, int col)
		        { return true; }
		    public void setValueAt(Object value, int row, int col) {
		        data[row][col] = value;
		        fireTableCellUpdated(row, col);
		    }
		};
		
		table.setModel(model);
		textPane.setText("Finished. " + res_files.size() + " files found. Took " + result + " ms. Highest estimated time was: " + highest_est + " ms.");
		
	}
	
	
	
	static long init_space = 0;
	
	static int level = 0;
	static int level_limit = 200;
	static int deepest_level = 0;
	
	static String ref_string = "";
	

	static long est_time = 0;
	static long highest_est = 0;
	static double last_rate = 1;
	
	static int files_per_second = 1;
	static long true_files_per_second = 0;
	static long average_size = 1;
	static long true_average_size = 1;
	
	static void doDir(String start, String dir) {
		
		ArrayList<String> files = new ArrayList<>();
		ArrayList<String> directories = new ArrayList<>();
		File root = new File(start);
		String str_root = root.getAbsolutePath();
		File file = new File(dir);
		
		long new_total = (long) Math.pow(root.getTotalSpace(),10);
		
		if(accounted > new_total) {
			System.out.println(new_total);
			System.out.println(accounted);
			System.exit(-200);
		}
		
		est_time = (long) (((new_total - accounted)/1000)/last_rate);	//Using this: https://www.wikihow.com/Calculate-Data-Transfer-Rate
		
		
		//System.out.println("Inspecting Folder: [" + dir + "] at level: [" + (level) + "]");
		
		if(highest_est < est_time) {
			highest_est = est_time;
		}
		
		ref_string = dir.concat("<" + Integer.toString(level) + ">");
		if(ref_string.contains("\\<") && level > 0) {
			return;
		}

		

		//if(level > 4) {
		//	updateExploredDirs();	//Works, but is very update intensive.
		//}
		
		/*
		if(level > level_limit) {
			for(int i = 0; i < file.list().length; i++) {
				System.out.println(i + ": " + file.list()[i]);
			}
			System.exit(0);
		}
		*/
		
		try {
			if(file.listFiles() != null) {
				
				Thread t1 = new Thread() {
					
					public void run() {
						
						delay(1000);
						
						true_files_per_second = average_size / files_per_second;
						
						average_size = 1;
						files_per_second = 1;
						
					}
					
				};
				t1.start();
				
				for(File f : file.listFiles()) {
				
					files_per_second++;
					average_size += f.getTotalSpace();
					accounted += f.getTotalSpace();
					
					String abs_path = f.getAbsolutePath();
	
					int pointer = abs_path.length()-1;
					
					if(f.isDirectory() == false) {
						textPane.setText("Looking at: " + f.getAbsolutePath() + " | Estimated time remaining: " + String.valueOf(est_time));
					}
					
					String path = abs_path;
					String ext = "";
					
					while(abs_path.charAt(pointer) != '.') {
						pointer--;
						if(pointer < 0)
							break;
					}

					if(pointer >= 0) {
						path = abs_path.substring(0,pointer);
						ext = abs_path.substring(pointer+1,abs_path.length());
					}
	
					boolean allowed_dir = true;
					boolean allowed_ext = true;
					
					for(String xd : excluded_dirs) {
						if(path.contains(xd)) {
							allowed_dir = false;
							break;
						}
					}
					
					if(f.isDirectory() == false) {
						
						for(String xfs : excluded_exts) {
							if(ext.contains(xfs)) {
								allowed_ext = false;
								break;
							}
						}
						
						if(allowed_dir && allowed_ext) {
							files.add(f.getAbsolutePath());
							res_files.add(f.getAbsolutePath());

								Path tpath = Paths.get(file.getAbsolutePath());	
								FileTime fileTime = null;
								
								fileTime = Files.getLastModifiedTime(tpath, LinkOption.NOFOLLOW_LINKS);
								
								Instant instant = fileTime.toInstant();
							    DateTimeFormatter DATE_TIME_FORMATTER = DateTimeFormatter.ofPattern("MM/dd/yyyy - hh:mm:ss").withZone(ZoneId.systemDefault());
								String time = DATE_TIME_FORMATTER.format(instant);
								
								res_times.add(time);
						}
					}
					else {
						if(allowed_dir) {
							directories.add(path);
							res_dirs.add(path);
						}
					}
				}
				
				//For each folder within a folder, go into it.
				for(String d : directories) {
					long sstart = System.nanoTime();
					
					File f = new File(d);
					long size = f.getTotalSpace();
					
					level++;
					doDir(str_root,d);
					level--;
					
					accounted = size;
					
					long end = System.nanoTime();
					long result = (end-sstart)/1000000;
					
					if(true_files_per_second == 0) {
						true_files_per_second = 1;
					}
					
					last_rate = true_files_per_second;
				}
			}
		}
		catch(Exception e) {
			e.printStackTrace();
		}
	}
	
	static long accounted = 0;
	
	static StringBuilder sb = new StringBuilder();
	static int last_dir = 0;
	private JPanel panel_4;
	private Component verticalStrut_1;
	private JButton btnNewButton;
	private JButton btnNewButton_1;
	private Component horizontalStrut_1;
	private JTextField textField;
	private JButton btnNewButton_2;
	private static void updateExploredDirs() {

		for(int i = last_dir; i < res_dirs.size(); i++) {
			sb.append(res_dirs.get(i) + "\n");
		}
		
		last_dir = res_dirs.size();
		
		textPane_1.setText(sb.toString());
	}




	static void delay(int i) {
		try {
			Thread.sleep(i);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}
}
