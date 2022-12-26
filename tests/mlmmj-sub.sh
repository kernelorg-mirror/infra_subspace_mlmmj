#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	add_basics \
	add_normal \
	add_normal_owner \
	add_nomail \
	add_digest \
	add_normal_then_nomail \
	add_both \
	add_both_with_digest \
	add_both_with_normal \
	remove_basics \
	remove_normal \
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
