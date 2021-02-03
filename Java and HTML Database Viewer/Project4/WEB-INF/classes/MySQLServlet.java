
// Based on SurveyServlet.java

import java.io.IOException;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Statement;

import javax.servlet.RequestDispatcher;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.UnavailableException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;

public class MySQLServlet extends HttpServlet 
{
	private static final long serialVersionUID = 1L;
	private Connection connection;
	private Statement statement;

	// set up database connection and create SQL statement
	public void init( ServletConfig config ) throws ServletException
	{
		// attempt database connection and create Statement
		try 
		{
			Class.forName("com.mysql.cj.jdbc.Driver");
			connection = DriverManager.getConnection("jdbc:mysql://localhost:3312/project4", "root", "root" );
			statement = connection.createStatement();
		} // end try
		// for any exception throw an UnavailableException to 
		// indicate that the servlet is not currently available
		catch ( Exception exception ) 
		{
			//exception.printStackTrace();
			throw new UnavailableException( exception.getMessage() );
		} // end catch
	}  // end method init 
	
	HttpServletRequest request_ref;
	
	// process survey response
	protected void doPost( HttpServletRequest request,  HttpServletResponse response ) throws ServletException, IOException		
	{
		String query = request.getParameter("textBox");
		String result = null;
		StringBuilder sb = new StringBuilder();
		request_ref = request;
		
		try 
		{ 
			ResultSet resultSet;
			int affected = 0;
			
			if(query.length() != 0) 
			{
				String[] query_components = query.split(" ");
				  
				// set query or update and then execute.
				if(query_components[0].toLowerCase().contains("SELECT".toLowerCase())) 
				{	
					resultSet = statement.executeQuery( query );
					resultSet.beforeFirst();
					ResultSetMetaData metaData = resultSet.getMetaData();
					  
					while(resultSet.isAfterLast() == false) {
						 
						String color = "'background-color:red'";
						 
						int row = resultSet.getRow();
						 
						if(row % 2 == 1) 
						{
							color = "'background-color:#CCCCCC'";
						}
						else if(row % 2 == 0 && row != 0) 
						{
							color = "'background-color:white'";
						}
						 
						sb.append("<TR style=" + color + ">");
						for(int i = 1; i < metaData.getColumnCount() + 1; i++) 
						{
							if(resultSet.isBeforeFirst() == false) 
							{
								sb.append("<TH>" + resultSet.getObject(i));
							}
							else 
							{
								sb.append("<TH><b>" + metaData.getColumnName(i) + "</b>");
							}
						}
						resultSet.next();
					}
				}
				else 
				{  
					
					int marks = 0;

					statement.executeUpdate("create table beforeShipments like shipments");
					statement.executeUpdate("insert into beforeShipments select * from shipments");

					affected = statement.executeUpdate(query);

					marks = statement.executeUpdate(
							"update suppliers "
							  + "set status = status + 5 "
							  + "where snum in ( "
							  +   "select distinct snum "
							  +   "from shipments left join beforeShipments "				//13,10
							  +      "using (snum, pnum, jnum, quantity) "
							  +   "where "
							  +      "beforeShipments.snum is null and quantity >= 100"
							  + ")"
							);
					
					statement.executeUpdate("drop table beforeShipments");

					sb.setLength(0);	//Clear sb.
					sb.append("<TR style=" + "'background-color:lime; color:white;'" + ">");
					sb.append("<TH><h4>The statement executed successfully.<br>" + affected + " row(s) affected.</h4>");
					
					if(marks > 0) 
					{
						sb.append("<b>Business Logic Detected! - Updating Supplier Status</b><br><br>");
						sb.append("<b>Business Logic updated " + marks + " supplier status marks.</b><br>");
					}
				}
			}
			 
			result = sb.toString();

		} // end try
		// if database exception occurs, return error page
		catch ( Exception e ) 
		{
			sb.setLength(0);//Clear sb.
			sb.append("<TR style=" + "'background-color:red; color:white;'" + ">");
			sb.append("<TH><b>Error executing the SQL statement:</b><br><abbr>" + e.getLocalizedMessage() + "</abbr>");
			 
			result = sb.toString();
		} // end catch
		finally
		{
			HttpSession session = request.getSession();
			session.setAttribute("result", result);
			session.setAttribute("textBox", query);
			RequestDispatcher dispatcher = session.getServletContext().getRequestDispatcher("/index.jsp");
			dispatcher.forward(request, response);
		}
	} // end method doPost

	// close SQL statements and database when servlet terminates - called by the Tomcat container on termination
	public void destroy()
	{
		// attempt to close statements and database connection
		try 
		{
			statement.close();
			connection.close();
		} // end try
		// handle database exceptions by returning error to client
		catch( SQLException sqlException ) 
		{
			//sqlException.printStackTrace();
		} // end catch
	} // end method destroy
} // end class SurveyServlet

