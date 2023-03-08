#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	nolongerbouncing \
	discarded \
	digests_0 \
	digests_1 \
	digests_2 \
	digests_3 \
	digests_4 \
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
	basics_0 \
	basics_1 \
	basics_2 \
	basics_3 \
	basics_4 \
	basics_5 \
	basics_6 \
	basics_7 \
	basics_8 \
	basics_9 \
	basics_10 \
	basics_11 \
	resend_requeue

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

basics_0_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml

	helptxt="Usage: $mlmmjmaintd [-L | -d] /path/to/dir [-F]
 -d: Full path to directory with listdirs
     Use this to run maintenance on all list directories
     in that directory.
 -L: Full path to one list directory
 -F: Don't fork, performing one maintenance run only.
     This option should be used when one wants to
     avoid running another daemon, and use e.g. cron to control it instead.
"

	atf_check -s exit:1 -e "inline:mlmmj-maintd: You have to specify -d or -L\n$mlmmjmaintd -h for help\n" $mlmmjmaintd
	atf_check -s exit:1 -e "inline:mlmmj-maintd: You have to specify either -d or -L\n$mlmmjmaintd -h for help\n" $mlmmjmaintd -L lists/ml -d .
	atf_check -s exit:0 -o "match:mlmmj-maintd version" $mlmmjmaintd -V
	atf_check -s exit:0 -o "inline:$helptxt" $mlmmjmaintd -h
}

basics_1_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

	atf_check -s exit:1 -e match:"Could not open the directory containing mailing lists.*: No such file or directory" $mlmmjmaintd -F -d nonexisting
}

basics_2_body()
{
	test $(id -u) = 0 && atf_skip "Can only be run as non-root"
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

	chmod -w lists/ml
	atf_check -s exit:0 -e match:"Could not open maintdlog.* Permission denied" $mlmmjmaintd -F -L lists/ml
}

basics_3_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

basics_4_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
	atf_check -s exit:0 $mlmmjmaintd -d lists -F
	atf_check -o "inline:$output" sed -e "s/at .*/at/" lists/ml/mlmmj-maintd.lastrun.log
}

basics_5_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

basics_6_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

basics_7_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

basics_8_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

basics_9_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

basics_10_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

basics_11_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

digests_0_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

digests_1_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

digests_2_body()
{
	test $(id -u) = 0 && atf_skip "Can only be run as non-root"
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

digests_3_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
}

digests_4_body()
{
	test $(id -u) = 0 && atf_skip "Can only be run as non-root"
	mlmmjmaintd=$(command -v mlmmj-maintd)
	mkdir lists
	init_ml lists/ml
	echo test@mlmmjtest > lists/ml/control/listaddress

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
	init_ml list
	echo "test@test" > list/control/listaddress

	# empty file, it should be ignored
	touch list/bounce/plop
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop
}

unsub_bouncers_1_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	init_ml list
	echo test@mlmmjtest > list/control/listaddress

	# ignore probe files
	touch list/bounce/plop-probe
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop-probe
}

unsub_bouncers_2_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	init_ml list
	echo test@mlmmjtest > list/control/listaddress

	# ignore lastmsg files
	touch list/bounce/plop.lastmsg
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop.lastmsg
}

unsub_bouncers_3_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	init_ml list
	echo test@mlmmjtest > list/control/listaddress

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
	init_ml list
	echo "test@test" > list/control/listaddress
	# filename should contain a = to be properly treated
	echo "a:1234 # plop" > list/bounce/plop
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop
}

unsub_bouncers_5_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	init_ml list
	echo "test@test" > list/control/listaddress

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
	init_ml list
	echo "test@test" > list/control/listaddress

	test $(id -u) = 0 && atf_skip "Can only be run as non-root"

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
	echo "test@test" > list/control/listaddress

	# keep if recent enough
	echo "a:$(date +%s) # plop" > list/bounce/plop=meh
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:0 test -f list/bounce/plop=meh
}

