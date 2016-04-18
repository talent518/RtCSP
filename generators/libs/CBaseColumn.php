<?php

abstract class CBaseColumn {
	public $name;
	public $type;
	public $size;
	public $option;
	public $default;
	public $isNull;
	
	public $comment;
	
	public $isUnsigned;
	
	public $ctype;
	public $cfield;
	
	abstract public static function getTableColumns($table);
}
