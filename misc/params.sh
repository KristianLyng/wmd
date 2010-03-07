#!/bin/sh

awk < params '
BEGIN {
	num=0;
	name="";
	type="";
	desc="";
	range="";
	default="";
	state=0;
	list[0] = "";
}
/^end description$/ {
	list[num] = toupper(name);
	if (num++ != 0)
		print ","
	state=0;
	if (type == "bool") {
		type = "BOOL";
		field = "b";
		min = 0;
		max = 1;
		if (default == "")
			default=0;
	}


	printf("PD(%s, %s, %s,	%s, %s, %s, \n\"%s\")",
		name, type, default, field, min, max, desc);

}
{
	if(state == 10)
		if (desc == "")
			desc = $0;
		else
			desc = desc "\n" $0;
}
/^name:/ {
	if (state==10)
		continue;
	if ($2 == "")
		exit 1;
	name=$2;
}
/^type:/ {
	if ($2 == "bool")
		type="bool";
	else
		exit 2;
}
/^start description$/ {
	state=10;
}
END {
	print
	for (i = 0; i < num; i++)
		printf("P_%s = %d,\n",list[i], i);
}
'

