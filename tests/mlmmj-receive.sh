#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	basics

basics_body()
{

	mlmmjreceive=$(command -v mlmmj-receive)
	helptxt="Usage: $mlmmjreceive -L /path/to/listdir
       [-s sender@example.org] [-e extension] [-h] [-V] [-P] [-F]
 -h: This help
 -F: Don't fork in the background
 -L: Full path to list directory
 -s: Specify sender address
 -e: The foo part of user+foo@example.org
 -P: Don't execute mlmmj-process
 -V: Print version
"
	init_ml ml

	atf_check -s exit:1 -e "inline:mlmmj-receive: You have to specify -L\n$mlmmjreceive -h for help\n" $mlmmjreceive
	atf_check -s exit:0 -o "match:mlmmj-receive version .*" $mlmmjreceive -V
	atf_check -s exit:0 -o "inline:$helptxt" $mlmmjreceive -h
	echo test@mlmmjtest > ml/control/listaddress
	rm -rf ml/incoming
	atf_check -s exit:1 -e inline:"mlmmj-receive: Cannot open(ml/incoming): No such file or directory\n" $mlmmjreceive -L ml -e bla -s meh -P <<EOF
EOF
	mkdir ml/incoming
	atf_check -s exit:0 $mlmmjreceive -L ml -e bla -s meh -P <<EOF
EOF
	rm -f ml/incoming/*
	atf_check -s exit:0 $mlmmjreceive -L ml -P <<EOF
$(printf "test\nplop\n")
EOF
	file=$(find ml/incoming -type f)
	atf_check -o "inline:test\nplop\n" cat $file
	rm $file
	atf_check -s exit:0 $mlmmjreceive -L ml -F <<EOF
$(printf "test\nplop\n")
EOF
	files=$(find ml/queue -type f)
	set -- $files
	if [ $# -ne 1 ]; then
		atf_fail "Extra files left '$@'"
	fi
	case $files in
	*/discarded/*) ;;
	*) atf_fail "Extra files left" ;;
	esac
	rm $files
	atf_check -s exit:0 $mlmmjreceive -L ml <<EOF
$(printf "test\nplop\n")
EOF
	sleep .2
	files=$(find ml/queue -type f)
	set -- $files
	if [ $# -ne 1 ]; then
		atf_fail "Extra files left '$@'"
	fi
	case $files in
	*/discarded/*) ;;
	*) atf_fail "Extra files left" ;;
	esac
}
