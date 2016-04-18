#include "<?php echo $table;?>.h"

static serialize_format_t <?php echo $table?>_formats[] = {
<?php foreach($columns as $column):
	echo "\t";
	if($column->name === $column->cfield):
		?>SFT_<?php echo $column->ctype === 'longdouble' ? 'LONG_DOUBLE' : strtoupper($column->ctype);?>(table_<?php echo $table;?>_t, <?php echo $column->name;?>, "table_<?php echo $table;?>_t of <?php echo $column->name;?>"), // <?php echo $column->comment, PHP_EOL;
	else:
		?>SFT_STR<?php echo $column->cfield{0} === '*' ? null : 'LEN';?>(table_<?php echo $table;?>_t, <?php echo $column->name;?>, <?php echo $column->name;?>_length<?php echo $column->cfield{0} !== '*' ? ", {$column->size}" : null;?>, "table_<?php echo $table;?>_t of <?php echo $column->name;?>"), // <?php echo $column->comment, PHP_EOL;
	endif;
endforeach;?>
	SFT_END
};

static serialize_object_t <?php echo $table?>_object = {<?php echo $table?>_formats, NULL};

serialize_format_t <?php echo $table?>_format = SFT_OBJECT(null_t, null, &<?php echo $table?>_object, "object_t of objects_format");

