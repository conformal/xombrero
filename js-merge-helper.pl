#!/bin/env perl -w
# Copyright (c) 2009 Leon Winter
# Copyright (c) 2009, 2010 Hannes Schueller
# Copyright (c) 2009, 2010 Matto Fransen
# Copyright (c) 2010 Hans-Peter Deifel
# Copyright (c) 2010 Thomas Adam
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

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

if (scalar @ARGV < 1) {
	print "usage: js-merge-helper.pl jsfile ... \n";
	exit 1;
}

my ($jsfile, $define, $js);

while (@ARGV) {

	$jsfile = shift @ARGV;
	my @fn = split /\//, $jsfile;
	my $fn = pop @fn;
	$fn =~ /^(.*)\.js$/;

	$define = "JS_".uc($1);
	$define =~ s/\-/_/;

	open(JSFILE, $jsfile) or die "Failed to open file: $!";
	$_ = do { local $/; <JSFILE> };
	close(JSFILE);

	$js = escape_c_string($_);

	print "#define $define ";
	printf "\"%s\"\n", $js;

}

exit;
