#! /usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	basics \
	simple \
	simple \
	subscribe \
	subscribe_digest \
	subscribe_nomail \
	subscribe_nomail_confunsub \
	subscribe_digest_confunsub \
	subscribe_both \
	ctrl_list \
	ctrl_help \
	ctrl_faq \
	ctrl_get \
	subscription_moderation \
	moderation \
	moderation_autosubscribe \
	moderation_notifymod \
	moderation_notmetoo \
	moderation_reject_invalid \
	maxmailsize \
	maxmailsize0 \
	normal_email \
	delheaders \
	delheaders_extras \
	customheaders \
	customheaders_blanks \
	customheaders_with_subst \
	verp \
	normal_email_with_dot \
	multi_line_headers \
	subrelease

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
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
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
Content-Type: text/plain; charset=utf-8
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
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
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
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
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
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
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
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
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
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
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
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
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
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
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

ctrl_help_body() {
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	cat > ctrlhelp << EOF
From: bob@test
To: test+help@mlmmjtest
Return-Path: jane@test
EOF

cat > expected-1.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Information_for_test=40mlmmjtest?=
From: test+owner@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Here is some information about the list.

You can subscribe to the following versions:

- The normal version: Every time a post is sent to the list, subscribers
  receive a copy of it. Subscribe by emailing <test+subscribe@mlmmjtest>.

- The digest version: Subscribers receive multiple posts in a single mail
  message, at regular intervals, or when a lot of posts have accumulated.
  Subscribe by emailing <test+subscribe-digest@mlmmjtest>.

- The no-mail version: Subscribers do not receive any posts to the list.
  This means, though, they are able to post to a list which only
  subscribers may post to, while they follow the list using a web archive
  or another subscribed email address. Subscribe by emailing
  <test+subscribe-nomail@mlmmjtest>.

Unsubscribe by emailing <test+unsubscribe@mlmmjtest>.

Posts are made by emailing <test@mlmmjtest>.

Subscribers can retrieve message number N from the list's archive by
sending a message to <test+get-N@mlmmjtest> (change the N to the number of
the desired message).

You can retrieve the frequently asked questions document for the list by
sending a message to <test+faq@mlmmjtest>.

To contact the list owner, send a message to <test+owner@mlmmjtest>.


.
QUIT
EOF
	atf_check $mlmmjreceive -L list -F <ctrlhelp
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-1.txt
}

ctrl_faq_body() {
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	cat > ctrlhelp << EOF
From: bob@test
To: test+faq@mlmmjtest
Return-Path: jane@test
EOF

cat > expected-1.txt << EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Frequently_asked_questions_of_test=40mlmmjtest?=
From: test+owner@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Sorry, no FAQ available yet.


.
QUIT
EOF
	atf_check $mlmmjreceive -L list -F <ctrlhelp
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-1.txt
}

ctrl_get_body() {
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	cat > ctrlget << EOF
From: bob@test
To: test+get-1@mlmmjtest
Return-Path: jane@test
EOF

	cat > ctrlbadget <<EOF
From: bob@text
To: test+get-1ab2@mlmmjtest
Return-Path: jane@test
EOF
	touch list/control/noget
	atf_check -s exit:0 $mlmmjreceive -L list -F <ctrlget
	atf_check -s exit:1 test -f mail-1.txt

	rm list/control/noget
	touch list/control/subonlyget
	atf_check -s exit:0 $mlmmjreceive -L list -F <ctrlget
	atf_check -s exit:1 test -f mail-1.txt

	echo "bob@test" > list/subscribers.d/b
	atf_check -s exit:0 $mlmmjreceive -L list -F <ctrlget
	atf_check -s exit:1 test -f mail-1.txt

	rm list/control/subonlyget
	atf_check -s exit:0 $mlmmjreceive -L list -F <ctrlget
	atf_check -s exit:1 test -f mail-1.txt

	atf_check -s exit:0 $mlmmjreceive -L list -F <ctrlbadget
	atf_check -s exit:1 test -f mail-1.txt

cat > list/archive/1 <<EOF
meh
EOF

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-bob=test@mlmmjtest>
RCPT TO:<bob@test>
DATA
meh

.
QUIT
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <ctrlget
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-1.txt

cat > list/archive/1 <<EOF
From: plop
To: test@mlmmjtest
Subject: meh

meh
EOF

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-bob=test@mlmmjtest>
RCPT TO:<bob@test>
DATA
From: plop
To: test@mlmmjtest
Subject: meh

meh

.
QUIT
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <ctrlget
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-2.txt
}

