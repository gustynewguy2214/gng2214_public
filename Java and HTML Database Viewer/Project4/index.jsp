<!DOCTYPE html>
<!-- Based off of welcome.jsp -->

<%
    String textBox = (String) session.getAttribute("textBox");
    String result = (String) session.getAttribute("result");
	
   if(result == null){
        result = " ";
   }
   if(textBox == null){
       textBox = " ";
   }
%>

<html lang="en">
   <!-- head section of document -->
   <head>
      <title>CNT 4714 Remote Database Management System</title>
	  <script src="reset_form.js"></script> 
	  <meta charset="utf-8">
	  <style type="text/css">
		body { 
			background-color: blue;
			color:white; font-family: verdana, arial, sans-serif;
			font-size: 18px;
		}
		h1 { color:white; text-align:center; font-family: serif; font-size: 37px}
		h2 { color:white; text-align:center; font-family: serif; font-size: 16px}
		h3 { font-family: arial; font-size: 18px}
		h4{font-size: 18px; color:black;}
		h5{font-family: arial; font-size: 16px; font-weight:normal;}
		textarea {background-color: black; color: lime;}
		input[type="submit"]{background-color:black; color:yellow; font-weight:bold;}
		button{background-color:black; color:yellow; font-weight:bold;}
		span {color: white;}
		table{
			border: solid black;
			color:black;
			font-family: arial;
		}
		th{
			font-weight: normal;
			font-size: 16px;
		}
		p{
			font-family:serif; color:white; text-align: center; font-size:19px;
		}
		.serif {
			font-family: "Times New Roman", Times, serif;
		}
		.sans-serif{
			font-family: "Courier New",Courier,monospace; 
		}
	  </style>
   </head>
   <!-- body section of document -->
   <h1> Welcome to the Spring 2020 Project 4 Enterprise System </h1>
   <h1> A Remote Database Management System </h1>
   <body>
	<hr>
	   <p>
		   You are connected to the Project 4 Enterprise System database.<br>
		   Please enter any valid SQL query or update statement.<br>
		   If no query/update command is initially provided the
		   Execute button will display all supplier information in the database.<br>
		   All execution results will appear below.
	   </p>
	   <form action = "MySQLServlet" method = "post">
               <p><textarea name = "textBox" rows="10" cols="90"></textarea>
					<p><br></p>
					<p>
					<input type = "submit" value = "Execute Command" />
					<button onClick="reset();" style="margin-bottom: 15px;" type="reset" class="btn btn-dark">Clear Form</button>
					</p>
               </p>
            </form>
			
	<hr>
	<h2> Database Results: </h2>
	<TABLE BORDER=1 align='center'>
        <%= result %>
    </TABLE>
   </body>
</html>  <!-- end HTML document -->


