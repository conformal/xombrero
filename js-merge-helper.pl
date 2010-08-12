#!/bin/env perl -w
use strict;

sub escape_c_string {
	shift;
	s/\t|\r|\n/ /g;     # convert spacings to whitespaces
	s/[^\/]\/\*.*?\*\///g;   # remove comments (careful: hinttags look like comment!)
	s/ {2,}/ /g;        # strip whitespaces
	s/(^|\(|\)|;|,|:|\}|\{|=|\+|\-|\*|\&|\||\<|\>|!) +/$1/g;
	s/ +($|\(|\)|;|,|:|\}|\{|=|\+|\-|\*|\&|\||\<|\>|!)/$1/g;
	s/\\/\\\\/g;        # escape backslashes
	s/\"/\\\"/g;        # escape quotes
	return $_
}

open(JSFILE, "hinting.js") or die "Failed to open file: $!";
$_ = do { local $/; <JSFILE> };
close(JSFILE);
my $js_hints = escape_c_string($_);

open(JSFILE, "input-focus.js") or die "Failed to open file: $!";
$_ = do { local $/; <JSFILE> };
close(JSFILE);
my $js_input = escape_c_string($_);

open(HFILE, ">javascript.h") or die "Failed to open javascript.h: $!";
print HFILE "#define JS_SETUP_HINTS ";
printf  HFILE "\"%s\"\n", $js_hints;
print HFILE "#define JS_SETUP_INPUT_FOCUS ";
printf  HFILE "\"%s\"\n", $js_input;
close(HFILE);

exit;