subscription_moderation_body() {
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	echo "me@modos" > list/control/owner
	touch list/control/submod

cat > subs <<EOF
From: bob@test
To: test+subscribe@mlmmjtest
Return-Path: bob@test
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <subs

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
Return-Path: bob@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <confirm
cat > expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Awaiting_permission_to_join_test=40mlmmjtest?=
From: test+owner@mlmmjtest
To: bob@test
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

Your request to join the list has been received. However, the gatekeepers
are being asked to review it before permitting you to join.


.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d" mail-2.txt
	cat > expected-3.txt << EOF
EHLO heloname
MAIL FROM:<test+owner@mlmmjtest>
RCPT TO:<me@modos>
DATA
Subject: =?utf-8?q?Subscription_request_for_test=40mlmmjtest:_bob=40test?=
From: test+owner@mlmmjtest
To: test-moderators@mlmmjtest
MIME-Version: 1.0
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit
Reply-To: test+permit-@mlmmjtest

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

There has been a request from <bob@test> to join the normal version of the
list.

To permit this, please send a message to
<test+permit-@mlmmjtest> which can usually be done simply
by replying to this message.

If you do not want to do this, either send a message to
<test+obstruct-@mlmmjtest> or simply ignore this message.

The following gatekeepers have received this mail:
- me@modos


.
QUIT
EOF
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d; s/permit-.*@mlmmjtest/permit-@mlmmjtest/g; s/obstruct-.*@mlmmjtest/obstruct-@mlmmjtest/g" mail-3.txt
	var=$(ls list/moderation/subscribe*)
	cat > reject << EOF
From: me@modos
To: test+obstruct-${var##*/subscribe}@mlmmjtest
Return-Path: bob@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <reject
	atf_check -s exit:1 test -f mail-4.txt
	atf_check -s exit:1 test -f $var
	atf_check $mlmmjreceive -L list -F <subs
	atf_check -s exit:1 test -f mail-5.txt
	atf_check -s exit:1 test -f mail-6.txt
	var=$(ls list/subconf)
	cat > confirm << EOF
From: bob@test
To: test+confsub-${var##*/}@mlmmjtest
Return-Path: bob@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <confirm
	var=$(ls list/moderation/subscribe*)
	cat > permit << EOF
From: me@modos
To: test+permit-${var##*/subscribe}@mlmmjtest
Return-Path: bob@test
Subject: subscribe

this is a subscribtion test
EOF
	atf_check $mlmmjreceive -L list -F <permit
	atf_check -o inline:"bob@test\n" cat list/subscribers.d/b
	cat > expected-7.txt << EOF
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

A gatekeeper has permitted you to join us. You have now been added to the
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
	atf_check -o file:expected-7.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-7.txt
}

moderation_init_body() {
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	echo "me@modos" > list/control/moderators
	touch list/control/moderated
	printf "user@test\nuser2@test" > list/subscribers.d/u
}

moderation_test_body() {
	moderator="test-moderators@mlmmjtest"
	rcpto="me@modos"

cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <first

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+owner@mlmmjtest>
RCPT TO:<${rcpto}>
DATA
Subject: =?utf-8?q?Please_moderate_test=40mlmmjtest:_yeah?=
MIME-Version: 1.0
Content-Type: multipart/mixed;
 boundary="boundary"
Content-Transfer-Encoding: 8bit
From: test+owner@mlmmjtest
To: ${moderator}
Reply-To: test+release-@mlmmjtest

--=boundary=
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

A message from <bob@test> with subject "yeah" has been submitted for
posting. You are being asked to moderate because this is a moderated list.
The message is below.

To release it to the list, please send a message to
<test+release-@mlmmjtest> which can usually be done simply
by replying to this message.

If you do not want to do any of this, either send a message to
<test+reject-@mlmmjtest> or simply ignore this message.

The following moderators have received this mail:
- me@modos

EOF
	cat >> expected-1.txt <<EOF
--=boundary=
Content-Type: message/rfc822
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename="message.eml"

From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email
--=boundary=--

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/release-.*@mlmmjtest/release-@mlmmjtest/g; s/reject-.*@mlmmjtest/reject-@mlmmjtest/g; s/boundary=\".*\"/boundary=\"boundary\"/g; s/^--=.*=/--=boundary=/g" mail-1.txt
	atf_check -s exit:1 test -f mail-2.txt
	var=$(ls list/moderation/)
	cat > reject << EOF
From: ${moderator}
To: test+reject-${var}@mlmmjtest
Return-path: ${moderator}
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <reject
	atf_check -o match:"${moderator} rejected ${var#*/}" cat list/mlmmj.operation.log
	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	var=$(ls list/moderation/)
	cat > release << EOF
From: ${moderator}
To: test+release-${var}@mlmmjtest
Return-path: ${moderator}
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <release
	atf_check -o match:"${moderator} released ${var#*/}" cat list/mlmmj.operation.log
	atf_check -s exit:0 test -f list/archive/1
	cat >> expected-3.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-1-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-3.txt
}

moderation_create_body() {
	moderation_init_body
	moderation_test_body
}

moderation_body() {
	moderation_create_body test@modos
}

moderation_autosubscribe_body() {
	moderation_init_body
	atf_check touch list/control/autosubscribe
	moderation_test_body
	atf_check -o inline:"bob@test\n" cat list/nomailsubs.d/b
}


moderation_notifymod_body() {
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	echo "me@modos" > list/control/moderators
	touch list/control/moderated
	touch list/control/notifymod
	printf "user@test\nuser2@test" > list/subscribers.d/u

cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <first

	cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+owner@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Awaiting_release_to_test=40mlmmjtest:_yeah?=
MIME-Version: 1.0
Content-Type: multipart/mixed;
 boundary="boundary"
Content-Transfer-Encoding: 8bit
From: test+owner@mlmmjtest
To: bob@test

--=boundary=
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

The message from <bob@test> with subject "yeah" has been submitted to the
list. However, the moderators are being asked to review it before releasing
it to the list because this is a moderated list.

(The message is below.)

--=boundary=
Content-Type: message/rfc822
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename="message.eml"

From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email
--=boundary=--

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/release-.*@mlmmjtest/release-@mlmmjtest/g; s/reject-.*@mlmmjtest/reject-@mlmmjtest/g; s/boundary=\".*\"/boundary=\"boundary\"/g; s/^--=.*=/--=boundary=/g" mail-1.txt

cat > expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+owner@mlmmjtest>
RCPT TO:<me@modos>
DATA
Subject: =?utf-8?q?Please_moderate_test=40mlmmjtest:_yeah?=
MIME-Version: 1.0
Content-Type: multipart/mixed;
 boundary="boundary"
Content-Transfer-Encoding: 8bit
From: test+owner@mlmmjtest
To: test-moderators@mlmmjtest
Reply-To: test+release-@mlmmjtest

--=boundary=
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

A message from <bob@test> with subject "yeah" has been submitted for
posting. You are being asked to moderate because this is a moderated list.
The message is below.

To release it to the list, please send a message to
<test+release-@mlmmjtest> which can usually be done simply
by replying to this message.

If you do not want to do any of this, either send a message to
<test+reject-@mlmmjtest> or simply ignore this message.

The following moderators have received this mail:
- me@modos

--=boundary=
Content-Type: message/rfc822
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename="message.eml"

From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email
--=boundary=--

.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d; s/release-.*@mlmmjtest/release-@mlmmjtest/g; s/reject-.*@mlmmjtest/reject-@mlmmjtest/g; s/boundary=\".*\"/boundary=\"boundary\"/g; s/^--=.*=/--=boundary=/g" mail-2.txt
	atf_check -s exit:1 test -f mail-3.txt
	var=$(ls list/moderation/)
	cat > reject << EOF
From: test@modos
To: test+reject-${var}@mlmmjtest
Return-path: test@modos
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <reject
	atf_check -o match:"test@modos rejected ${var#*/}" cat list/mlmmj.operation.log
	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	var=$(ls list/moderation/)
	cat > release << EOF
From: test@modos
To: test+release-${var}@mlmmjtest
Return-path: test@modos
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <release
	atf_check -o match:"test@modos released ${var#*/}" cat list/mlmmj.operation.log
	atf_check -s exit:0 test -f list/archive/1
	cat >> expected-3.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-1-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/release-.*@mlmmjtest/release-@mlmmjtest/g; s/reject-.*@mlmmjtest/reject-@mlmmjtest/g; s/boundary=\".*\"/boundary=\"boundary\"/g; s/^--=.*=/--=boundary=/g" mail-3.txt
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d; s/release-.*@mlmmjtest/release-@mlmmjtest/g; s/reject-.*@mlmmjtest/reject-@mlmmjtest/g; s/boundary=\".*\"/boundary=\"boundary\"/g; s/^--=.*=/--=boundary=/g" mail-4.txt
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-5.txt
}

moderation_notmetoo_body() {
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	echo "me@modos" > list/control/moderators
	touch list/control/moderated
	touch list/control/notmetoo
	printf "user@test\nuser2@test" > list/subscribers.d/u

cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <first

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+owner@mlmmjtest>
RCPT TO:<me@modos>
DATA
Subject: =?utf-8?q?Please_moderate_test=40mlmmjtest:_yeah?=
MIME-Version: 1.0
Content-Type: multipart/mixed;
 boundary="boundary"
Content-Transfer-Encoding: 8bit
From: test+owner@mlmmjtest
To: test-moderators@mlmmjtest
Reply-To: test+release-@mlmmjtest

--=boundary=
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

A message from <bob@test> with subject "yeah" has been submitted for
posting. You are being asked to moderate because this is a moderated list.
The message is below.

To release it to the list, please send a message to
<test+release-@mlmmjtest> which can usually be done simply
by replying to this message.

If you do not want to do any of this, either send a message to
<test+reject-@mlmmjtest> or simply ignore this message.

The following moderators have received this mail:
- me@modos

--=boundary=
Content-Type: message/rfc822
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename="message.eml"

From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email
--=boundary=--

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/release-.*@mlmmjtest/release-@mlmmjtest/g; s/reject-.*@mlmmjtest/reject-@mlmmjtest/g; s/boundary=\".*\"/boundary=\"boundary\"/g; s/^--=.*=/--=boundary=/g" mail-1.txt
	atf_check -s exit:1 test -f mail-2.txt
	var=$(ls list/moderation/ | grep -v omit)
	atf_check -o inline:"bob@test" cat list/moderation/${var}.omit
	cat > reject << EOF
From: test@modos
To: test+reject-${var}@mlmmjtest
Return-path: test@modos
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <reject
	atf_check -o match:"test@modos rejected ${var#*/}" cat list/mlmmj.operation.log
	atf_check -s exit:1 test -f list/moderation/${var}
	atf_check -s exit:1 test -f list/moderation/${var}.omit
	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	var=$(ls list/moderation/ | grep -v omit)
	atf_check -o inline:"bob@test" cat list/moderation/${var}.omit
	cat > release << EOF
From: test@modos
To: test+release-${var}@mlmmjtest
Return-path: test@modos
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <release
	atf_check -s exit:1 test -f list/moderation/${var}
	atf_check -s exit:1 test -f list/moderation/${var}.omit
	atf_check -o match:"test@modos released ${var#*/}" cat list/mlmmj.operation.log
	atf_check -s exit:0 test -f list/archive/1
	cat >> expected-3.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-1-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-3.txt
}

moderation_reject_invalid_body() {
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	echo "me@modos" > list/control/moderators
	touch list/control/moderated
	touch list/control/notmetoo
	printf "user@test\nuser2@test" > list/subscribers.d/u

cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <first

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+owner@mlmmjtest>
RCPT TO:<me@modos>
DATA
Subject: =?utf-8?q?Please_moderate_test=40mlmmjtest:_yeah?=
MIME-Version: 1.0
Content-Type: multipart/mixed;
 boundary="boundary"
Content-Transfer-Encoding: 8bit
From: test+owner@mlmmjtest
To: test-moderators@mlmmjtest
Reply-To: test+release-@mlmmjtest

--=boundary=
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

A message from <bob@test> with subject "yeah" has been submitted for
posting. You are being asked to moderate because this is a moderated list.
The message is below.

To release it to the list, please send a message to
<test+release-@mlmmjtest> which can usually be done simply
by replying to this message.

If you do not want to do any of this, either send a message to
<test+reject-@mlmmjtest> or simply ignore this message.

The following moderators have received this mail:
- me@modos

--=boundary=
Content-Type: message/rfc822
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename="message.eml"

From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email
--=boundary=--

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/release-.*@mlmmjtest/release-@mlmmjtest/g; s/reject-.*@mlmmjtest/reject-@mlmmjtest/g; s/boundary=\".*\"/boundary=\"boundary\"/g; s/^--=.*=/--=boundary=/g" mail-1.txt
	atf_check -s exit:1 test -f mail-2.txt
	var=$(ls list/moderation/ | grep -v omit)
	atf_check -o inline:"bob@test" cat list/moderation/${var}.omit
	cat > reject << EOF
From: test@modos
To: test+reject-nopenope@mlmmjtest
Return-path: test@modos
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <reject
	atf_check -s exit:0 test -f list/moderation/${var}
	atf_check -s exit:0 test -f list/moderation/${var}.omit
}

maxmailsize_body() {
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	echo "1" > list/control/maxmailsize
	printf "user@test\nuser2@test" > list/subscribers.d/u

cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <first

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Post_to_test=40mlmmjtest_denied:_yeah?=
MIME-Version: 1.0
Content-Type: multipart/mixed;
 boundary
Content-Transfer-Encoding: 8bit
From: test+owner@mlmmjtest
To: bob@test

--=RANDOM=
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

The message from <bob@test> with subject "yeah" was unable to be delivered
to the list because it exceeded the maximum allowed message size of 1
bytes.

(The beginning of the denied message is below.)

--=RANDOM=
Content-Type: message/rfc822
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename="message.eml"

From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email
--=RANDOM=--

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/boundary=.*/boundary/g; s/--=.*=/--=RANDOM=/g" mail-1.txt
	atf_check -s exit:1 test -f mail-2.txt
	return
}

maxmailsize0_body() {
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	echo "0" > list/control/maxmailsize
	printf "user@test\nuser2@test" > list/subscribers.d/u

cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <first

cat > expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-help@mlmmjtest>
RCPT TO:<bob@test>
DATA
Subject: =?utf-8?q?Post_to_test=40mlmmjtest_denied:_yeah?=
MIME-Version: 1.0
Content-Type: multipart/mixed;
 boundary
Content-Transfer-Encoding: 8bit
From: test+owner@mlmmjtest
To: bob@test

--=RANDOM=
Content-Type: text/plain; charset=utf-8
Content-Transfer-Encoding: 8bit

Hi, this is the Mlmmj program managing the <test@mlmmjtest> mailing list.

The message from <bob@test> with subject "yeah" was unable to be delivered
to the list because it exceeded the maximum allowed message size of 0
bytes.

(The beginning of the denied message is below.)

--=RANDOM=
Content-Type: message/rfc822
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename="message.eml"

From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email
--=RANDOM=--

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d; s/boundary=.*/boundary/g; s/--=.*=/--=RANDOM=/g" mail-1.txt
	atf_check -s exit:1 test -f mail-2.txt
	return
}

normal_email_body()
{
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	printf "user@test\nuser2@test" > list/subscribers.d/u
cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF

cat > nosubject <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test

Let's go, first email
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	cat >> expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-1-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-1.txt
	touch list/control/footer
	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-1.txt
	cat >> expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-2-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-2-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-2.txt
	printf "myfooter\nreally" > list/control/footer
	touch list/control/prefix
	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	cat >> expected-3.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-3-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email
myfooter
really
.
MAIL FROM:<test+bounces-3-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email
myfooter
really
.
QUIT
EOF
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-3.txt
	echo "[plop]" > list/control/prefix
	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	cat >> expected-4.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-4-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: [plop] yeah

Let's go, first email
myfooter
really
.
MAIL FROM:<test+bounces-4-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: [plop] yeah

Let's go, first email
myfooter
really
.
QUIT
EOF
	atf_check -o file:expected-4.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-4.txt

	atf_check -s exit:0 $mlmmjreceive -L list -F <nosubject
	cat >> expected-5.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-5-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: [plop]

Let's go, first email
myfooter
really
.
MAIL FROM:<test+bounces-5-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: [plop]

Let's go, first email
myfooter
really
.
QUIT
EOF
	atf_check -o file:expected-5.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-5.txt

        touch list/control/replyto
	atf_check -s exit:0 $mlmmjreceive -L list -F <nosubject
	cat >> expected-6.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-6-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
Reply-To: bob@test
To: test@mlmmjtest
Subject: [plop]

Let's go, first email
myfooter
really
.
MAIL FROM:<test+bounces-6-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
Reply-To: bob@test
To: test@mlmmjtest
Subject: [plop]

Let's go, first email
myfooter
really
.
QUIT
EOF
	atf_check -o file:expected-6.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-6.txt
}

delheaders_body()
{
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo
	printf "X-H1\nNope\n" > list/control/delheaders

	printf "user@test\nuser2@test" > list/subscribers.d/u
cat > first <<EOF
From: bob@test
To: test@mlmmjtest
X-H1: plop
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF

	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	cat >> expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-1-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-1.txt
}

delheaders_extras_body()
{
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo
	printf "X-k3\nx-h1\nx-L\n\n\n \n\t\nplop\nx-sym-colon:\nNope\n" > list/control/delheaders

	printf "user@test\nuser2@test" > list/subscribers.d/u
cat > first <<EOF
From: bob@test
To: test@mlmmjtest
X-L4: plop
X-K3: plop
X-H2: plop
X-H1: plop
X-sym-COLON: plop
Received: from test.com ( mail.test.com [192.168.168.1])
	by 192.168.168.168 with ESMTP id 0A21C400643
	for <test@mlmmjtest>; Sat, 10 May 2025 17:17:17 +0000 (UTC)
Received: from int.test.com ( mail.int.test.com [10.0.0.1])
 by 192.168.168.1 with ESMTP id 0A41D900249
 for <test@mlmmjtest>; Sat, 10 May 2025 17:16:17 +0000 (UTC)
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF

	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	cat >> expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
X-H2: plop
Received: from test.com ( mail.test.com [192.168.168.1])
	by 192.168.168.168 with ESMTP id 0A21C400643
	for <test@mlmmjtest>; Sat, 10 May 2025 17:17:17 +0000 (UTC)
Received: from int.test.com ( mail.int.test.com [10.0.0.1])
 by 192.168.168.1 with ESMTP id 0A41D900249
 for <test@mlmmjtest>; Sat, 10 May 2025 17:16:17 +0000 (UTC)
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-1-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
X-H2: plop
Received: from test.com ( mail.test.com [192.168.168.1])
	by 192.168.168.168 with ESMTP id 0A21C400643
	for <test@mlmmjtest>; Sat, 10 May 2025 17:17:17 +0000 (UTC)
Received: from int.test.com ( mail.int.test.com [10.0.0.1])
 by 192.168.168.1 with ESMTP id 0A41D900249
 for <test@mlmmjtest>; Sat, 10 May 2025 17:16:17 +0000 (UTC)
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-1.txt
}

customheaders_body()
{
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo
	printf "X-H1: test\nNope: really not\n" > list/control/customheaders

	printf "user@test\nuser2@test" > list/subscribers.d/u
cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF

	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	cat >> expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah
X-H1: test
Nope: really not

Let's go, first email

.
MAIL FROM:<test+bounces-1-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah
X-H1: test
Nope: really not

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-1.txt
cat > second <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Mime: really
Subject: yeah

Let's go, first email
EOF

	atf_check -s exit:0 $mlmmjreceive -L list -F <second
	cat >> expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-2-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
X-H1: test
Nope: really not
Mime: really
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-2-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
X-H1: test
Nope: really not
Mime: really
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-2.txt
}

customheaders_blanks_body()
{
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo
	printf "X-H1: test\nNope: really not\n\n   \n" > list/control/customheaders

	printf "user@test\nuser2@test" > list/subscribers.d/u
cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF

	atf_check -o empty -s exit:0 $mlmmjreceive -L list -F <first
	cat >> expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah
X-H1: test
Nope: really not

Let's go, first email

.
MAIL FROM:<test+bounces-1-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah
X-H1: test
Nope: really not

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-1.txt
cat > second <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Mime: really
Subject: yeah

Let's go, first email
EOF

	atf_check -s exit:0 $mlmmjreceive -L list -F <second
	cat >> expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-2-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
X-H1: test
Nope: really not
Mime: really
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-2-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
X-H1: test
Nope: really not
Mime: really
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-2.txt
}

customheaders_with_subst_body()
{
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo
	printf "X-H1: test\nNope: really not\nX-Poster-Address: \$posteraddr\$\n" > list/control/customheaders

	printf "user@test\nuser2@test" > list/subscribers.d/u
cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF

	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	cat >> expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah
X-H1: test
Nope: really not
X-Poster-Address: bob@test

Let's go, first email

.
MAIL FROM:<test+bounces-1-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah
X-H1: test
Nope: really not
X-Poster-Address: bob@test

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-1.txt
cat > second <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Mime: really
Subject: yeah

Let's go, first email
EOF

	atf_check -s exit:0 $mlmmjreceive -L list -F <second
	cat >> expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-2-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
X-H1: test
Nope: really not
X-Poster-Address: bob@test
Mime: really
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-2-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
X-H1: test
Nope: really not
X-Poster-Address: bob@test
Mime: really
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-2.txt
}

verp_body()
{
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "postfix" > list/control/verp
	echo 2 > list/control/maxverprecips
	echo "heloname" > list/control/smtphelo
	printf "X-H1: test\nNope: really not\n" > list/control/customheaders

	printf "user@test\nuser2@test\nuser3@test" > list/subscribers.d/u
cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF

	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	cat >> expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1@mlmmjtest> XVERP=-=
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-1.txt
cat > second <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Mime: really
Subject: yeah

Let's go, first email
EOF

	cat >> expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1@mlmmjtest> XVERP=-=
RCPT TO:<user@test>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah
X-H1: test
Nope: really not

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-2.txt
	cat >> expected-3.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1@mlmmjtest> XVERP=-=
RCPT TO:<user3@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah
X-H1: test
Nope: really not

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-3.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-3.txt
	atf_check -s exit:1 test -f mail-4.txt
}

normal_email_with_dot_body()
{
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	printf "user@test\nuser2@test" > list/subscribers.d/u
cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Hello
.world
EOF

cat > nosubject <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test

Hello
.world
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	cat >> expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Hello
..world

.
MAIL FROM:<test+bounces-1-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
From: bob@test
To: test@mlmmjtest
Subject: yeah

Hello
..world

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-1.txt
	touch list/control/footer
	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-1.txt
}

multi_line_headers_body()
{
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	printf "user@test\nuser2@test" > list/subscribers.d/u
cat > first <<EOF
Received: from test.com ( mail.test.com [192.168.168.1])
	by 192.168.168.168 with ESMTP id 0A21C400643
	for <test@mlmmjtest>; Sat, 10 May 2025 17:17:17 +0000 (UTC)
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF

	atf_check -s exit:0 $mlmmjreceive -L list -F <first
	cat >> expected-1.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-1-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
Received: from test.com ( mail.test.com [192.168.168.1])
	by 192.168.168.168 with ESMTP id 0A21C400643
	for <test@mlmmjtest>; Sat, 10 May 2025 17:17:17 +0000 (UTC)
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-1-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
Received: from test.com ( mail.test.com [192.168.168.1])
	by 192.168.168.168 with ESMTP id 0A21C400643
	for <test@mlmmjtest>; Sat, 10 May 2025 17:17:17 +0000 (UTC)
From: bob@test
To: test@mlmmjtest
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-1.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-1.txt
cat > second <<EOF
Received: from test.com ( mail.test.com [192.168.168.1])
	by 192.168.168.168 with ESMTP id 0A21C400643
	for <test@mlmmjtest>; Sat, 10 May 2025 17:17:17 +0000 (UTC)
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Mime: really
Subject: yeah

Let's go, first email
EOF

	atf_check -s exit:0 $mlmmjreceive -L list -F <second
	cat >> expected-2.txt <<EOF
EHLO heloname
MAIL FROM:<test+bounces-2-user=test@mlmmjtest>
RCPT TO:<user@test>
DATA
Received: from test.com ( mail.test.com [192.168.168.1])
	by 192.168.168.168 with ESMTP id 0A21C400643
	for <test@mlmmjtest>; Sat, 10 May 2025 17:17:17 +0000 (UTC)
From: bob@test
To: test@mlmmjtest
Mime: really
Subject: yeah

Let's go, first email

.
MAIL FROM:<test+bounces-2-user2=test@mlmmjtest>
RCPT TO:<user2@test>
DATA
Received: from test.com ( mail.test.com [192.168.168.1])
	by 192.168.168.168 with ESMTP id 0A21C400643
	for <test@mlmmjtest>; Sat, 10 May 2025 17:17:17 +0000 (UTC)
From: bob@test
To: test@mlmmjtest
Mime: really
Subject: yeah

Let's go, first email

.
QUIT
EOF
	atf_check -o file:expected-2.txt sed -e "/^Message-ID:/d; /^Date:/d;" mail-2.txt
}

subrelease_body()
{
	init_ml list
	rmdir list/text
	ln -s ${top_srcdir}/listtexts/en list/text
	echo test@mlmmjtest > list/control/listaddress
	start_fakesmtp list
	echo "heloname" > list/control/smtphelo

	touch list/control/subonlypost
	printf "user@test\nuser2@test" > list/subscribers.d/u

cat > first <<EOF
From: bob@test
To: test@mlmmjtest
Return-path: bob@test
Subject: yeah

Let's go, first email
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <first

	# Verify the mail was held in moderation
	var=$(ls list/moderation/)
	if [ -z "$var" ]; then
		atf_fail "No file in moderation directory"
	fi

	# Verify the deny-post mail contains subrelease addresses
	atf_check -o match:"test\+subrelease-${var}@mlmmjtest" cat mail-1.txt
	atf_check -o match:"test\+digestsubrelease-${var}@mlmmjtest" cat mail-1.txt
	atf_check -o match:"test\+nomailsubrelease-${var}@mlmmjtest" cat mail-1.txt
	# Verify Reply-To is set to subrelease address
	atf_check -o match:"Reply-To: test\+subrelease-${var}@mlmmjtest" cat mail-1.txt

	# Now send to the subrelease address to subscribe and release
cat > subrel <<EOF
From: bob@test
To: test+subrelease-${var}@mlmmjtest
Return-path: bob@test
EOF
	atf_check -s exit:0 $mlmmjreceive -L list -F <subrel

	# Verify the sender is subscribed
	atf_check -o match:"bob@test" cat list/subscribers.d/b

	# Verify the post was released (archived)
	atf_check -s exit:0 test -f list/archive/1

	# Verify the released mail was sent to all subscribers (order is non-deterministic)
	atf_check -o match:"EHLO heloname" cat mail-2.txt
	atf_check -o match:"RCPT TO:<user@test>" cat mail-2.txt
	atf_check -o match:"RCPT TO:<user2@test>" cat mail-2.txt
	atf_check -o match:"RCPT TO:<bob@test>" cat mail-2.txt
	atf_check -o match:"Subject: yeah" cat mail-2.txt
	atf_check -o match:"Let's go, first email" cat mail-2.txt
	atf_check -o match:"QUIT" cat mail-2.txt
}
