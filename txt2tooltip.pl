#!/bin/perl

# Copyright (c) 2012 Todd T. Fries <todd@fries.net>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

# read in 'ascii2txt.pl' formatted man pages, spit out #defines for tooltips

use strict;
use warnings;

our $verbose = 0;

my $line;
my @lines;
my @states = ("prekewords", "keywords", "postkeywords");
my $state = 0;

my $tipname;
my @tipinfo;
my $tiplines;
while(<STDIN>) {
	chomp($line = $_);
	if ($state == 0) {
		if ($line =~ /following keywords:$/) {
			$state++;
		}
		next;
	}
	if ($state == 1) {
		if ($line =~ /^$/) {
			next;
		}
		if ($line =~ /^[A-Z]/) {
			showtip($tipname,@tipinfo);
			$state++;
			next;
		}
		if ($line =~ /^ {11,11}([a-z_]+)[ ]*(.*)$/) {
			showtip($tipname,@tipinfo);
			$tiplines = 1;
			$tipname = $1;
			@tipinfo = ($2);
			next;
		}
		if ($line =~ /^ {39,39}(.*)$/) {
			push @tipinfo, $1;
		}
	}
}

sub showtip {
	my ($tip,@info) = @_;

	my $text;
	my $count = 1;

	if (length($tip) < 1) {
		return;
	}

	my $fmt;
	for my $line (@info) {
		$line =~ s/&/&amp;/g;
		$line =~ s/</&lt;/g;
		$line =~ s/>/&gt;/g;
		$line =~ s/"/&quot;/g;
		$line =~ s/^ {6,6}/\\t/g;
		$line =~ s/\\t {6,6}/\\t\\t/g;
		$line =~ s/\\t {6,6}/\\t\\t/g;
		if ($count > $#info) {
			$fmt = "	\"%s\"\n";
		} else {
			$fmt = "	\"%s\\n\" \\\n";
		}
		$text .= sprintf $fmt,$line;
		$count++;
	}

	$tip = uc($tip);
	printf "#define TT_%s%s",$tip,$text;
}
exit(0);
