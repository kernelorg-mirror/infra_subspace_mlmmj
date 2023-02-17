#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

kill_fakesmtp()
{
	pid=$(cat fakesmtp.pid)
	kill -15 $pid
}

tests_init \
	add_basics \
	add_normal \
	add_normal_welcome \
	add_normal_welcome_confirm \
	add_normal_welcome_moderate \
	add_normal_owner \
	add_nomail \
	add_digest \
	add_normal_then_nomail \
	add_both \
	add_both_with_digest \
	add_both_with_normal \
	add_normal_with_both \
	add_digest_with_both \
	add_nomail_with_both \
	remove_basics \
	remove_normal \
	remove_normal_confirm \
	remove_normal_owner \
	remove_nomail \
	remove_digest

add_normal_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	echo test@mlmmjtest > ml/control/listaddress
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org
	atf_check -o inline:"john@doe.org\n" cat ml/subscribers.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -a jane@doe.org
	atf_check -o inline:"john@doe.org\njane@doe.org\n" cat ml/subscribers.d/j
}

add_normal_welcome_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	echo test@mlmmjtest > ml/control/listaddress
	echo "25678" > ml/control/smtpport
	echo "heloname" > ml/control/smtphelo
	rmdir ml/text
	ln -s ${srcdir}/../listtexts/en ml/text
	atf_check -s exit:0 $mlmmjsub -L ml -c -a john@doe.org
	cat > expected.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<john@doe.org>
DATA
Subject: =?utf-8?q?Welcome_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: john@doe.org
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

An administrator has subscribed you to the normal version of the list.

The email address you are subscribed with is <john@doe.org>.

If you ever wish to unsubscribe, send a message to
<test+unsubscribe@mlmmjtest> using this email address. The subject and the
body of the message can be anything. You will then receive confirmation or
further instructions.

For other information and help about this list, send a message to
<test+help@mlmmjtest>.


.
QUIT
EOF

	atf_check -o file:expected.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-1.txt
	atf_check -o inline:"john@doe.org\n" cat ml/subscribers.d/j
	atf_check -s exit:1 test -f mail-2.txt
}

add_normal_welcome_confirm_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	echo test@mlmmjtest > ml/control/listaddress
	echo "25678" > ml/control/smtpport
	echo "heloname" > ml/control/smtphelo
	rmdir ml/text
	ln -s ${srcdir}/../listtexts/en ml/text
	atf_check -s exit:0 $mlmmjsub -L ml -c -C -a john@doe.org
	cat > expected.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confsub-@mlmmjtest>
RCPT TO:<john@doe.org>
DATA
Subject: =?utf-8?q?Confirm_subscription_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: john@doe.org
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confsub-@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

An administrator has requested that your email address <john@doe.org> be
added to the list. This means every time a post is sent to the list, you
will receive a copy of it.

To confirm you want to do this, please send a message to
<test+confsub-@mlmmjtest> which can usually be done simply
by replying to this message. The subject and the body of the message can be
anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF

	atf_check -o file:expected.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confsub-.*@mlmmjtest/confsub-@mlmmjtest/g" mail-1.txt
	atf_check -s exit:1 test -f ml/subscribers.d/j
	atf_check -o match:"^ml/subconf/.*" find ml/subconf/ -type f
	atf_check -s exit:1 test -f mail-2.txt
}

add_normal_welcome_moderate_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	echo test@mlmmjtest > ml/control/listaddress
	echo "25678" > ml/control/smtpport
	echo "heloname" > ml/control/smtphelo
	rmdir ml/text
	ln -s ${srcdir}/../listtexts/en ml/text
	cat > ml/control/submod <<EOF
modo1@test
modo2@test
EOF
	atf_check -s exit:0 $mlmmjsub -L ml -c -a john@doe.org
	cat > expected.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<john@doe.org>
DATA
Subject: =?utf-8?q?Awaiting_permission_to_join_test=40mlmmjtest?=
From: test+owner@mlmmjtest
To: john@doe.org
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Your request to join the list has been received. However, the gatekeepers
are being asked to review it before permitting you to join.


.
QUIT
EOF

	cat > expected-modos.txt <<EOF
EHLO heloname
MAIL FROM:<test+owner@mlmmjtest>
RCPT TO:<modo1@test>
DATA
Subject: =?utf-8?q?Subscription_request_for_test=40mlmmjtest:_john=40doe.org?=
From: test+owner@mlmmjtest
To: test-moderators@mlmmjtest
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+permit-@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

There has been a request from <john@doe.org> to join the normal version of
the list.

To permit this, please send a message to
<test+permit-@mlmmjtest> which can usually be done simply
by replying to this message.

