<?php

define('ROOT', dirname(__FILE__));
define('GENERATOR_DIR', ROOT . DIRECTORY_SEPARATOR . 'generators');

spl_autoload_register('rtcsp_autoload');

$makeFile = ROOT . '/Makefile';
if(!file_exists($makeFile)) {
	exit("File " . ROOT . '/Makefile' . ' not exists.'. PHP_EOL);
}

$prefix = '/usr/local';
$fp = @fopen($makeFile, 'rb+');
if($fp) {
	while(!feof($fp)) {
		$buffer = fgets($fp, 2048);
		@list($var, $val) = preg_split('/\s*=\s*/', $buffer, 2);
		if($var === 'prefix') {
			$prefix = trim($val);
		}
	}
	fclose($fp);
} else {
	exit("File " . ROOT . '/Makefile' . ' not read.'. PHP_EOL);
}

$host = 'localhost';
$user = 'root';
$passwd = '';
$database = 'rtcsp';
$port = 3306;
$socket = null;
$charset = 'utf8';

$iniFile = $prefix . '/etc/rtcsp.ini';
if(file_exists($iniFile)) {
	$ini = parse_ini_file($iniFile, true);
	if(isset($ini['my'])) {
		foreach($ini['my'] as $var=>$val) {
			$$var = $val;
		}
	}
}

$dsn = "mysql:dbname=$database;host=$host;port=$port;dbname=$database;charset=$charset";
if($socket) {
	$dsn .= ';unix_socket=' . $socket;
}

$options = array(
    PDO::MYSQL_ATTR_INIT_COMMAND => 'SET NAMES ' . $charset,
);
$driver_options = array(
	PDO::ATTR_CURSOR => PDO::CURSOR_FWDONLY
);

try {
	$pdo = new PDO($dsn, $user, $passwd, $options);
} catch(PDOException $e) {
	echo 'Connection Error:', PHP_EOL;
	echo 'Code: ', $e->getCode(), PHP_EOL;
	echo 'Message: ', $e->getMessage(), PHP_EOL;
	
	debug_print_backtrace();
	exit;
}

$driverName = $pdo->getAttribute(PDO::ATTR_DRIVER_NAME);

$sql = 'SHOW TABLES LIKE ?';

$list = findAll($sql, array('%s%'), PDO::FETCH_COLUMN);
if(empty($list)) {
	echo 'None table.', PHP_EOL;
	exit;
}

echo 'Table list:', PHP_EOL;
foreach($list as $i=>$table) {
	echo "\t", $i, ')', $table, PHP_EOL;
}
echo 'Please input a number for id): ';
@$i = fgets(STDIN, 1024) + 0;
if(isset($list[$i])) {
	$table = $list[$i];
} else if($i<0) {
	$table = first($list);
} else {
	$table = end($list);
}

import('libs');

$class = 'C' . ucfirst($driverName) . 'Column';
$columns = $class::getTableColumns($table);

$modelFile = ROOT . '/models/' . $table . '.h';
confirm($modelFile) or @file_put_contents($modelFile, generator('model-h', get_defined_vars())) or die('file ' . $modelFile . ' not writable.' . PHP_EOL);

$modelFile = ROOT . '/models/' . $table . '.c';
confirm($modelFile) or @file_put_contents($modelFile, generator('model-c', get_defined_vars())) or die('file ' . $modelFile . ' not writable.' . PHP_EOL);

function find($sql, array $params = array()) {
	global $pdo, $driver_options;
	
	try {
		$stmt = $pdo->prepare($sql, $driver_options);
		$stmt->execute($params);
		return $stmt->fetch(PDO::FETCH_CLASS);
	} catch(PDOException $e) {
		echo 'Connection Error:', PHP_EOL;
		echo 'Code: ', $e->getCode(), PHP_EOL;
		echo 'Message: ', $e->getMessage(), PHP_EOL;
	
		debug_print_backtrace();
		exit;
	}
}

function findAll($sql, array $params = array(), $fetchStyle = PDO::FETCH_CLASS) {
	global $pdo, $driver_options;
	
	try {
		$stmt = $pdo->prepare($sql, $driver_options);
		$stmt->execute($params);
		return $stmt->fetchAll($fetchStyle);
	} catch(PDOException $e) {
		echo 'Connection Error:', PHP_EOL;
		echo 'Code: ', $e->getCode(), PHP_EOL;
		echo 'Message: ', $e->getMessage(), PHP_EOL;
	
		debug_print_backtrace();
		exit;
	}
}

function import($alias, $isReturn = false) {
	$path = GENERATOR_DIR . DIRECTORY_SEPARATOR . str_replace('.', DIRECTORY_SEPARATOR, $alias);
	if($isReturn) {
		return $path . '.php';
	} else {
		set_include_path($path . PATH_SEPARATOR . get_include_path());
	}
}

function rtcsp_autoload($class) {
	include $class . '.php';
}

function generator($__view, array $__data) {
	foreach($__data as $var => &$val) {
		$$var = $val;
	}
	unset($var, $val);

	ob_start();
	ob_implicit_flush(false);

	include import('views.' . $__view, true);

	return ob_get_clean();
}

function confirm($file) {
	echo PHP_EOL;
	if(file_exists($file)) {
		do {
			echo 'Is replace file <', $file, '> (Y/n): ';
		
			$c = strtolower(trim(fgets(STDIN)));
		} while($c !== 'y' && $c !== 'n' && $c !== '');
	} else {
		$c = 'y';
	}
	
	if($c === 'n') {
		echo 'Skip generate file <', $file, '>', PHP_EOL;
		return true;
	} else {
		echo 'Generating file <', $file, '> ...', PHP_EOL;
	}
}
