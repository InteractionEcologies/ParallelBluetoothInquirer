<?
$host="localhost";
$user="multipleBT";
$password="multipleBT";
$database="BTSignalRecords";
$table="btSignalRecords";

header("content-type:text/xml");

function getXML($sql="Default Query", $conn)
{
	$result = mysql_query($sql, $conn);
	$columns="";
	while($row=mysql_fetch_assoc($result)){
		$columns.="<record>\n";
		foreach($row as $key => $value){
			$columns.="<$key>$value</$key>\n";
		}
		$columns.="</record>\n";
	}
	return $columns;
}

function main()
{
	global $host, $user, $password, $database, $table;
	$conn=mysql_connect($host, $user, $password);
	$db=mysql_select_db($database);
	
	# Get all machineIDs
	echo 'start to get ids...';
	$query = "SELECT DISTINCT machineID from $table"; 
	$ids = mysql_query($query, $conn);
	echo 'ok';

	$idCounter=0;
	# Generate a xml file for each machineIDs
	while($id=mysql_fetch_assoc($ids)){
		echo "id is {$id["machineID"]}:\n";
		$mFile = "btID_{$idCounter}.xml";
		$fh = fopen($mFile, 'w') or die("can't open file");
		
		$queryForAnId = "SELECT machineID, timeDate, timeStamp, deviceClass, location FROM $table WHERE machineID = '{$id["machineID"]}'";
		echo "query = $queryForAnId\n";	
		$xmlString = getXML($queryForAnId, $conn);
		
		fwrite($fh, $xmlString);
		fclose($fh);
		$idCounter++;
	}
}

main();