If you do not want to do this, either send a message to
<test+obstruct-@mlmmjtest> or simply ignore this message.

The following gatekeepers have received this mail:
- modo1@test
- modo2@test


.
MAIL FROM:<test+owner@mlmmjtest>
RCPT TO:<modo2@test>
DATA
Subject: =?utf-8?q?Subscription_request_for_test=40mlmmjtest:_john=40doe.org?=
From: test+owner@mlmmjtest
To: test-moderators@mlmmjtest
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+permit-@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

There has been a request from <john@doe.org> to join the normal version of
the list.

To permit this, please send a message to
<test+permit-@mlmmjtest> which can usually be done simply
by replying to this message.

If you do not want to do this, either send a message to
<test+obstruct-@mlmmjtest> or simply ignore this message.

The following gatekeepers have received this mail:
- modo1@test
- modo2@test


.
QUIT
EOF
	atf_check -o file:expected.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-1.txt
	atf_check -s exit:1 test -f ml/subscribers.d/j
	atf_check -o file:expected-modos.txt sed -e "/^Message-ID:/d; /^Date:/d; s/permit-.*@mlmmjtest/permit-@mlmmjtest/g; s/obstruct-.*@mlmmjtest/obstruct-@mlmmjtest/g" mail-2.txt
	atf_check -s exit:1 test -f mail-3.txt
	atf_check -o match:"^ml/moderation/subscribe.*" find ml/moderation/ -type f
	atf_check -o inline:"john@doe.org\nSUB_NORMAL\n" cat ml/moderation/subscribe*
	file=$(ls ml/moderation/subscribe*)
	atf_check ${mlmmjsub} -L ml -m ${file##*subscribe} -c
	atf_check -o inline:"john@doe.org\n" cat ml/subscribers.d/j
	atf_check -s exit:1 test -f $file
}

add_normal_then_nomail_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	echo test@mlmmjtest > ml/control/listaddress
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org
	atf_check -o inline:"john@doe.org\n" cat ml/subscribers.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -n -a john@doe.org
	atf_check -o inline:"john@doe.org\n" cat ml/nomailsubs.d/j
	if [ -f ml/subscribers.d/j ]; then
		atf_fail "John Doe still subscribed"
	fi
}

add_normal_owner_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	echo test@mlmmjtest > ml/control/listaddress
	atf_check -s exit:0 $mlmmjsub -L ml -q -a john@doe.org
	atf_check -o inline:"john@doe.org\n" cat ml/subscribers.d/j
}

add_digest_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	echo test@mlmmjtest > ml/control/listaddress
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org -d
	atf_check -o inline:"john@doe.org\n" cat ml/digesters.d/j
}

add_nomail_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	echo test@mlmmjtest > ml/control/listaddress
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org -n
	atf_check -o inline:"john@doe.org\n" cat ml/nomailsubs.d/j
}

add_both_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	echo test@mlmmjtest > ml/control/listaddress
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org -b
	atf_check -o inline:"john@doe.org\n" cat ml/subscribers.d/j
	atf_check -o inline:"john@doe.org\n" cat ml/digesters.d/j
}

add_both_with_digest_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	echo test@mlmmjtest > ml/control/listaddress
	echo "john@doe.org" > ml/digesters.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org -b
	atf_check -o inline:"john@doe.org\n" cat ml/subscribers.d/j
	atf_check -o inline:"john@doe.org\n" cat ml/digesters.d/j
}

add_both_with_normal_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	echo test@mlmmjtest > ml/control/listaddress
	echo "john@doe.org" > ml/subscribers.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org -b
	atf_check -o inline:"john@doe.org\n" cat ml/subscribers.d/j
	atf_check -o inline:"john@doe.org\n" cat ml/digesters.d/j
}

add_normal_with_both_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	echo test@mlmmjtest > ml/control/listaddress
	echo "john@doe.org" > ml/subscribers.d/j
	echo "john@doe.org" > ml/digesters.d/j
	echo "john@doe.org" > ml/nomailsubs.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org
	atf_check -s exit:1 test -f ml/digesters.d/j
	atf_check -o inline:"john@doe.org\n" cat ml/subscribers.d/j
	atf_check -s exit:1 test -f ml/nomail.d/j
}

add_digest_with_both_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	echo test@mlmmjtest > ml/control/listaddress
	echo "john@doe.org" > ml/subscribers.d/j
	echo "john@doe.org" > ml/digesters.d/j
	echo "john@doe.org" > ml/nomailsubs.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org -d
	atf_check -s exit:1 test -f ml/subscribers.d/j
	atf_check -o inline:"john@doe.org\n" cat ml/digesters.d/j
	atf_check -s exit:1 test -f ml/nomail.d/j
}