unsub_bouncers_8_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	init_ml list
	echo "test@test" > list/control/listaddress

	echo "a:1234 # plop" > list/bounce/plop=meh
	atf_check -s exit:0 $mlmmjmaintd -L list -F
	atf_check -s exit:1 test -f list/bounce/plop=meh
}

unsub_bouncers_9_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	init_ml list
	echo "test@test" > list/control/listaddress

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
	init_ml list
	echo "test@test" > list/control/listaddress

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

resend_requeue_body()
{
	mlmmjmaintd=$(command -v mlmmj-maintd)
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	init_ml list
	echo "test@test" > list/control/listaddress
	echo "25678" > list/control/smtpport
	echo "heloname" > list/control/smtphelo
	# Not a directory
cat >> list/requeue/1 <<EOF
user1@test1
user2@test2
user3@test3
EOF
	atf_check $mlmmjmaintd -L list -F

cat >> expect1.txt <<EOF
Starting maintenance run at

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
EOF

	atf_check -o file:expect1.txt sed -e "s/at .*/at/" list/mlmmj-maintd.lastrun.log
	# no archive and don't remove empty dir
	rm list/requeue/1
	mkdir list/requeue/1
	mkdir list/requeue/2
	touch -t 000101010101 list/requeue/2
	mkdir list/requeue/3
	cat >> list/requeue/1/subscribers <<EOF
user1@test1
user2@test2
user3@test3
EOF
	atf_check $mlmmjmaintd -L list -F
	atf_check -o file:expect1.txt sed -e "s/at .*/at/" list/mlmmj-maintd.lastrun.log
	test -d list/requeue/3 || atf_fail "list/requeue/3 should not have been removed"
	if test -d list/requeue/2; then
		atf_fail "list/requeue/2 should have been removed"
	fi
	atf_check -s exit:1 test -f mail-1.txt
cat > list/requeue/3/mailfile <<EOF
From: plop
To: bla
Subject: test

body
EOF
	# no subscribers: delete the mailfile
	atf_check $mlmmjmaintd -L list -F
	atf_check -o file:expect1.txt sed -e "s/at .*/at/" list/mlmmj-maintd.lastrun.log
	if test -f list/requeue/3/mailfile; then
		atf_fail "list/requeue/3/mailfile should have been removed"
	fi

cat > list/requeue/3/mailfile <<EOF
From: plop
To: bla
Subject: test

body
EOF
	cat >> list/requeue/3/subscribers <<EOF
user1@test1
EOF
	# now the mail should be sent
	atf_check $mlmmjmaintd -L list -F
	cat > expect1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-3-user1=test1@test>
RCPT TO:<user1@test1>
DATA
From: plop
To: bla
Subject: test

body

.
QUIT
EOF
	atf_check -o file:expect1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confsub-.*@mlmmjtest/confsub-@mlmmjtest/g" mail-1.txt
	atf_check -s exit:1 test -f mail-2.txt
	atf_check -s exit:1 test -f list/requeue/3/mailfile
cat > list/archive/1 <<EOF
From: plop
To: bla
Subject: test

body
EOF
	# now with archive
	cat > expect2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user1=test1@test>
RCPT TO:<user1@test1>
DATA
From: plop
To: bla
Subject: test

body

.
MAIL FROM:<test+bounces-1-user2=test2@test>
RCPT TO:<user2@test2>
DATA
From: plop
To: bla
Subject: test

body

.
MAIL FROM:<test+bounces-1-user3=test3@test>
RCPT TO:<user3@test3>
DATA
From: plop
To: bla
Subject: test

body

.
QUIT
EOF
	atf_check $mlmmjmaintd -L list -F
	atf_check -o file:expect2.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confsub-.*@mlmmjtest/confsub-@mlmmjtest/g" mail-2.txt
	atf_check -s exit:1 test -f mail-4.txt
	atf_check -s exit:0 test -f list/archive/1
}
