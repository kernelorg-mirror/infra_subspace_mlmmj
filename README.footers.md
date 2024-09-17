README.footers

# Footers in Mlmmj

Mlmmj's built-in footer support is very rudimentary. It will work for plain
text emails, and that's about it. It doesn't understand HTML or MIME or
anything like that.

There are a few solutions to this. They all involve piping incoming mail
through a filter before it reaches Mlmmj. A script to do this, called
mlmmj-amime-receive, is included in amime-receive in the contrib directory.

It can be used with a number of different pre-processors. One day we also hope
to improve the integration of these external filters with Mlmmj, e.g. so only
list posts are processed. However, the piping solution has worked for a number
of people over the years quite satisfactorily, so this is not a high priority.

Here are some pre-processors you can use.

## alterMIME

The mlmmj-amime-receive script is designed to work with a program called
alterMIME. The script itself (in amime-receive in the contrib directory)
contains links to that software, and instructions.