add_nomail_with_both_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-sub)
	echo test@mlmmjtest > ml/control/listaddress
	echo "john@doe.org" > ml/subscribers.d/j
	echo "john@doe.org" > ml/digesters.d/j
	echo "john@doe.org" > ml/nomailsubs.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org -n
	atf_check -s exit:1 test -f ml/subscribers.d/j
	atf_check -s exit:1 test -f ml/digesters.d/j
	atf_check -s exit:1 test -f ml/nomail.d/j
}

remove_normal_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-unsub)
	printf "To: test@mlmmjtest1\r\nSubject: test\n\nplop" > mail
	echo test@mlmmjtest > ml/control/listaddress
	echo "john@doe.org" > ml/subscribers.d/j
	echo "bob@doe.org" >> ml/subscribers.d/j
	echo "rebecca@doe.org" >> ml/subscribers.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org
	atf_check -o inline:"bob@doe.org\nrebecca@doe.org\n" cat ml/subscribers.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -a jane@doe.org -q -s
	atf_check -o inline:"bob@doe.org\nrebecca@doe.org\n" cat ml/subscribers.d/j
}

remove_normal_confirm_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-unsub)
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	echo test@mlmmjtest > ml/control/listaddress
	echo "john@doe.org" > ml/subscribers.d/j
	echo "bob@doe.org" >> ml/subscribers.d/j
	echo "rebecca@doe.org" >> ml/subscribers.d/j
	echo "25678" > ml/control/smtpport
	echo "heloname" > ml/control/smtphelo
	rmdir ml/text
	ln -s ${srcdir}/../listtexts/en ml/text
	atf_check -s exit:0 $mlmmjsub -L ml -c -a john@doe.org
	atf_check -o inline:"bob@doe.org\nrebecca@doe.org\n" cat ml/subscribers.d/j
	cat > expected.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<john@doe.org>
DATA
Subject: =?utf-8?q?Goodbye_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: john@doe.org
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

An administrator has removed you from the list.


.
QUIT
EOF
	atf_check -s exit:1 test -f mail-2.txt
	atf_check -o file:expected.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-1.txt
	touch ml/control/notifysub
	atf_check -s exit:0 $mlmmjsub -L ml -c -a john@doe.org
	atf_check -o inline:"bob@doe.org\nrebecca@doe.org\n" cat ml/subscribers.d/j
	cat > expected2.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<john@doe.org>
DATA
Subject: =?utf-8?q?Unable_to_unsubscribe_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: john@doe.org
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

You were unable to be unsubscribed from the list because you are not
subscribed.

If you are receiving messages, perhaps a different email address is
subscribed. To find out which address you are subscribed with, refer to the
message welcoming you to the list, or look at the envelope "Return-Path"
header of a message you receive from the list.


.
QUIT
EOF
	atf_check -o file:expected2.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-2.txt

	echo "john@doe.org" >> ml/subscribers.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -c -a john@doe.org
	atf_check -o inline:"bob@doe.org\nrebecca@doe.org\n" cat ml/subscribers.d/j
	cat > expected3.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<john@doe.org>
DATA
Subject: =?utf-8?q?Goodbye_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: john@doe.org
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

An administrator has removed you from the list.


.
QUIT
EOF
	atf_check -o file:expected3.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-3.txt
	cat > expected4.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<test+owner@mlmmjtest>
DATA
Subject: =?utf-8?q?Unsubscribed_from_test=40mlmmjtest:_john=40doe.org?=
From: test+owner@mlmmjtest
To: test+owner@mlmmjtest
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

The address <john@doe.org> has been unsubscribed from the list because an
administrator commanded it.


.
QUIT
EOF
	atf_check -o file:expected4.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-4.txt
	atf_check -s exit:1 test -f mail-5.txt
}

remove_normal_owner_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-unsub)
	printf "To: test@mlmmjtest1\r\nSubject: test\n\nplop" > mail
	echo 25678 > ml/control/smtpport
	echo test@mlmmjtest > ml/control/listaddress
	echo owner@mlmmjtest > ml/control/owner
	echo "john@doe.org" > ml/subscribers.d/j
	echo "bob@doe.org" >> ml/subscribers.d/j
	echo "rebecca@doe.org" >> ml/subscribers.d/j
	touch ml/control/notifysub
	atf_check -s exit:0 $mlmmjsub -L ml -q -a john@doe.org
	atf_check -o inline:"bob@doe.org\nrebecca@doe.org\n" cat ml/subscribers.d/j
}

remove_digest_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-unsub)
	printf "To: test@mlmmjtest1\r\nSubject: test\n\nplop" > mail
	echo test@mlmmjtest > ml/control/listaddress
	echo "john@doe.org" > ml/digesters.d/j
	echo "bob@doe.org" >> ml/digesters.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org -d
	atf_check -o inline:"bob@doe.org\n" cat ml/digesters.d/j
}

