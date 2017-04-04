--------MAIN GOAL----------
On Snitch
- Create a software which:
	- Connects to all enabled servers (available in MySQL) through sockets (reserve one port for each server).
	- Sends a periodic keep-alive message to the servers.
	- Receives servers requests and re-route to snitch's apache.
	- Gets the apache response and send it to the server which requested. (Make sure to separate server connections).

On Server
- Create a software which
	- Receives connections of snitches on the port 2220
	- Manage to connect each snitch to a different port.
	- Manage to send Apache's requests to the right snitch (Snitch Serial Number and IP/Port(Socket)).
---------------------------



--------FIRST STEP---------
- Create a communication channel between the server and the Snitch.
- Create a link between Snitch's Apache and the server connection (study how to connecto to Apache).
- Create a link between Server's Apache and the snitch connection.
- Make those 2 apaches connect to each other.
- Make the server capable of handling more than 1 snitch at the same time.