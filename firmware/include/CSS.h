void append_page_header() {

  webpage  = F("<!DOCTYPE html><html>");
  webpage += F("<head>");
  webpage += F("<title>ESP32 Datalogger Webserver - INTI</title>"); // NOTE: 1em = 16px
  webpage += F("<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>");
  webpage += F("<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>");
  webpage += F("<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@4.0.0/dist/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>");
 	
  webpage += F("<style>");
  webpage += F("body{max-width:65%;margin:0 auto;font-family:arial;font-size:105%;text-align:center;color:blue;background-color:#F7F2Fd;}");
  webpage += F("ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.375em;overflow:hidden;background-color:#dcade6;font-size:1em;}");
  webpage += F("li{float:left;border-radius:0.375em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}");
  webpage += F("li a{display: block;border-radius:0.375em;padding:0.44em 0.44em;text-decoration:none;font-size:85%}");
  webpage += F("li a:hover{background-color:#EAE3EA;border-radius:0.375em;font-size:85%}");
  webpage += F("section {font-size:0.88em;}");
  webpage += F("h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#558ED5;}");
  webpage += F("h2{color:orange;font-size:1.0em;}");
  webpage += F("h3{font-size:0.8em;}");
  webpage += F("table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:85%;}"); 
  //webpage += F("th,td {border:0.06em solid #dddddd;text-align:left;padding:0.3em;border-bottom:0.06em solid #dddddd;}"); 
  webpage += F("th,td {border:0.06em solid #dddddd;text-align:center;padding:0.3em;border-bottom:0.06em solid #dddddd;}"); 
  webpage += F("tr:nth-child(odd) {background-color:#eeeeee;}");
  webpage += F(".rcorners_n {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:75%;}");
  webpage += F(".rcorners_m {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:50%;color:white;font-size:75%;}");
  webpage += F(".rcorners_w {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:70%;color:white;font-size:75%;}");
  webpage += F(".column{float:left;width:50%;height:45%;}");
  webpage += F(".row:after{content:'';display:table;clear:both;}");
  webpage += F("*{box-sizing:border-box;}");
  webpage += F("footer{background-color:#eedfff; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}");
  webpage += F("button{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:130%;}");
  webpage += F(".buttons {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:80%;}");
  webpage += F(".buttonsm{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:9%; color:white;font-size:70%;}");
  webpage += F(".buttonm {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:70%;}");
  webpage += F(".buttonw {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:40%;color:white;font-size:70%;}");
  webpage += F(".btn {background-color: DodgerBlue;border: none;color: white;padding: 12px 30px;cursor: pointer;font-size: 20px;}");
  webpage += F(".btn:hover {background-color: RoyalBlue;}");


  webpage += F("a{font-size:75%;}");
  webpage += F("p{font-size:75%;}");
  webpage += F("</style></head><body><table><tr><td><h1>ESP32 Datalogger Webserver - INTI</h1></td>");
  //if(flagWebserverLibre == 0){//si está ocupado deshabilita el login
  //if(flagUsuarioLogueado == 1){//si está logueado deshabilita el login y habilita el logout
/*
  if(flagClienteNuevo == 0){//si está logueado deshabilita el login y habilita el logout
	webpage += ("<td><form action='/login' method='post'><input type='hidden' name='login' id='login' value='login'><input type='Submit' value='Login' disabled></form></td><td>Usuario: " + usernameLogin + "</td><td><form action='/logout' method='post'><input type='hidden' name='logout' id='logout' value='logout'><input type='Submit' value='Logout'></form></td></tr></table>");	  
  //}else{//si está libre deshabilita el logout
  }else{//si no está logueado habilita el login y deshabilita el logout
	webpage += ("<td><form action='/login' method='post'><input type='hidden' name='login' id='login' value='login'><input type='Submit' value='Login'></form></td><td>Usuario: </td><td><form action='/logout' method='post'><input type='hidden' name='logout' id='logout' value='logout'><input type='Submit' value='Logout' disabled></form></td></tr></table>");
  }
*/
/*
	if(flagUsuarioLogueado == 0 && flagClienteNuevo == 1){//logout deshabilitado, login habilitado
		webpage += ("<td><form action='/login' method='post'><input type='hidden' name='login' id='login' value='login'><input type='Submit' value='Login'></form></td><td>Usuario: </td><td><form action='/logout' method='post'><input type='hidden' name='logout' id='logout' value='logout'><input type='Submit' value='Logout' disabled></form></td></tr></table>");
	}else if(flagUsuarioLogueado == 1 && flagClienteNuevo == 1){//logout deshabilitado, login deshabilitado
		webpage += ("<td><form action='/login' method='post'><input type='hidden' name='login' id='login' value='login'><input type='Submit' value='Login' disabled></form></td><td>Usuario: "+ usernameLogin +"</td><td><form action='/logout' method='post'><input type='hidden' name='logout' id='logout' value='logout'><input type='Submit' value='Logout' disabled></form></td></tr></table>");
	}else if(flagUsuarioLogueado == 1 && flagClienteNuevo == 0){//logout habilitado, login deshabilitado
		webpage += ("<td><form action='/login' method='post'><input type='hidden' name='login' id='login' value='login'><input type='Submit' value='Login' disabled></form></td><td>Usuario: "+ usernameLogin +"</td><td><form action='/logout' method='post'><input type='hidden' name='logout' id='logout' value='logout'><input type='Submit' value='Logout'></form></td></tr></table>");
	}
*/
/*
	if (server.hasArg("logout")){//si se desloguea

		Serial.print("Llegó argumento Logout");
		flagUsuarioLogueado = 0;
		flagClienteNuevo = 0;
		usernameLogin = "";
	
	}
	if (server.hasArg("reintentar")){//si se desloguea

		flagUsuarioLogueado = 0;
		flagClienteNuevo = 0;
		usernameLogin = "";
	
	}
*/
	if(flagUsuarioLogueado == 0 && flagClienteNuevo == 1){//logout deshabilitado, login habilitado
		webpage += ("<td>Usuario: </td><td><form action='/' method='post'><input type='hidden' name='logout' id='logout' value='logout'><input type='Submit' value='Logout' disabled></form></td></tr></table>");
	}else if(flagUsuarioLogueado == 1 && flagClienteNuevo == 1){//logout deshabilitado, login deshabilitado
		webpage += ("<td>Usuario: "+ usernameLogin +"</td><td><form action='/' method='post'><input type='hidden' name='logout' id='logout' value='logout'><input type='Submit' value='Logout' disabled></form></td></tr></table>");
		webpage += ("<form action='/' method='post'><input type='hidden' name='refresh' id='refresh' value='refresh'><input type='Submit' value='Refrescar'></form>");
	}else if(flagUsuarioLogueado == 1 && flagClienteNuevo == 0){//logout habilitado, login deshabilitado
		webpage += ("<td>Usuario: "+ usernameLogin +"</td><td><form action='/' method='post'><input type='hidden' name='logout' id='logout' value='logout'><input type='Submit' value='Logout'></form></td></tr></table>");
	}else if(flagUsuarioLogueado == 0 && flagClienteNuevo == 0){//logout deshabilitado, login habilitado
		webpage += ("<td>Usuario: "+ usernameLogin +"</td></tr></table>");
	}			

  
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void append_page_footer(){ // Saves repeating many lines of code for HTML page footers

/*
  webpage += F("<ul>");
  webpage += F("<li><a href='/'>Home</a></li>"); // Lower Menu bar command entries
  webpage += F("<li><a href='/download'>Download</a></li>"); 
  webpage += F("<li><a href='/upload'>Upload</a></li>"); 
  webpage += F("<li><a href='/stream'>Stream</a></li>"); 
  webpage += F("<li><a href='/delete'>Delete</a></li>"); 
  webpage += F("<li><a href='/dir'>Directory</a></li>");
  webpage += F("</ul>");
  webpage += "<footer>&copy;"+String(char(byte(0x40>>1)))+String(char(byte(0x88>>1)))+String(char(byte(0x5c>>1)))+String(char(byte(0x98>>1)))+String(char(byte(0x5c>>1)));
  webpage += String(char((0x84>>1)))+String(char(byte(0xd2>>1)))+String(char(0xe4>>1))+String(char(0xc8>>1))+String(char(byte(0x40>>1)));
  webpage += String(char(byte(0x64/2)))+String(char(byte(0x60>>1)))+String(char(byte(0x62>>1)))+String(char(0x70>>1))+"</footer>";
  */
  webpage += F("</body></html>");
}
