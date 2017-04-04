<?php
$webRetString = shell_exec("ps aux | grep \"scm_server https\"");
$sshRetString = shell_exec("ps aux | grep \"scm_server ssh\"");
$rtspRetString = shell_exec("ps aux | grep \"scm_server rtsp\"");

$isWebUp = strpos($webRetString, "/usr/bin/");
$isSshUp = strpos($sshRetString, "/usr/bin/");
$isRtspUp = strpos($rtspRetString, "/usr/bin/");

if($isWebUp===FALSE)
{
	exec("sudo /usr/bin/daemonize /usr/bin/scm_server https");
}
if($isSshUp===FALSE)
{
	exec("sudo /usr/bin/daemonize /usr/bin/scm_server ssh");
}
if($isRtspUp===FALSE)
{
	exec("sudo /usr/bin/daemonize /usr/bin/scm_server rtsp");
}

?>