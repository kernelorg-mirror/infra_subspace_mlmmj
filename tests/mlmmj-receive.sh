#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	basics \
	simple \
	subscribe \
	subscribe_digest \
	subscribe_nomail \
	subscribe_nomail_confunsub \
	subscribe_digest_confunsub \
	subscribe_both \
	ctrl_list

mlmmjreceive=$(command -v mlmmj-receive)

basics_body()
{

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

simple_body()
{
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	init_ml list
	rmdir list/text
	ln -s ${srcdir}/../listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	echo "25678" > list/control/smtpport
	echo "heloname" > list/control/smtphelo
	cat > incoming-invalid << EOF
From: bob@test
To: invalid@test
Subject: subscribe

this is a subscribtion test
EOF

	atf_check -s exit:1 $mlmmjreceive -L list -F <incoming-invalid
	atf_check -s exit:1 test -f mail-1.txt

	cat > incoming << EOF
From: bob@test
To: invalid@test
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <incoming
cat > expected-1.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Post_to_test=40mlmmjtest_denied:_subscribe?=
MIME-Version: 1.0
Content-Type: multipart/mixed;
 boundary
Content-Transfer-Encoding: 8bit
From: test+owner@mlmmjtest
To: bob@test

--=RANDOM=
Content-Type: text/plain; charset=utf8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

The message from <bob@test> with subject "subscribe" was unable to be
delivered to the list because the list address was not found in either the
To: or CC: header.

(The denied message is below.)

--=RANDOM=
Content-Type: message/rfc822
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename="message.eml"

From: bob@test
To: invalid@test
Subject: subscribe

this is a subscribtion test
--=RANDOM=--

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/boundary=.*/boundary/g; s/--=.*=/--=RANDOM=/g" mail-1.txt
	cat > incoming << EOF
From: bob@test
To: test@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	cat > archived << EOF
From: bob@test
To: test@mlmmjtest
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <incoming
	atf_check -o file:archived cat list/archive/1
}

subscribe_body()
{
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	init_ml list
	rmdir list/text
	ln -s ${srcdir}/../listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	echo "25678" > list/control/smtpport
	echo "heloname" > list/control/smtphelo

	cat > subscribe << EOF
From: bob@test
To: test+subscribe@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <subscribe

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_subscription_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confsub-@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that your email address
<bob@test> be added to the list. This means every time a post is sent to
the list, you will receive a copy of it.

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
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confsub-.*@mlmmjtest/confsub-@mlmmjtest/g" mail-1.txt
	var=$(ls list/subconf)

	cat > confirm << EOF
From: bob@test
To: test+confsub-${var##*/}@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <confirm
	cat > expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Welcome_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your subscription. You have now been added to the
normal version of the list.

The email address you are subscribed with is <bob@test>.

If you ever wish to unsubscribe, send a message to
<test+unsubscribe@mlmmjtest> using this email address. The subject and the
body of the message can be anything. You will then receive confirmation or
further instructions.

For other information and help about this list, send a message to
<test+help@mlmmjtest>.


.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-2.txt
	atf_check -o inline:"bob@test\n" cat list/subscribers.d/b
	atf_check -s exit:1 test -f list/subconf/${var}

# second confirmation
	atf_check $mlmmjreceive -L list -F <confirm
	atf_check -s exit:1 test -f mail-3.txt
	cat > expected-3.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Unable_to_subscribe_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

You were unable to be subscribed to the list because you are already
subscribed.


.
QUIT
EOF
	# New subscribtion
	atf_check $mlmmjreceive -L list -F <subscribe
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-3.txt

	cat > unsubscribe <<EOF
From: bob@test
To: test+unsubscribe@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <unsubscribe
	cat > expected-4.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confunsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_unsubscribe_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confunsub-@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that the email address
<bob@test> be removed from the list.

To confirm you want to do this, please send a message to
<test+confunsub-@mlmmjtest> which can usually be done
simply by replying to this message. The subject and the body of the message
can be anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF
	atf_check -o file:expected-4.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confunsub-.*@mlmmjtest/confunsub-@mlmmjtest/g" mail-4.txt
	var=$(ls list/unsubconf)
	cat > confirm-unsub << EOF
From: bob@test
To: test+confunsub-${var##*/}@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <confirm-unsub
	cat > expected-5.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Goodbye_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your unsubscribe. You have now been removed from
the list.


.
QUIT
EOF
	atf_check -o file:expected-5.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-5.txt
	atf_check -s exit:1 test -f list/subscribers.d/b
}

subscribe_digest_body()
{
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	init_ml list
	rmdir list/text
	ln -s ${srcdir}/../listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	echo "25678" > list/control/smtpport
	echo "heloname" > list/control/smtphelo

	cat > subscribe << EOF
From: bob@test
To: test+subscribe-digest@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <subscribe

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_subscription_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confsub-digest@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that your email address
<bob@test> be added to the list, to receive digests. This means you will
receive multiple posts in a single mail message, at regular intervals, or
when a lot of posts have accumulated.

To confirm you want to do this, please send a message to
<test+confsub-digest@mlmmjtest> which can usually be done
simply by replying to this message. The subject and the body of the message
can be anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confsub-digest.*@mlmmjtest/confsub-digest@mlmmjtest/g; s/bounces-confsub-.*@mlmmjtest/bounces-confsub-@mlmmjtest/g" mail-1.txt
	var=$(ls list/subconf)

	cat > confirm << EOF
From: bob@test
To: test+confsub-digest-${var##*/}@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <confirm
	cat > expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Welcome_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your subscription. You have now been added to the
digest version of the list.

The email address you are subscribed with is <bob@test>.

If you ever wish to unsubscribe, send a message to
<test+unsubscribe@mlmmjtest> using this email address. The subject and the
body of the message can be anything. You will then receive confirmation or
further instructions.

For other information and help about this list, send a message to
<test+help@mlmmjtest>.


.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-2.txt
	atf_check -o inline:"bob@test\n" cat list/digesters.d/b
	atf_check -s exit:1 test -f list/subconf/${var}

# second confirmation
	atf_check $mlmmjreceive -L list -F <confirm
	atf_check -s exit:1 test -f mail-3.txt
	cat > expected-3.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Unable_to_subscribe_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

You were unable to be subscribed to the list because you are already
subscribed.


.
QUIT
EOF
	# New subscribtion
	atf_check $mlmmjreceive -L list -F <subscribe
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-3.txt

	cat > unsubscribe <<EOF
From: bob@test
To: test+unsubscribe-digest@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <unsubscribe
	cat > expected-4.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confunsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_unsubscribe_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confunsub-@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that the email address
<bob@test> be removed from the list.

To confirm you want to do this, please send a message to
<test+confunsub-@mlmmjtest> which can usually be done
simply by replying to this message. The subject and the body of the message
can be anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF
	atf_check -o file:expected-4.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confunsub-.*@mlmmjtest/confunsub-@mlmmjtest/g" mail-4.txt
	var=$(ls list/unsubconf)
	cat > confirm-unsub << EOF
From: bob@test
To: test+confunsub-${var##*/}@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <confirm-unsub
	cat > expected-5.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Goodbye_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your unsubscribe. You have now been removed from
the list.


.
QUIT
EOF
	atf_check -o file:expected-5.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-5.txt
	atf_check -s exit:1 test -f list/digesters.d/b
}

subscribe_nomail_body()
{
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	init_ml list
	rmdir list/text
	ln -s ${srcdir}/../listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	echo "25678" > list/control/smtpport
	echo "heloname" > list/control/smtphelo

	cat > subscribe << EOF
From: bob@test
To: test+subscribe-nomail@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <subscribe

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_subscription_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confsub-nomail@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that your email address
<bob@test> be added to the list, without mail delivery. This means you will
not receive any posts to the list, but you are considered a member. This
means, for instance, you are able to post to a list which only subscribers
may post to, while you follow the list using a web archive or another
subscribed email address.

To confirm you want to do this, please send a message to
<test+confsub-nomail@mlmmjtest> which can usually be done
simply by replying to this message. The subject and the body of the message
can be anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confsub-nomail.*@mlmmjtest/confsub-nomail@mlmmjtest/g; s/bounces-confsub-.*@mlmmjtest/bounces-confsub-@mlmmjtest/g" mail-1.txt
	var=$(ls list/subconf)

	cat > confirm << EOF
From: bob@test
To: test+confsub-nomail-${var##*/}@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <confirm
	cat > expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Welcome_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your subscription. You have now been added to the
no-mail version of the list.

The email address you are subscribed with is <bob@test>.

If you ever wish to unsubscribe, send a message to
<test+unsubscribe@mlmmjtest> using this email address. The subject and the
body of the message can be anything. You will then receive confirmation or
further instructions.

For other information and help about this list, send a message to
<test+help@mlmmjtest>.


.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-2.txt
	atf_check -o inline:"bob@test\n" cat list/nomailsubs.d/b
	atf_check -s exit:1 test -f list/subconf/${var}

# second confirmation
	atf_check $mlmmjreceive -L list -F <confirm
	atf_check -s exit:1 test -f mail-3.txt
	cat > expected-3.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Unable_to_subscribe_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

You were unable to be subscribed to the list because you are already
subscribed.


.
QUIT
EOF
	# New subscribtion
	atf_check $mlmmjreceive -L list -F <subscribe
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-3.txt

	cat > unsubscribe <<EOF
From: bob@test
To: test+unsubscribe-nomail@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <unsubscribe
	cat > expected-4.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confunsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_unsubscribe_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confunsub-@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that the email address
<bob@test> be removed from the list.

To confirm you want to do this, please send a message to
<test+confunsub-@mlmmjtest> which can usually be done
simply by replying to this message. The subject and the body of the message
can be anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF
	atf_check -o file:expected-4.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confunsub-.*@mlmmjtest/confunsub-@mlmmjtest/g" mail-4.txt
	var=$(ls list/unsubconf)
	cat > confirm-unsub << EOF
From: bob@test
To: test+confunsub-${var##*/}@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <confirm-unsub
	cat > expected-5.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Goodbye_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your unsubscribe. You have now been removed from
the list.


.
QUIT
EOF
	atf_check -o file:expected-5.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-5.txt
	atf_check -s exit:1 test -f list/nomailsubs.d/b
}

subscribe_both_body()
{
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	init_ml list
	rmdir list/text
	ln -s ${srcdir}/../listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	echo "25678" > list/control/smtpport
	echo "heloname" > list/control/smtphelo

	cat > subscribe << EOF
From: bob@test
To: test+subscribe-both@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <subscribe

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_subscription_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confsub-both@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that your email address
<bob@test> be added

To confirm you want to do this, please send a message to
<test+confsub-both@mlmmjtest> which can usually be done
simply by replying to this message. The subject and the body of the message
can be anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confsub-both.*@mlmmjtest/confsub-both@mlmmjtest/g; s/bounces-confsub-.*@mlmmjtest/bounces-confsub-@mlmmjtest/g" mail-1.txt
	var=$(ls list/subconf)

	cat > confirm << EOF
From: bob@test
To: test+confsub-both-${var##*/}@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <confirm
	cat > expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Welcome_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your subscription. You have now been added to the
version of the list.

The email address you are subscribed with is <bob@test>.

If you ever wish to unsubscribe, send a message to
<test+unsubscribe@mlmmjtest> using this email address. The subject and the
body of the message can be anything. You will then receive confirmation or
further instructions.

For other information and help about this list, send a message to
<test+help@mlmmjtest>.


.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-2.txt
	atf_check -o inline:"bob@test\n" cat list/subscribers.d/b
	atf_check -o inline:"bob@test\n" cat list/digesters.d/b
	atf_check -s exit:1 test -f list/subconf/${var}

# second confirmation
	atf_check $mlmmjreceive -L list -F <confirm
	atf_check -s exit:1 test -f mail-3.txt
	cat > expected-3.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Unable_to_subscribe_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

You were unable to be subscribed to the list because you are already
subscribed.


.
QUIT
EOF
	# New subscribtion
	atf_check $mlmmjreceive -L list -F <subscribe
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-3.txt

	cat > unsubscribe <<EOF
From: bob@test
To: test+unsubscribe@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <unsubscribe
	cat > expected-4.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confunsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_unsubscribe_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confunsub-@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that the email address
<bob@test> be removed from the list.

To confirm you want to do this, please send a message to
<test+confunsub-@mlmmjtest> which can usually be done
simply by replying to this message. The subject and the body of the message
can be anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF
	atf_check -o file:expected-4.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confunsub-.*@mlmmjtest/confunsub-@mlmmjtest/g" mail-4.txt
	var=$(ls list/unsubconf)
	cat > confirm-unsub << EOF
From: bob@test
To: test+confunsub-${var##*/}@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <confirm-unsub
	cat > expected-5.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Goodbye_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your unsubscribe. You have now been removed from
the list.


.
QUIT
EOF
	atf_check -o file:expected-5.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-5.txt
	atf_check -s exit:1 test -f list/subscribers.d/b
	atf_check -s exit:1 test -f list/digesters.d/b
}

subscribe_nomail_confunsub_body()
{
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	init_ml list
	rmdir list/text
	ln -s ${srcdir}/../listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	echo "25678" > list/control/smtpport
	echo "heloname" > list/control/smtphelo

	cat > subscribe << EOF
From: bob@test
To: test+subscribe-nomail@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <subscribe

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_subscription_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confsub-nomail@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that your email address
<bob@test> be added to the list, without mail delivery. This means you will
not receive any posts to the list, but you are considered a member. This
means, for instance, you are able to post to a list which only subscribers
may post to, while you follow the list using a web archive or another
subscribed email address.

To confirm you want to do this, please send a message to
<test+confsub-nomail@mlmmjtest> which can usually be done
simply by replying to this message. The subject and the body of the message
can be anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confsub-nomail.*@mlmmjtest/confsub-nomail@mlmmjtest/g; s/bounces-confsub-.*@mlmmjtest/bounces-confsub-@mlmmjtest/g" mail-1.txt
	var=$(ls list/subconf)

	cat > confirm << EOF
From: bob@test
To: test+confsub-nomail-${var##*/}@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <confirm
	cat > expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Welcome_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your subscription. You have now been added to the
no-mail version of the list.

The email address you are subscribed with is <bob@test>.

If you ever wish to unsubscribe, send a message to
<test+unsubscribe@mlmmjtest> using this email address. The subject and the
body of the message can be anything. You will then receive confirmation or
further instructions.

For other information and help about this list, send a message to
<test+help@mlmmjtest>.


.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-2.txt
	atf_check -o inline:"bob@test\n" cat list/nomailsubs.d/b
	atf_check -s exit:1 test -f list/subconf/${var}

# second confirmation
	atf_check $mlmmjreceive -L list -F <confirm
	atf_check -s exit:1 test -f mail-3.txt
	cat > expected-3.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Unable_to_subscribe_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

You were unable to be subscribed to the list because you are already
subscribed.


.
QUIT
EOF
	# New subscribtion
	atf_check $mlmmjreceive -L list -F <subscribe
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-3.txt

	cat > unsubscribe <<EOF
From: bob@test
To: test+unsubscribe-nomail@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <unsubscribe
	cat > expected-4.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confunsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_unsubscribe_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confunsub-@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that the email address
<bob@test> be removed from the list.

To confirm you want to do this, please send a message to
<test+confunsub-@mlmmjtest> which can usually be done
simply by replying to this message. The subject and the body of the message
can be anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF
	atf_check -o file:expected-4.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confunsub-.*@mlmmjtest/confunsub-@mlmmjtest/g" mail-4.txt
	var=$(ls list/unsubconf)
	cat > confirm-unsub << EOF
From: bob@test
To: test+confunsub-nomail-${var##*/}@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <confirm-unsub
	cat > expected-5.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Goodbye_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your unsubscribe. You have now been removed from
the list.


.
QUIT
EOF
	atf_check -o file:expected-5.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-5.txt
	atf_check -s exit:1 test -f list/nomailsubs.d/b
}

subscribe_digest_confunsub_body()
{
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	init_ml list
	rmdir list/text
	ln -s ${srcdir}/../listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	echo "25678" > list/control/smtpport
	echo "heloname" > list/control/smtphelo

	cat > subscribe << EOF
From: bob@test
To: test+subscribe-digest@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <subscribe

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_subscription_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confsub-digest@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that your email address
<bob@test> be added to the list, to receive digests. This means you will
receive multiple posts in a single mail message, at regular intervals, or
when a lot of posts have accumulated.

To confirm you want to do this, please send a message to
<test+confsub-digest@mlmmjtest> which can usually be done
simply by replying to this message. The subject and the body of the message
can be anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confsub-digest.*@mlmmjtest/confsub-digest@mlmmjtest/g; s/bounces-confsub-.*@mlmmjtest/bounces-confsub-@mlmmjtest/g" mail-1.txt
	var=$(ls list/subconf)

	cat > confirm << EOF
From: bob@test
To: test+confsub-digest-${var##*/}@mlmmjtest
Return-Path: jane@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <confirm
	cat > expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Welcome_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your subscription. You have now been added to the
digest version of the list.

The email address you are subscribed with is <bob@test>.

If you ever wish to unsubscribe, send a message to
<test+unsubscribe@mlmmjtest> using this email address. The subject and the
body of the message can be anything. You will then receive confirmation or
further instructions.

For other information and help about this list, send a message to
<test+help@mlmmjtest>.


.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-2.txt
	atf_check -o inline:"bob@test\n" cat list/digesters.d/b
	atf_check -s exit:1 test -f list/subconf/${var}

# second confirmation
	atf_check $mlmmjreceive -L list -F <confirm
	atf_check -s exit:1 test -f mail-3.txt
	cat > expected-3.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Unable_to_subscribe_to_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

You were unable to be subscribed to the list because you are already
subscribed.


.
QUIT
EOF
	# New subscribtion
	atf_check $mlmmjreceive -L list -F <subscribe
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-3.txt

	cat > unsubscribe <<EOF
From: bob@test
To: test+unsubscribe-digest@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <unsubscribe
	cat > expected-4.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-confunsub-@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Confirm_unsubscribe_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+confunsub-@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Somebody (and we hope it was you) has requested that the email address
<bob@test> be removed from the list.

To confirm you want to do this, please send a message to
<test+confunsub-@mlmmjtest> which can usually be done
simply by replying to this message. The subject and the body of the message
can be anything.

After doing so, you should receive a reply informing you that the operation
succeeded.

If you do not want to do this, simply ignore this message.


.
QUIT
EOF
	atf_check -o file:expected-4.txt sed -e "/^Message-ID:/d; /^Date:/d; s/confunsub-.*@mlmmjtest/confunsub-@mlmmjtest/g" mail-4.txt
	var=$(ls list/unsubconf)
	cat > confirm-unsub << EOF
From: bob@test
To: test+confunsub-digest-${var##*/}@mlmmjtest
Return-Path: jane@test
EOF
	atf_check $mlmmjreceive -L list -F <confirm-unsub
	cat > expected-5.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Goodbye_from_test=40mlmmjtest?=
From: test+help@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Thank you for confirming your unsubscribe. You have now been removed from
the list.


.
QUIT
EOF
	atf_check -o file:expected-5.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-5.txt
}

ctrl_list_body() {
	atf_check $srcdir/fakesmtpd
	trap kill_fakesmtp EXIT TERM
	init_ml list
	rmdir list/text
	ln -s ${srcdir}/../listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	echo "25678" > list/control/smtpport
	echo "heloname" > list/control/smtphelo

	cat > ctrllist << EOF
From: bob@test
To: test+list@mlmmjtest
Return-Path: jane@test
EOF

	atf_check $mlmmjreceive -L list -F <ctrllist
	atf_check -s exit:1 test -f mail-1.txt

	echo "bob@test" > list/control/owner
	atf_check $mlmmjreceive -L list -F <ctrllist

cat > expected-1.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Subscribers_to_test=40mlmmjtest?=
From: test+owner@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Here is the list of subscribers (to all versions of the list):



.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-1.txt

	printf "bob@test" > list/control/owner
	printf "jane@doe.org\njo@bob.org\n" > list/subscribers.d/j
	printf "jane@doe.org\n" > list/digesters.d/j
	printf "bob@doe.org" > list/nomailsubs.d/b
cat > expected-2.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Subscribers_to_test=40mlmmjtest?=
From: test+owner@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Here is the list of subscribers (to all versions of the list):

- jane@doe.org
- jo@bob.org
- jane@doe.org
- bob@doe.org


.
QUIT
EOF
	atf_check $mlmmjreceive -L list -F <ctrllist
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-2.txt
	touch list/control/nolistsubsemail
	atf_check $mlmmjreceive -L list -F <ctrllist
	atf_check -s exit:1 test -f mail-3.txt
}
