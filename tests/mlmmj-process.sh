#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	usage \
	basic_0 \
	basic_1 \
	basic_2 \
	basic_3 \
	basic_4 \
	basic_5 \

mlmmjprocess=$(command -v mlmmj-process)
usage_body()
{
	output="Usage: $mlmmjprocess -L /path/to/list
       -m /path/to/mail [-h] [-P] [-V]
 -h: This help
 -L: Full path to list directory
 -m: Full path to mail file
 -P: Don't execute mlmmj-send
 -V: Print version
"
	atf_check -o inline:"$output" $mlmmjprocess -h
}

basic_0_body()
{
	output="mlmmj-process: You have to specify -L and -m
$mlmmjprocess -h for help
"
	atf_check -s exit:1 -e inline:"$output" $mlmmjprocess
}

basic_1_body()
{
	output="mlmmj-process: You have to specify -L and -m
$mlmmjprocess -h for help
"
	atf_check -s exit:1 -e inline:"$output" $mlmmjprocess -L plop
}

basic_2_body()
{
	atf_check -s exit:1 $mlmmjprocess -L plop -m meh
}

basic_3_body()
{
	mkdir plop
	atf_check -s exit:1 -e inline:"mlmmj-process: Cannot open(plop/control): No such file or directory\n" $mlmmjprocess -L plop -m meh
}

basic_4_body()
{
	mkdir -p plop/control
	atf_check -s exit:1 $mlmmjprocess -L plop -m meh
}

basic_5_body()
{
	mkdir -p plop/control
	mkdir -p plop/queue
	atf_check -s exit:1 $mlmmjprocess -L plop -m meh
}
