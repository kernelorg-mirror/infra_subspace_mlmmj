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
	addr_in_to_cc

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
	atf_check -s exit:1 -e inline:"mlmmj-process: Cannot open(plop): No such file or directory\n" $mlmmjprocess -L plop -m meh
}

basic_3_body()
{
	mkdir plop
	atf_check -s exit:1 -e inline:"mlmmj-process: Cannot open(plop/control): No such file or directory\n" $mlmmjprocess -L plop -m meh
}

basic_4_body()
{
	mkdir -p plop/control
	echo "test@test" > plop/control/listaddress
	atf_check -s exit:1 $mlmmjprocess -L plop -m meh
}

basic_5_body()
{
	mkdir -p plop/control
	mkdir -p plop/queue
	echo "test@test" > plop/control/listaddress
	atf_check -s exit:1 $mlmmjprocess -L plop -m meh
}

addr_in_to_cc_body()
{
	init_ml list
cat > mailfile <<EOF
Return-Path: sender@yeah
From: sender@yeah
Bcc: list@test
EOF
	echo "list@test" > list/control/listaddress
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	atf_check -s exit:0 $mlmmjprocess -L list -m mailfile
	rm list/queue/*.reciptto
	rm list/queue/*.mailfrom
	rmdir list/queue/discarded
	atf_check -o match:".*The message from <sender@yeah> with subject \"\" was unable to be delivered.*" cat list/queue/*
}
