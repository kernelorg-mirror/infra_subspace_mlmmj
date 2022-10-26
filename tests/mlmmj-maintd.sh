#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	nolongerbouncing \
	discarded \
	basics

nolongerbouncing_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	mkdir -p lists/ml/bounce

	echo test@mlmmjtest > lists/ml/control/listaddress
	f1=lists/ml/bounce/msg1
	f2=lists/ml/bounce/msg2
	now=$(date +%s)
	now=$((now - 2))
	echo "12" > ${f1}-probe
	touch ${f1}
	touch ${f1}.lastmsg
	echo "$now" > ${f2}-probe
	touch ${f2}
	touch ${f2}.lastmsg
	atf_check -s exit:0 -o empty -e ignore $mlmmjmaintd -L lists/ml -F
	atf_check -s exit:1 -o empty -e empty test -f $f1
	atf_check -s exit:1 -o empty -e empty test -f $f1-probe
	atf_check -s exit:1 -o empty -e empty test -f $f1.lastmsg
	atf_check -s exit:0 -o empty -e empty test -f $f2
	atf_check -s exit:0 -o empty -e empty test -f $f2-probe
	atf_check -s exit:0 -o empty -e empty test -f $f2.lastmsg
}

discarded_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml

	echo test@mlmmjtest > lists/ml/control/listaddress
	mkdir -p lists/ml/queue/discarded
	f1=lists/ml/queue/discarded/veryold
	f2=lists/ml/queue/discarded/notsoold
	atf_check touch -t "197001020000"  $f1
	atf_check touch $f2
	atf_check -s exit:0 -o empty -e ignore $mlmmjmaintd -L lists/ml -F
	atf_check -s exit:1 -o empty -e empty test -f $f1
	atf_check -s exit:0 -o empty -e empty test -f $f2
}

basics_body()
{

	mlmmjmaintd=$(command -v mlmmj-maintd)
	helptxt="Usage: /usr/home/bapt/dev/mlmmj/tests/../src/mlmmj-maintd [-L | -d] /path/to/dir [-F]
 -d: Full path to directory with listdirs
     Use this to run maintenance on all list directories
     in that directory.
 -L: Full path to one list directory
 -F: Don't fork, performing one maintenance run only.
     This option should be used when one wants to
     avoid running another daemon, and use e.g. cron to control it instead.
"
	mkdir lists
	init_ml lists/ml

	atf_check -s exit:1 -e "inline:mlmmj-maintd: You have to specify -d or -L\n$mlmmjmaintd -h for help\n" $mlmmjmaintd
	atf_check -s exit:1 -e "inline:mlmmj-maintd: You have to specify either -d or -L\n$mlmmjmaintd -h for help\n" $mlmmjmaintd -L lists/ml -d .
	atf_check -s exit:0 -o "inline:mlmmj-maintd version 1.3.0\n" $mlmmjmaintd -V
	atf_check -s exit:0 -o "inline:$helptxt" $mlmmjmaintd -h
	echo test@mlmmjtest > lists/ml/control/listaddress
	atf_check -s exit:0 -o empty -e ignore $mlmmjmaintd -L lists/ml -F
	output="Starting maintenance run at

clean_moderation
clean_discarded
clean_subconf
clean_unsubconf
resend_queue
resend_requeue
clean_nolongerbouncing
unsub_bouncers
probe_bouncers
run_digests
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	rm lists/ml/*.log
	atf_check -s exit:0 -e ignore $mlmmjmaintd -d lists -F
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
}
