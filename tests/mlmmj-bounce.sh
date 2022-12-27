#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	basics_0 \
	basics_1 \
	basics_2 \
	basics_3 \
	basics_4 \
	basics_5 \
	basics_6 \
	basics_7 \
	bounce_subscription \
	probe \
	dsnparse

basics_0_body()
{

	mlmmjbounce=$(command -v mlmmj-bounce)
	helptxt="Usage: $mlmmjbounce -L /path/to/list
       [-a john=doe.org | -d] [-n num | -p]
 -a: Address string that bounces
 -h: This help
 -L: Full path to list directory
 -n: Message number in the archive
 -d: Attempt to parse DSN to determine address
 -p: Send out a probe
 -V: Print version
"
	init_ml ml
	echo test@mlmmjtest > ml/control/listaddress

	atf_check -s exit:1 -e "inline:mlmmj-bounce: You have to specify -L, -a or -d and -n or -p\n$mlmmjbounce -h for help\n" $mlmmjbounce
	atf_check -s exit:0 -o "inline:$helptxt" $mlmmjbounce -h
}

basics_1_body()
{
	mlmmjbounce=$(command -v mlmmj-bounce)
	init_ml ml
	echo test@mlmmjtest > ml/control/listaddress
	# no '=' in the mail address
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me -n 2
}

basics_2_body()
{
	mlmmjbounce=$(command -v mlmmj-bounce)
	init_ml ml
	echo test@mlmmjtest > ml/control/listaddress

	touch ${TMPDIR}/plop
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me -n 2 -m ${TMPDIR}/plop
	atf_check -s exit:1 test -f ${TMPDIR}/plop
}

basics_3_body()
{
	mlmmjbounce=$(command -v mlmmj-bounce)
	init_ml ml
	echo test@mlmmjtest > ml/control/listaddress
	# not subscribed
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me=meh -n 2
}

basics_4_body()
{
	mlmmjbounce=$(command -v mlmmj-bounce)
	init_ml ml
	echo test@mlmmjtest > ml/control/listaddress
	touch ${TMPDIR}/plop
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me=meh -n 2 -m ${TMPDIR}/plop
	atf_check -s exit:1 test -f ${TMPDIR}/plop
}

basics_5_body()
{
	mlmmjbounce=$(command -v mlmmj-bounce)
	init_ml ml
	echo test@mlmmjtest > ml/control/listaddress
	# now the user is subscribed but the file is a symlink
	echo "me@meh" > ml/subscribers.d/m
	ln -sf  plop ml/bounce/me=meh
	atf_check -s exit:1 $mlmmjbounce -L ./ml -a me=meh -n 2
	rm ml/bounce/me=meh
}

basics_6_body()
{
	mlmmjbounce=$(command -v mlmmj-bounce)
	init_ml ml
	echo test@mlmmjtest > ml/control/listaddress
	echo "me@meh" > ml/subscribers.d/m
	# not allowed to write
	chmod 444 ml/bounce
	atf_check -s exit:1 $mlmmjbounce -L ./ml -a me=meh -n 2
	chmod 755 ml/bounce
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me=meh -n 2
	atf_check test -f ml/bounce/me=meh
}

basics_7_body()
{
	mlmmjbounce=$(command -v mlmmj-bounce)
	init_ml ml
	echo test@mlmmjtest > ml/control/listaddress
	echo "me@meh" > ml/subscribers.d/m
	echo "coucou" > ${TMPDIR}/plop
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me=meh -n 2 -m ${TMPDIR}/plop
	atf_check -o inline:"coucou\n" cat ml/bounce/me=meh.lastmsg
	atf_check -s exit:1 test -f ${TMPDIR}/plop
	rm ml/bounce/me=meh.lastmsg
}

bounce_subscription_body()
{
	mlmmjbounce=$(command -v mlmmj-bounce)

	init_ml ml
	echo test@mlmmjtest > ml/control/listaddress
	touch ml/subconf/me=meh
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me=meh -n confsub
	atf_check -s exit:1 test -f ml/subconf/me=meh
	touch plop
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me=meh -n confsub -m ${TMPDIR}/plop
	atf_check -s exit:1 test -f ${TMPDIR}/plop

	touch ml/unsubconf/me=meh
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me=meh -n confunsub
	atf_check -s exit:1 test -f ml/unsubconf/me=meh
	touch plop
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me=meh -n confunsub -m ${TMPDIR}/plop
	atf_check -s exit:1 test -f ${TMPDIR}/plop
}

probe_body()
{
	mlmmjbounce=$(command -v mlmmj-bounce)

	init_ml ml
	echo test@mlmmjtest > ml/control/listaddress
	touch ml/bounce/me=meh-probe
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me=meh -n probe
	atf_check -s exit:1 test -f ml/bounce/me=meh-probe

	touch ${TMPDIR}/plop
	atf_check -s exit:0 $mlmmjbounce -L ./ml -a me=meh -n probe -m ${TMPDIR}/plop
	atf_check -s exit:1 test -f ${TMPDIR}/plop

}

dsnparse_body()
{
	mlmmjbounce=$(command -v mlmmj-bounce)

	init_ml ml
	echo test@mlmmjtest > ml/control/listaddress

	cp $(atf_get_srcdir)/dsnmail dsnmail
	atf_check -s exit:0 $mlmmjbounce -L ./ml -d -n 2 -m dsnmail

}
