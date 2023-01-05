#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	basics \
	normal \
	nomail \
	digest \
	moderators \
	owner

normal_body()
{
	init_ml ml

	atf_check mlmmj-list -L ml -s
	atf_check -o "inline:0\n" mlmmj-list -L ml -s -c

	mkdir ml/subscribers.d/j
	atf_check mlmmj-list -L ml -s
	atf_check -o "inline:0\n" mlmmj-list -L ml -s -c
	rmdir ml/subscribers.d/j

	touch ml/subscribers.d/j
	atf_check mlmmj-list -L ml -s
	atf_check -o "inline:0\n" mlmmj-list -L ml -s -c

	echo "john@doe.org" > ml/subscribers.d/j
	atf_check -o "inline:john@doe.org\n" mlmmj-list -L ml -s
	atf_check -o "inline:1\n" mlmmj-list -L ml -s -c

	# check the default
	atf_check -o "inline:john@doe.org\n" mlmmj-list -L ml
	atf_check -o "inline:1\n" mlmmj-list -L ml -c

	echo "jane@doe.org" >> ml/subscribers.d/j
	atf_check -o "inline:john@doe.org\njane@doe.org\n" mlmmj-list -L ml
	atf_check -o "inline:2\n" mlmmj-list -L ml -c

	echo -n "jack@doe.org" >> ml/subscribers.d/j
	atf_check -o "inline:john@doe.org\njane@doe.org\njack@doe.org\n" mlmmj-list -L ml
	atf_check -o "inline:3\n" mlmmj-list -L ml -c
}

nomail_body()
{
	init_ml ml
	echo "john@doe.org" > ml/nomailsubs.d/j
	atf_check -o "inline:john@doe.org\n" mlmmj-list -L ml -n
	atf_check -o "inline:1\n" mlmmj-list -L ml -n -c
}

digest_body()
{
	init_ml ml
	echo "john@doe.org"> ml/digesters.d/j
	atf_check -o "inline:john@doe.org\n" mlmmj-list -L ml -d
	atf_check -o "inline:1\n" mlmmj-list -L ml -d -c
}

moderators_body()
{
	init_ml ml
	echo "john@doe.org"> ml/control/moderators
	atf_check -o "inline:john@doe.org\n" mlmmj-list -L ml -m
	atf_check -o "inline:1\n" mlmmj-list -L ml -m -c
}

owner_body()
{
	init_ml ml
	echo "john@doe.org"> ml/control/owner
	atf_check -o "inline:john@doe.org\n" mlmmj-list -L ml -o
	atf_check -o "inline:1\n" mlmmj-list -L ml -o -c
}

basics_body()
{

	helptxt="Usage: mlmmj-list -L /path/to/listdir
       [-c] [-d] [-h] [-m] [-n] [-o] [-s] [-V]
 -L: Full path to list directory
 -c: Print subscriber count
 -d: Print for digesters list
 -h: This help
 -m: Print moderators for list
 -n: Print for nomail version of list
 -o: Print owner(s) of list
 -s: Print normal subscribers (default) 
 -V: Print version
"
	atf_check -s exit:1 -e "inline:mlmmj-list: You have to specify -L\nmlmmj-list -h for help\n" mlmmj-list
	atf_check -s exit:1 -e "inline:mlmmj-list: Unable to open(nonexisting): No such file or directory\n" mlmmj-list -L nonexisting
	mkdir ml
	atf_check -s exit:1 -e "inline:mlmmj-list: Unable to open(ml/subscribers.d): No such file or directory\n" mlmmj-list -L ml
	atf_check -s exit:0 -o "match:mlmmj-list version .*" mlmmj-list -V
	atf_check -s exit:0 -o "inline:$helptxt" mlmmj-list -h
}
