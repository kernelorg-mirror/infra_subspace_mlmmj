#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	nolongerbouncing \
	discarded \
	digests \
	unsub_bouncers_0 \
	unsub_bouncers_1 \
	unsub_bouncers_2 \
	unsub_bouncers_3 \
	unsub_bouncers_4 \
	unsub_bouncers_5 \
	unsub_bouncers_6 \
	unsub_bouncers_7 \
	unsub_bouncers_8 \
	unsub_bouncers_9 \
	unsub_bouncers_10 \
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
	atf_check -s exit:0 -o empty $mlmmjmaintd -L lists/ml -F
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
	atf_check -s exit:0 -o empty $mlmmjmaintd -L lists/ml -F
	atf_check -s exit:1 -o empty -e empty test -f $f1
	atf_check -s exit:0 -o empty -e empty test -f $f2
}

basics_body()
{

	mlmmjmaintd=$(command -v mlmmj-maintd)
	helptxt="Usage: $mlmmjmaintd [-L | -d] /path/to/dir [-F]
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
	atf_check -s exit:0 -o "match:mlmmj-maintd version" $mlmmjmaintd -V
	atf_check -s exit:0 -o "inline:$helptxt" $mlmmjmaintd -h
	echo test@mlmmjtest > lists/ml/control/listaddress
	atf_check -s exit:1 -e match:"Could not open the directory containing mailing lists.*: No such file or directory" $mlmmjmaintd -F -d nonexisting
	chmod -w lists/ml
	atf_check -s exit:0 -e match:"Could not open maintdlog.* Permission denied" $mlmmjmaintd -F -L lists/ml
	chmod +w lists/ml
	atf_check -s exit:0 $mlmmjmaintd -L lists/ml -F
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
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	rm lists/ml/*.log
	atf_check -s exit:0 $mlmmjmaintd -d lists -F
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	rmdir lists/ml/queue/discarded
	atf_check -s exit:0 -e match:"An error occured while cleaning discarded mails, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L lists/ml -F
	output="Starting maintenance run at

clean_moderation
clean_discarded
 - Could not open 'queue/discarded': No such file or directory
clean_subconf
clean_unsubconf
resend_queue
resend_requeue
clean_nolongerbouncing
unsub_bouncers
probe_bouncers
run_digests
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	mkdir lists/ml/queue/discarded

	rm -rf lists/ml/queue
	atf_check -s exit:0 -e match:"An error occured while cleaning discarded mails, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L lists/ml -F
	output="Starting maintenance run at

clean_moderation
clean_discarded
 - Could not open 'queue/discarded': No such file or directory
clean_subconf
clean_unsubconf
resend_queue
 - Could not open 'queue': No such file or directory
resend_requeue
clean_nolongerbouncing
unsub_bouncers
probe_bouncers
run_digests
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	mkdir -p lists/ml/queue/discarded

	rm -rf lists/ml/requeue
	atf_check -s exit:0 -e match:"An error occured while resending requeued mails, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L lists/ml -F
	output="Starting maintenance run at

clean_moderation
clean_discarded
clean_subconf
clean_unsubconf
resend_queue
resend_requeue
 - Could not open 'requeue': No such file or directory
clean_nolongerbouncing
unsub_bouncers
probe_bouncers
run_digests
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	mkdir -p lists/ml/requeue

	rm -rf lists/ml/bounce
	atf_check -s exit:0 -e match:"An error occured while cleaning no longer bouncing traces, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L lists/ml -F
	atf_check -s exit:0 -e match:"An error occured while unsubscribing bouncing emails, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L lists/ml -F
	atf_check -s exit:0 -e match:"An error occured while sending probes for bouncers, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L lists/ml -F
	output="Starting maintenance run at

clean_moderation
clean_discarded
clean_subconf
clean_unsubconf
resend_queue
resend_requeue
clean_nolongerbouncing
 - Could not open 'bounce': No such file or directory
unsub_bouncers
 - Could not open 'bounce': No such file or directory
probe_bouncers
 - Could not open 'bounce': No such file or directory
run_digests
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	mkdir -p lists/ml/bounce

	rm -rf lists/ml/subconf
	atf_check -s exit:0 -e match:"An error occured while cleaning subscribtion confirmations, mails, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L lists/ml -F
	output="Starting maintenance run at

clean_moderation
clean_discarded
clean_subconf
 - Could not open 'subconf': No such file or directory
clean_unsubconf
resend_queue
resend_requeue
clean_nolongerbouncing
unsub_bouncers
probe_bouncers
run_digests
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	mkdir -p lists/ml/subconf

	rm -rf lists/ml/unsubconf
	atf_check -s exit:0 -e match:"An error occured while cleaning unsubscribtion confirmations, mails, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L lists/ml -F
	output="Starting maintenance run at

clean_moderation
clean_discarded
clean_subconf
clean_unsubconf
 - Could not open 'unsubconf': No such file or directory
resend_queue
resend_requeue
clean_nolongerbouncing
unsub_bouncers
probe_bouncers
run_digests
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	mkdir -p lists/ml/unsubconf

	rm -rf lists/ml/moderation
	atf_check -s exit:0 -e match:"An error occured while cleaning moderation, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L lists/ml -F
	output="Starting maintenance run at

clean_moderation
 - Could not open 'moderation': No such file or directory
clean_discarded
clean_subconf
clean_unsubconf
resend_queue
resend_requeue
clean_nolongerbouncing
unsub_bouncers
probe_bouncers
run_digests
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	mkdir -p lists/ml/moderation
}

digests_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml

	touch lists/ml/control/noarchive
	atf_check -s exit:0 $mlmmjmaintd -L lists/ml -F
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
 - noarchive tunable: skipping digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log

	rm lists/ml/control/noarchive
	touch lists/ml/index
	atf_check -s exit:0 $mlmmjmaintd -L lists/ml -F
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

	chmod -r lists/ml/index
	atf_check -s exit:0 $mlmmjmaintd -L lists/ml -F
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
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	chmod +r lists/ml/index
	echo "-1" > lists/ml/index
	atf_check -s exit:0 -e match:"An error occured while running digests, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L lists/ml -F
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
 - Invalid index content '-1': too small
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
	echo "0" > lists/ml/index

	touch lists/ml/lastdigest
	chmod -w lists/ml/lastdigest
	atf_check -s exit:0 -e match:"An error occured while running digests, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L lists/ml -F
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
 - Could not open or create 'lastdigest': Permission denied
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
}

unsub_bouncers_0_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml list

	# empty file, it should be ignored
	touch list/bounce/plop
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop
}

unsub_bouncers_1_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml list

	# ignore probe files
	touch list/bounce/plop-probe
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop-probe
}

unsub_bouncers_2_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml list

	# ignore lastmsg files
	touch list/bounce/plop.lastmsg
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop.lastmsg
}

unsub_bouncers_3_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml list

	# ignore files with a probe out already
	echo "a:1234 # plop" > list/bounce/plop
	touch list/bounce/plop-probe
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop
	atf_check -s exit:0 test -f list/bounce/plop-probe
}

unsub_bouncers_4_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml list

	# filename should contain a = to be properly treated
	echo "a:1234 # plop" > list/bounce/plop
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop
}

unsub_bouncers_5_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml list

	# Log if invalid format
	echo "a:a #plop" > list/bounce/plop=meh
	atf_check -s exit:0 -e match:"An error occured while unsubscribing bouncing emails, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop=meh
	output="Starting maintenance run at

clean_moderation
clean_discarded
clean_subconf
clean_unsubconf
resend_queue
resend_requeue
clean_nolongerbouncing
unsub_bouncers
 - Error parsing the first line of bounce/plop=meh: invalid
probe_bouncers
run_digests
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" list/mlmmj-maintd.lastrun.log
}

unsub_bouncers_6_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml list

	# Log unreabable bouncefile
	echo "a:1234 #plop" > list/bounce/plop=meh
	chmod -r list/bounce/plop=meh
	atf_check -s exit:0 -e match:"An error occured while unsubscribing bouncing emails, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop=meh
	output="Starting maintenance run at

clean_moderation
clean_discarded
clean_subconf
clean_unsubconf
resend_queue
resend_requeue
clean_nolongerbouncing
unsub_bouncers
 - Could not open bounce/plop=meh: Permission denied
probe_bouncers
run_digests
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" list/mlmmj-maintd.lastrun.log
}

unsub_bouncers_7_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml list

	# keep if recent enough
	echo "a:$(date +%s) # plop" > list/bounce/plop=meh
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop=meh
}

unsub_bouncers_8_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml list

	echo "a:1234 # plop" > list/bounce/plop=meh
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:1 test -f list/bounce/plop=meh
}

unsub_bouncers_9_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml list

	# last message properly cleaned out
	echo "a:1234 # plop" > list/bounce/plop=meh
	echo plop > list/bounce/plop=meh.lastmsg
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:1 test -f list/bounce/plop=meh
	atf_check -s exit:1 test -f list/bounce/plop=meh.lastmsg
}

unsub_bouncers_10_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml list

	# has been unsubscribe from regular
	echo "a:1234 # plop" > list/bounce/plop=meh
	echo "plop@meh" > list/subscribers.d/p
	echo "plop@meh" > list/digesters.d/p
	echo "plop@meh" > list/nomailsubs.d/p
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:1 test -f list/bounce/plop=meh
	atf_check -s exit:1 test -f list/subscribers.d/p
	atf_check -s exit:1 test -f list/digesters.d/p
	atf_check -s exit:1 test -f list/nomailsubs.d/p

	echo "a:1234 # plop" > list/bounce/plop=meh
	echo "plop@meh" > list/digesters.d/p
	rm -rf list/nomailsubs.d
	atf_check -s exit:0 -e match:"An error occured while unsubscribing bouncing emails, see mlmmj-maintd.lastrun.log" $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop=meh
	atf_check -s exit:1 test -f list/digesters.d/p
	output="Starting maintenance run at

clean_moderation
clean_discarded
clean_subconf
clean_unsubconf
resend_queue
resend_requeue
clean_nolongerbouncing
unsub_bouncers
 - Some errors during unsubscription of plop@meh
probe_bouncers
run_digests
 - No readable index file: no digest
"
	atf_check -o "inline:$output" sed -e "s/at .*/at/" list/mlmmj-maintd.lastrun.log
}
