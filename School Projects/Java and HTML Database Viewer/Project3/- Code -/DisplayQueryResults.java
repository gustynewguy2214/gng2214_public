import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Font;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.sql.Connection;
import java.sql.SQLException;
import java.util.ArrayList;

import javax.swing.DefaultComboBoxModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.ScrollPaneConstants;
import javax.swing.SwingConstants;
import javax.swing.table.DefaultTableModel;

import com.mysql.cj.jdbc.MysqlDataSource;

public class DisplayQueryResults extends JFrame {

	private static final long serialVersionUID = 1L;
   
	private JTextArea queryArea;
	private JTextField databaseUserField;
	private JPasswordField databasePasswordField;
	private JComboBox<String> databaseURLDropdownMenu;
	private JLabel connection_status_label;
	private JScrollPane scrollPane_1;
   
	private Connection connection;
	private boolean connectionEstablished;
   
	private MysqlDataSource dataSource = new MysqlDataSource();
	private ResultSetTableModel tableModel;
	private JTable resultTable;

   // create ResultSetTableModel and GUI
	public DisplayQueryResults() {   
      
		super( "SQL Client GUI - (MUL - CNT 4714 - Spring 2020)" );

		tableModel = null;
		getContentPane().setLayout(new GridLayout(0, 1, 0, 0));
		
		JPanel upper_panel = new JPanel();
		getContentPane().add(upper_panel);
		upper_panel.setLayout(null);
		
		JPanel lower_panel = new JPanel();
        getContentPane().add(lower_panel);
		lower_panel.setLayout(new BorderLayout(0, 0));
		
		// create JTable delegate for tableModel 
		resultTable = new JTable( tableModel );
		resultTable.setFillsViewportHeight(true);
		scrollPane_1 = new JScrollPane( resultTable );
		lower_panel.add( scrollPane_1 );
       
		JLabel lblNewLabel = new JLabel("Enter Database Information");
		lblNewLabel.setForeground(Color.BLUE);
		lblNewLabel.setFont(new Font("Arial", Font.BOLD, 16));
		lblNewLabel.setBounds(5, 7, 272, 14);
		upper_panel.add(lblNewLabel);
		
		JPanel panel_2 = new JPanel();
		panel_2.setBounds(5, 25, 90, 30);
		upper_panel.add(panel_2);
		panel_2.setBackground(Color.LIGHT_GRAY);
		panel_2.setLayout(new BorderLayout(0, 0));
		
		JLabel lblNewLabel_1 = new JLabel("JDBC Driver");
		panel_2.add(lblNewLabel_1);
		lblNewLabel_1.setBackground(Color.GRAY);
		lblNewLabel_1.setFont(new Font("Arial Narrow", Font.BOLD, 16));
		
		JPanel panel_5 = new JPanel();
		panel_5.setBounds(5, 60, 90, 30);
		upper_panel.add(panel_5);
		panel_5.setBackground(Color.LIGHT_GRAY);
		panel_5.setLayout(new BorderLayout(0, 0));
         
		JLabel lblDatabaseUrl = new JLabel("Database URL");
		panel_5.add(lblDatabaseUrl, BorderLayout.WEST);
		lblDatabaseUrl.setFont(new Font("Arial Narrow", Font.BOLD, 16));
		lblDatabaseUrl.setBackground(Color.GRAY);
		
		JPanel panel_8 = new JPanel();
		panel_8.setBounds(5, 95, 90, 30);
		upper_panel.add(panel_8);
		panel_8.setBackground(Color.LIGHT_GRAY);
		panel_8.setLayout(new BorderLayout(0, 0));
		
		JLabel lblUsername = new JLabel("Username");
		panel_8.add(lblUsername, BorderLayout.WEST);
		lblUsername.setFont(new Font("Arial Narrow", Font.BOLD, 16));
		lblUsername.setBackground(Color.GRAY);
		
		JPanel panel_6 = new JPanel();
		panel_6.setBounds(5, 130, 90, 30);
		upper_panel.add(panel_6);
		panel_6.setBackground(Color.LIGHT_GRAY);
		panel_6.setLayout(new BorderLayout(0, 0));
		        
		JLabel lblPassword = new JLabel("Password");
		panel_6.add(lblPassword, BorderLayout.WEST);
		lblPassword.setFont(new Font("Arial Narrow", Font.BOLD, 16));
		lblPassword.setBackground(Color.GRAY);
		
		String[] comboBoxList = new String[] {"com.mysql.cj.jdbc.Driver"};
		JComboBox<String> comboBox = new JComboBox<String>();
		comboBox.setBounds(95, 25, 260, 30);
		upper_panel.add(comboBox);
		comboBox.setModel(new DefaultComboBoxModel<String>(comboBoxList));
		
		ArrayList<String> dropdownOptions = new ArrayList<String>();
		dropdownOptions.add("jdbc:mysql://localhost:3312/project3");
		dropdownOptions.add("jdbc:mysql://localhost:3312/bikedb");
		
		String[] ddo_array = new String[dropdownOptions.size()];
		for(int i = 0; i < dropdownOptions.size(); i++) ddo_array[i] = dropdownOptions.get(i);
		
		databaseURLDropdownMenu = new JComboBox<String>();
		databaseURLDropdownMenu.setBounds(95, 60, 260, 30);
		upper_panel.add(databaseURLDropdownMenu);
		databaseURLDropdownMenu.setModel(new DefaultComboBoxModel<String>(ddo_array));
		        
		databaseUserField = new JTextField();
		databaseUserField.setBounds(95, 95, 260, 30);
		upper_panel.add(databaseUserField);
		databaseUserField.setColumns(10);
		        
		databasePasswordField = new JPasswordField();
		databasePasswordField.setBounds(95, 130, 260, 30);
		upper_panel.add(databasePasswordField);
        
        JButton add_url_button = new JButton("+");
        add_url_button.addMouseListener(new MouseAdapter() {
        	@Override
        	public void mouseClicked(MouseEvent e) {
        		
        		String newURL = JOptionPane.showInputDialog("Enter a new URL");
        		if(newURL != "" && newURL != null && newURL.length() > 0) {
        			dropdownOptions.add(newURL);
        			String[] ddo_array = new String[dropdownOptions.size()];
        			for(int i = 0; i < dropdownOptions.size(); i++) ddo_array[i] = dropdownOptions.get(i);
        			databaseURLDropdownMenu.setModel(new DefaultComboBoxModel<String>(ddo_array));
        		}
        	}
        });
        add_url_button.setToolTipText("Add a URL to the dropdown. Must follow the format: protocol//[hosts]/[database] to succeed.");
        add_url_button.setBounds(357, 64, 41, 23);
        upper_panel.add(add_url_button);
       
        JScrollPane scrollPane = new JScrollPane( ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED, ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER );
        scrollPane.setBounds(418, 25, 459, 152);
        upper_panel.add(scrollPane);
       
        // set up JTextArea in which user types queries
		queryArea = new JTextArea( 3, 100);
        //queryArea = new JTextArea( DEFAULT_QUERY, 3, 100 );
        scrollPane.setViewportView(queryArea);
        queryArea.setWrapStyleWord( true );
        queryArea.setLineWrap( true );
       
        JLabel lblEnterAnSql = new JLabel("Enter An SQL Command");
        lblEnterAnSql.setBounds(418, 6, 210, 19);
        upper_panel.add(lblEnterAnSql);
        lblEnterAnSql.setForeground(Color.BLUE);
        lblEnterAnSql.setFont(new Font("Arial", Font.BOLD, 16));
       
        JPanel panel_1 = new JPanel();
        panel_1.setBackground(Color.BLACK);
        panel_1.setBounds(4, 230, 300, 23);
        upper_panel.add(panel_1);
        panel_1.setLayout(new BorderLayout(0, 0));
       
        connection_status_label = new JLabel("No Connection Now");
        connection_status_label.setFont(new Font("Tahoma", Font.BOLD, 11));
        connection_status_label.setForeground(Color.RED);
       	connection_status_label.setBackground(Color.BLACK);
       	panel_1.add(connection_status_label);
       
       	JButton btnNewButton_1 = new JButton("Clear SQL Command");
       	btnNewButton_1.setForeground(Color.RED);
       	btnNewButton_1.setBackground(Color.WHITE);
       	btnNewButton_1.setBounds(502, 230, 196, 23);
       	upper_panel.add(btnNewButton_1);
       	btnNewButton_1.addMouseListener(new MouseAdapter() {
    	   
    	   public void mouseClicked(MouseEvent e) {
    		   
    		   queryArea.setText("");
    		   
    	   }
       	});
       
       	JButton btnNewButton_2 = new JButton("Connect to Database");
       	btnNewButton_2.addMouseListener(new MouseAdapter() {
       		public void mouseClicked(MouseEvent e) {
       		
	       		final String user = databaseUserField.getText();
	       		final String password = String.copyValueOf(databasePasswordField.getPassword());
	       		final String url = databaseURLDropdownMenu.getSelectedItem().toString();

	       		Thread t = new Thread() {
	       			public void run() {
       				
	       				if(connectionEstablished) {
	       					
	       					try {
	       						if(tableModel != null) tableModel.disconnectFromDatabase();
	       					}
	       					catch(Exception e) {
	       						
	       						e.printStackTrace();
	       						
	       					}
	       					finally {
	
	       						resultTable = new JTable( new DefaultTableModel() );
	       		         		scrollPane_1.setViewportView(resultTable);
	       		         		tableModel = null;
	       		         		queryArea.setText("");
	       		         	
	       						dataSource.setUser("");
	               	       		dataSource.setPassword("");
	               	       		dataSource.setUrl("");
	               	       		connection = null;
	               	       		connectionEstablished = false;
	               	       		connection_status_label.setText("No connection now.");
	               	       		btnNewButton_2.setText("Connect to database");
	               	       		
	       					}
	       				}
	       				else {
	       					
	       					dataSource.setUser(user);
	           	       		dataSource.setPassword(password);
	           	       		dataSource.setUrl(url);

	           	       		try {
	           	       			
	    						connection = dataSource.getConnection();
	    						connectionEstablished = true;
	    						connection_status_label.setText("Connected to: " + url);
	
	    						btnNewButton_2.setText("Disconnect from Database");
	    						
	    					} 
	           	       		catch (SQLException e) {

	           	       			String top_error = null;
	           	       			String bottom_error = null; 
	           	       			
	           	       			try {
	           	       				
	           	       				top_error = e.getLocalizedMessage();
	           	       				bottom_error = getRootCause(e).getMessage();
	           	       				
	           	       			}
	           	       			catch(java.lang.NullPointerException npe) {
	           	       				
	           	       				
	           	       				
	           	       			}
	           	       			
		           	       		if(top_error.contains("denied")) {
	           	       				
	           	       				if(top_error.contains("to database")) showMessage(2,"Connection Error","ERROR: You do not have permission to access the " + url.split("/")[url.split("/").length - 1] + " database.");
	           	       				else showMessage(2, "Connection Error", "ERROR: Your username, password, or URL is invalid. Please try again.");
	           	       				
	           	       			}
	           	       			else if(top_error.contains("Unknown")){
		           	       			
	           	       				showMessage(2, "Connection Error", "ERROR: " + e.getLocalizedMessage() + ".\nThe database may not exist.");
	           	       				
	           	       			}
	           	       			else if(top_error.contains("connection string")) {
	           	       				
	           	       				showMessage(2,"Connection Error","ERROR: Invalid URL format. Please ensure your URL matches the format:\nprotocol//[hosts]/[database] before re-trying.");
	           	       				
	           	       			}
	           	       			else if(bottom_error.contains("refused")) {
	           	       				
	           	       				showMessage(2,"Connection Error", "ERROR: There is no service listening on port: " + url.split("/")[2]);
	           	       				
	           	       			}
	    					}
	       				}
	       			}
	       		};
	       		t.start();
       		
       		}
       	});
       	btnNewButton_2.setFont(new Font("Tahoma", Font.BOLD, 11));
       	btnNewButton_2.setBackground(Color.BLUE);
       	btnNewButton_2.setForeground(Color.YELLOW);
       	btnNewButton_2.setBounds(308, 230, 184, 23);
       	upper_panel.add(btnNewButton_2);

       	JLabel lblSqlExecutionResult = new JLabel("SQL Execution Result Window");
       	lblSqlExecutionResult.setBounds(5, 264, 272, 14);
       	upper_panel.add(lblSqlExecutionResult);
       	lblSqlExecutionResult.setForeground(Color.BLUE);
       	lblSqlExecutionResult.setFont(new Font("Arial", Font.BOLD, 14));
        
        JPanel panel_7 = new JPanel();
        panel_7.setBounds(357, 271, 10, 10);
        lower_panel.add(panel_7,BorderLayout.SOUTH);
        
        JButton btnNewButton = new JButton("Clear Result Window");
        btnNewButton.setHorizontalAlignment(SwingConstants.LEFT);
        panel_7.add(btnNewButton);
        btnNewButton.addMouseListener(new MouseAdapter() {
        	@Override
        	public void mouseClicked(MouseEvent e) {
         		resultTable = new JTable( new DefaultTableModel() );
        		scrollPane_1.setViewportView(resultTable);
        		tableModel = null;
        		
        	}
        });
        btnNewButton.setForeground(new Color(85, 107, 47));
        btnNewButton.setFont(new Font("Tahoma", Font.BOLD, 12));
        btnNewButton.setBackground(Color.YELLOW);
        
        JPanel panel_12 = new JPanel();
        panel_7.add(panel_12);
        
        JPanel panel_13 = new JPanel();
        panel_7.add(panel_13);
        
        JPanel panel_14 = new JPanel();
        panel_7.add(panel_14);
        
        JPanel panel_15 = new JPanel();
        panel_7.add(panel_15);
        
        JPanel panel_16 = new JPanel();
        panel_7.add(panel_16);
        
        JPanel panel_17 = new JPanel();
        panel_7.add(panel_17);
        
        JPanel panel_18 = new JPanel();
        panel_7.add(panel_18);
        
        JPanel panel_19 = new JPanel();
        panel_7.add(panel_19);
        
        JPanel panel_20 = new JPanel();
        panel_7.add(panel_20);
        
        JPanel panel_21 = new JPanel();
        panel_7.add(panel_21);
        
        JPanel panel_22 = new JPanel();
        panel_7.add(panel_22);
        
        JPanel panel_23 = new JPanel();
        panel_7.add(panel_23);
        
        JPanel panel_24 = new JPanel();
        panel_7.add(panel_24);
        
        JPanel panel_25 = new JPanel();
        panel_7.add(panel_25);
        
        JPanel panel_26 = new JPanel();
        panel_7.add(panel_26);
        
        JPanel panel_27 = new JPanel();
        panel_7.add(panel_27);
        
        JPanel panel_28 = new JPanel();
        panel_7.add(panel_28);
        
        JPanel panel_29 = new JPanel();
        panel_7.add(panel_29);
        
        JPanel panel_30 = new JPanel();
        panel_7.add(panel_30);
        
        JPanel panel_31 = new JPanel();
        panel_7.add(panel_31);
        
        JPanel panel_32 = new JPanel();
        panel_7.add(panel_32);
        
        JPanel panel_33 = new JPanel();
        panel_7.add(panel_33);
        
        JPanel panel_34 = new JPanel();
        panel_7.add(panel_34);
        
        JPanel panel_35 = new JPanel();
        panel_7.add(panel_35);
        
        JPanel panel_36 = new JPanel();
        panel_7.add(panel_36);
        
        JPanel panel_37 = new JPanel();
        panel_7.add(panel_37);
        
        JPanel panel_38 = new JPanel();
        panel_7.add(panel_38);
        
        JPanel panel_39 = new JPanel();
        panel_7.add(panel_39);
        
        JPanel panel_40 = new JPanel();
        panel_7.add(panel_40);
        
        JPanel panel_41 = new JPanel();
        panel_7.add(panel_41);
        
        JPanel panel_42 = new JPanel();
        panel_7.add(panel_42);
        
        JPanel panel_43 = new JPanel();
        panel_7.add(panel_43);
        
        JPanel panel_44 = new JPanel();
        panel_7.add(panel_44);
        
        JPanel panel_45 = new JPanel();
        panel_7.add(panel_45);
        
        JPanel panel_46 = new JPanel();
        panel_7.add(panel_46);
        
        JPanel panel_47 = new JPanel();
        panel_7.add(panel_47);
        
        JPanel panel_48 = new JPanel();
        panel_7.add(panel_48);
        
        JPanel panel_49 = new JPanel();
        panel_7.add(panel_49);
        
        JPanel panel_50 = new JPanel();
        panel_7.add(panel_50);
        
        JPanel panel_51 = new JPanel();
        panel_7.add(panel_51);
        
        JPanel panel_52 = new JPanel();
        panel_7.add(panel_52);
        
        JPanel panel_53 = new JPanel();
        panel_7.add(panel_53);
        
        JPanel panel_54 = new JPanel();
        panel_7.add(panel_54);
               	
       	JPanel panel_4 = new JPanel();
       	panel_4.setBounds(485, 264, 10, 10);
       	lower_panel.add(panel_4,BorderLayout.EAST);
       	
       	JPanel panel = new JPanel();
       	panel.setBounds(647, 268, 10, 10);
       	lower_panel.add(panel,BorderLayout.WEST);
       	
       	JPanel panel_3 = new JPanel();
       	panel.add(panel_3);
       	
       	JPanel panel_9 = new JPanel();
       	panel_4.add(panel_9);

       	
        // set up JButton for submitting queries
        JButton submitButton = new JButton( "Execute SQL Command" );
        submitButton.setFont(new Font("Tahoma", Font.BOLD, 11));
        submitButton.setBounds(704, 230, 173, 23);
        upper_panel.add(submitButton);
        submitButton.setBackground(Color.GREEN);
        submitButton.setForeground(new Color(0, 100, 0));
        // create event listener for submitButton
        submitButton.addActionListener( 
        	new ActionListener() {
            	
               // pass query to table model
               public void actionPerformed( ActionEvent event ){
                  // perform a new query
                  try {
                	 
                	 if(connectionEstablished) {
                		 
                		 if(tableModel == null) {
                    		 
                			 if(queryArea.getText().length() != 0) {
                				 
                				 tableModel = new ResultSetTableModel(connection, queryArea.getText());
                        		 resultTable.setModel(tableModel);
                				 
                			 }
                    	 }
                		 
                		 if(tableModel != null && queryArea.getText().length() != 0) {
                			 
                			 String query = queryArea.getText();
                			 String[] query_components = query.split(" ");
                			 
            	           // set query or update and then execute.
                			if(query_components[0].toLowerCase().contains("SELECT".toLowerCase())) {
            	        	   tableModel.setQuery( query );
            	           	}
            	           	else {
            	        	   
            	        	    tableModel.setUpdate (query);
            	        		showMessage(0,null,"Operation successful.");
            	        		
            	           	}
                		 }
                	 }
                	 else {
                		 
                		 showMessage(2,null,"ERROR: Can't execute a command without a valid connection.");
                		 
                	 }
                  } // end try
                  catch ( SQLException sqlException ) {
                     JOptionPane.showMessageDialog( null, 
                        sqlException.getMessage(), "Database error", 
                        JOptionPane.ERROR_MESSAGE );
                     
                                      
                  } // end outer catch
                  catch (ClassNotFoundException e) {
					e.printStackTrace();
                  }
               } // end actionPerformed
            }  // end ActionListener inner class          
        ); // end call to addActionListener

        setSize( 900, 622 ); // set window size
        setVisible( true ); // display window  

        setDefaultCloseOperation( DISPOSE_ON_CLOSE );
      
      // ensure database connection is closed when user quits application
      addWindowListener(new WindowAdapter() {
            // disconnect from database and exit when window has closed
            public void windowClosed( WindowEvent event ) {
               if(tableModel != null) tableModel.disconnectFromDatabase();
               System.exit( 0 );
            } // end method windowClosed
         } // end WindowAdapter inner class
      ); // end call to addWindowListener
   } // end DisplayQueryResults constructor
   
   // execute application
   public static void main( String args[] ){
      new DisplayQueryResults();     
   }
   
   /**
    * Takes a cascading exception e and returns the root cause of it.
    * 
    * @param e - The exception to get the root cause of.
    * @return The root cause.
    */
   private Throwable getRootCause(Exception e) {
 		
	   Throwable cause = e.getCause();
	   
	   if(cause != null) {
		   
		   while(cause.getCause() != null) {
				
				cause = cause.getCause();
				
		   }
		   
	   }

	   return cause;
		
   }
   
   /**
    * Print an information, warning, or error message with a single "OK" option.
    * 
    * @param severity - An integer (1 for warning, 2 for error, any for default) telling how bad this error was.
    * @param title - The title of the error box, if any.
    * @param message - The message to report.
    */
   private void showMessage(int severity, String title, String message) {
		
		int icon = JOptionPane.INFORMATION_MESSAGE;
		
		//Set the icon according to the severity of the message.
		if(severity == 1) icon = JOptionPane.WARNING_MESSAGE;
		else if(severity == 2) icon = JOptionPane.ERROR_MESSAGE;
		
		JOptionPane.showMessageDialog( null, message, title, icon );
		
	}
}


