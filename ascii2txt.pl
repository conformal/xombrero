#!/bin/perl

# Copyright (c) 2010,2011,2012 Todd T. Fries <todd@fries.net>
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

# read in 'mandoc -Tascii' formatted man pages, spit out txt useful for further
# processing by other utilities

use strict;
use warnings;

our $fileinfo = $ARGV[0];

our $verbose = 0;

my $line;
my @lines;
while(<STDIN>) {
	$line = $_;
	push @lines,$line;
}
my $oline = "";
my $fmtline = "%s";
foreach $line (@lines) {

	my $newline = "";
	foreach my $seg (split(/(.\x08.)/,$line)) {
		my $newseg = $seg;
		$newseg =~ m/^(.)\x08(.)$/;
		if (!defined($1) || !defined($2)) {
			$newline .= $seg;
			next;
		}
		if ($1 eq $2) {
			$newline .= "${2}";
			next;
		}
		if ($1 eq "_") {
			$newline .= "${2}";
			next;
		}
		$newline .= $seg;
		next;
	}
	if ($verbose > 0) {
		printf STDERR "==> text{bf,it}\n   line: <%s>\nnewline: <%s>\n",$line,$newline;
	}
	$line = $newline;
	$line =~ m/(.)\x08/;
	if (defined($1)) {
		printf STDERR "Removing %s\\x08\n",$1;
	}
	$line =~ s/.\x08//g;

	# combine adjacent entries
	foreach my $macro (("textbf", "textit")) {
		$oline = "";
		while ($oline ne $line) {
			#printf STDERR "combine adjacent\n";
			$oline = $line;
			$line =~ s/\xab\\${macro}\{([^\}]*)\}\xbb\xab\\${macro}\{([^\}]*)\}\xbb/\xab\\${macro}\{$1$2\}\xbb/g;
		}
	}
	# combine space separated
	foreach my $macro (("textbf")) {
		#printf STDERR "combine space\n";
		$oline = "";
		while ($oline ne $line) {
			$oline = $line;
			$line =~ s/\xab\\${macro}\{([^\}]*)\}\xbb[ ]+\xab\\${macro}\{([^\}]*)\}\xbb/\xab\\${macro}\{$1 $2\}\xbb/g;
		}
	}

	# do the substitution one at a time to be sure to add all man pages, not just the last ones per line.
	# XXX provide an exceptions list, audio(9) has mono(1) and stereo(2)
	# XXX references, which are _not_ man pages
	$oline = "";
	while ($oline ne $line) {
		$oline=$line;
		$line =~ s/\{(http|ftp|https):\/\/(.*)\}/ $1:\/\/$2 /;
		if (0) {
		if ($line =~ m/ ([a-z][a-z0-9\.\-\_]*)\(([1-9])\)([,\.\) ])/) {
			my $quote = texquote($1);
			$line =~ s/ ([a-z][a-z0-9\.\-\_]*)\(([1-9])\)([,\.\) ])/ \xab\\man{$quote}{$2}\xbb$3/;
		}
		
		if ($line =~ m/ ([a-z][a-z0-9\.\-\_]*)\(([1-9])\)$/) {
			my $quote = texquote($1);
			$line =~ s/ ([a-z][a-z0-9\.\-\_]*)\(([1-9])\)$/ \xab\\man{$quote}{$2}\xbb/;
		}
		}
	}
	my @macros = ("textbf","textit","man","href");
	# quote arguments for tex
	foreach my $macro (@macros) {
		my $newline = "";
		foreach my $seg (split(/(\xab\\${macro}\{[^\xbb]*\}\xbb)/,$line)) {
			#printf STDERR "quote args\n";
			my $newseg = $seg;
			# check for nesting first; we only want to escape the
			# inner most argument, process nested macro if it has a nested macro
			# since the nested macro won't catch in the other regex cases
			my $foundnest = 0;
			foreach my $nest (@macros) {
				if ($macro eq $nest) {
					next;
				}
				
				$newseg =~ m/^\xab\\${macro}\{[ ]*\\${nest}\{([^\xbb]*)\}\{([^\xbb]*)\}\}\xbb$/;
				if (defined($2)) {
					$foundnest = 1;
					$newline .= "\xab\\${macro}\{\\${nest}\{".texquote($1)."\}\{".texquote(${2})."\}\}\xbb";
					last;
				}
				$newseg =~ m/^\xab\\${macro}\{[ ]*\\${nest}\{([^\xbb]*)\}\}\xbb$/;
				if (defined($1)) {
					$foundnest = 1;
					$newline .= "\xab\\${macro}\{\\${nest}\{".texquote($1)."\}\}\xbb";
					last;
				}
			}
			if ($foundnest > 0) {
				next;
			}
				
			# check for 2 args first
			$newseg =~ m/^\xab\\${macro}\{([^\xbb]*)\}\{([^\xbb]*)\}\xbb$/;
			if (defined($2)) {
				$newline .= "\xab\\${macro}\{".texquote($1)."\}\{".texquote(${2})."\}\xbb";
				next;
			}
			$newseg =~ m/^\xab\\${macro}\{([^\xbb]*)\}\xbb$/;
			if (defined($1)) {
				$newline .= "\xab\\${macro}\{".texquote($1)."\}\xbb";
				next;
			}
			$newline .= $seg;
		}
		$line = $newline;
	}
	printf $fmtline,$line;
}

1;

sub texquote {
        my ($text) = @_;
        my ($ret) = "";
        my ($esctest) = "";
        my ($escbase) = "BaCkSlAsH";
        my ($esccount) = 0;

	#$verbose++;
	if ($verbose > 0) {
        	printf STDERR "\ntexquote: '%s' -> ",$text;
	}

        if ($text =~ m/\\/) {
                $esctest=sprintf "%s%d",$escbase,$esccount++;
                while ($text =~ m/$esctest/) {
                        $esctest=sprintf "%s%d",$escbase,$esccount++;
                }
                $text =~ s/\\/$esctest/g;
		if ($verbose > 0) {
                	printf STDERR "'%s' -> ",$text;
		}
        }

        $text =~ s/([%\{\}_#\&\$\^])/\\$1/g;
        $text =~ s/([<>\|\*~])/\{\$$1\$\}/g;

        if ($esccount > 0) {
                $text =~ s/$esctest/\$\\backslash\$/g;
        }
	if ($verbose > 0) {
        	printf STDERR "'%s'\n",$text;
	}
	#$verbose--;

        return $text;
}
