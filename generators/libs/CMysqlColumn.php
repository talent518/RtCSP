<?php

final class CMysqlColumn extends CBaseColumn {
	public function __construct($field) {
		$this->name = $field->Field;
		$this->isNull = $field->Null === 'YES';
		$this->default = $field->Default;
		$this->comment = $field->Comment;
		
		if(empty($this->comment)) {
			$this->comment = trim(ucwords(preg_replace('/([A-Z])/', ' \1', str_replace('_', ' ', $this->name))));
		}

		@list($this->type, $this->size, $this->option) = preg_split('/[\(\)\s]+/', $field->Type);

		$this->isUnsigned = $this->option === 'unsigned';

		if($this->isUnsigned && $this->default === NULL) {
			$this->default = 0;
		}
		
		$isNum = preg_match('/int|float|double|real|decimal|bit|boolean|serial/', $this->type);
		
		if($isNum && $this->default !== NULL) {
			$this->default += 0;
		}
		
		$u = null;
		if($this->isUnsigned) {
			$u = 'u';
		}
		$this->ctype = $this->type;
		$this->cfield = $this->name;
		switch($this->type) {
			case 'byte':
			case 'tinyint':
				$this->ctype = $u . 'byte';
				break;
			case 'smallint':
			case 'year':
				$this->ctype = $u . 'short';
				break;
			case 'int':
			case 'integer':
			case 'mediumint':
				$this->ctype = $u . 'int';
				break;
			case 'bit':
			case 'bigint':
				$this->ctype = $u . 'long';
				break;
			case 'real':
				$this->ctype = 'double';
			case 'double':
			case 'float':
				break;
			case 'decimal':
				$this->ctype = 'longdouble';
				break;
			case 'date':
			case 'time':
				$this->ctype = 'char';
				$this->cfield = $this->name . '[11]';
				$this->size = 10;
				break;
			case 'datetime':
			case 'timestamp':
				$this->ctype = 'char';
				$this->cfield = $this->name . '[20]';
				$this->size = 19;
				break;
			case 'char':
			case 'binary':
				$this->ctype = 'char';
				if($this->size>1) {
					$this->cfield = $this->name . '[' . ($this->size + 1) . ']';
				} else {
					$this->size = 0;
				}
				break;
			case 'varchar':
			case 'varbinary':
				$this->ctype = 'char';
				$this->cfield = $this->name . '[' . ($this->size + 1) . ']';
				break;
			case 'tinytext':
			case 'tinyblob':
				$this->ctype = 'char';
				$this->cfield = $this->name . '[256]';
				$this->size = 255;
				break;
			case 'text':
			case 'blob':
				$this->ctype = 'char';
				$this->cfield = $this->name . '[65536]';
				$this->size = 65535;
				break;
			case 'mediumtext':
			case 'mediumblob':
				$this->ctype = 'char';
				$this->cfield = '*' . $this->name;
				$this->size = 16777215;
				break;
			case 'longtext':
			case 'longblob':
				$this->ctype = 'char';
				$this->cfield = '*' . $this->name;
				$this->size = 4294967295;
				break;
			case 'enum':
				$this->ctype = 'char';
				$this->cfield = $this->name . '[1025]';
				$this->size = 1024;
				break;
			case 'set':
				$this->ctype = 'char';
				$this->cfield = $this->name . '[65536]';
				$this->size = 65535;
				break;
			default:
				$this->ctype = 'char';
				$this->cfield = '*' . $this->name;
				$this->size = 0;
				break;
		}
	}
	
	public static function getTableColumns($table) {
		$sql = 'SHOW FULL COLUMNS FROM `' . $table . '`';
		$list = findAll($sql);
		
		$columns = array();
		foreach($list as $field) {
			$columns[] = new self($field);
		}
		return $columns;
	}
}
