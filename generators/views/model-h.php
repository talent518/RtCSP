#ifndef HAVE_<?php echo strtoupper($table);?>_H
#define HAVE_<?php echo strtoupper($table);?>_H
#include "serialize.h"

typedef struct {
<?php
foreach($columns as $column):
	echo "\t", $column->ctype;?>_t <?php echo $column->cfield;?>; // <?php echo $column->comment, PHP_EOL;
	if($column->name !== $column->cfield):
		echo "\tsize_t {$column->name}_length;\n";
	endif;
endforeach;
?>} table_<?php echo $table;?>_t;

extern serialize_format_t <?php echo $table?>_format;

#endif