remove_nomail_body()
{
	init_ml ml
	mlmmjsub=$(command -v mlmmj-unsub)
	printf "To: test@mlmmjtest1\r\nSubject: test\n\nplop" > mail
	echo test@mlmmjtest > ml/control/listaddress
	echo "john@doe.org" > ml/nomailsubs.d/j
	echo "bob@doe.org" >> ml/nomailsubs.d/j
	echo "rebecca@doe.org" >> ml/nomailsubs.d/j
	atf_check -s exit:0 $mlmmjsub -L ml -a john@doe.org -n
	atf_check -o inline:"bob@doe.org\nrebecca@doe.org\n" cat ml/nomailsubs.d/j
}

add_basics_body()
{

	mlmmjsub=$(command -v mlmmj-sub)
	helptxt="Usage: $mlmmjsub -L /path/to/list {-a john@doe.org | -m str}
       [-c] [-C] [-f] [-h] [-L] [-d | -n] [-q] [-r | -R] [-s] [-U] [-V]
 -a: Email address to subscribe 
 -c: Send welcome mail (unless requesting confirmation)
 -C: Request mail confirmation (unless switching versions)
 -d: Subscribe to digest of list
 -f: Force subscription (do not moderate)
 -h: This help
 -L: Full path to list directory
 -m: moderation string
 -n: Subscribe to no mail version of list
 -q: Be quiet (don't notify owner about the subscription)
 -r: Behave as if request arrived via email (internal use)
 -R: Behave as if confirmation arrived via email (internal use)
 -s: Don't send a mail to subscriber if already subscribed
 -U: Don't switch to the user id of the listdir owner
 -V: Print version
To ensure a silent subscription, use -f -q -s"
	init_ml ml
	echo "test@mlmmjtest" > ml/control/listaddress
	atf_check -s exit:1 -e "inline:mlmmj-sub: No '@' in the provided email address 'plop'\n" $mlmmjsub -L ml -a plop -n
	atf_check -s exit:1 -e "inline:mlmmj-sub: You have to specify -a or -m\n$mlmmjsub -h for help\n" $mlmmjsub -L ml
	atf_check -s exit:1 -e "inline:mlmmj-sub: Specify at most one of -b, -d and -n\n$mlmmjsub -h for help\n" $mlmmjsub -L ml -a john@doe.org -b -d -n
	atf_check -s exit:1 -e "inline:mlmmj-sub: Cannot subscribe the list address to the list\n" $mlmmjsub -L ml -a test@mlmmjtest -b
	atf_check -s exit:0 -o "inline:${helptxt}\n" $mlmmjsub -h
	atf_check -s exit:0 -o "match:mlmmj-sub version" $mlmmjsub -V
	atf_check -s exit:1 -e "inline:mlmmj-sub: You have to specify -L\n$mlmmjsub -h for help\n" $mlmmjsub
}

remove_basics_body()
{

	mlmmjsub=$(command -v mlmmj-unsub)
	helptxt="Usage: $mlmmjsub -L /path/to/list -a john@doe.org
       [-c | -C] [-h] [-L] [-d | -n | -N] [-q] [-r | -R] [-s] [-V]
 -a: Email address to unsubscribe 
 -c: Send goodbye mail
 -C: Request mail confirmation
 -d: Unsubscribe from digest of list
 -h: This help
 -L: Full path to list directory
 -n: Unsubscribe from no mail version of list
 -N: Unsubscribe from normal version of list
 -q: Be quiet (don't notify owner about the subscription)
 -r: Behave as if request arrived via email (internal use)
 -R: Behave as if confirmation arrived via email (internal use)
 -s: Don't send a mail to the address if not subscribed
 -U: Don't switch to the user id of the listdir owner
 -V: Print version
To ensure a silent unsubscription, use -q -s"
	init_ml ml
	echo "test@mlmmjtest" > ml/control/listaddress
	atf_check -s exit:1 -e "inline:mlmmj-unsub: No '@' in the provided email address 'plop'\n" $mlmmjsub -L ml -a plop -n
	atf_check -s exit:1 -e "inline:mlmmj-unsub: You have to specify -L and -a\n$mlmmjsub -h for help\n" $mlmmjsub -L ml
	atf_check -s exit:0 -o "inline:${helptxt}\n" $mlmmjsub -h
	atf_check -s exit:0 -o "match:mlmmj-unsub version" $mlmmjsub -V
	atf_check -s exit:1 -e "inline:mlmmj-unsub: You have to specify -L and -a\n$mlmmjsub -h for help\n" $mlmmjsub
}
