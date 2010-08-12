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
