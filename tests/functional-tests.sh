#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	discard

discard_body()
{

	mlmmjrecv=$(command -v mlmmj-receive)
	mlmmjmaint=$(command -v mlmmj-maintd)
	init_ml list
	echo "list@test" > list/control/listaddress
	here=$(pwd)

	atf_check -s exit:0 $mlmmjrecv -L list -F <<-EOF
bla
EOF
	atf_check -o inline:"bla\n" cat list/queue/discarded/*
	# the discarded file is too new, do not drop it yet
	atf_check -s exit:0 -e ignore $mlmmjmaint -F -L ${here}/list
	atf_check -o inline:"bla\n" cat list/queue/discarded/*
	atf_check touch -m -t 197001010101 list/queue/discarded/*
	atf_check -s exit:0 -e ignore $mlmmjmaint -F -L ${here}/list
	atf_check -o inline:"list/queue/discarded\n" find list/queue/discarded  -type d -empty

	#now with relative path
	atf_check -s exit:0 $mlmmjrecv -L list -F <<-EOF
bla
EOF
	atf_check -o inline:"bla\n" cat list/queue/discarded/*
	atf_check -s exit:0 -e ignore $mlmmjmaint -F -L list
	atf_check -o inline:"bla\n" cat list/queue/discarded/*
	atf_check touch -m -t 197001010101 list/queue/discarded/*
	atf_check -s exit:0 -e ignore $mlmmjmaint -F -L list
	atf_check -o inline:"list/queue/discarded\n" find list/queue/discarded  -type d -empty
}
