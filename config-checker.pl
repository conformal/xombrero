#!/usr/bin/perl -P

# Copyright (c) 2012 Stevan Andjelkovic <stevan.andjelkovic@strath.ac.uk>
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


use constant SETTINGS => "settings.c";
use constant CONFIG   => "xxxterm.conf";
use constant MAIN     => "xxxterm.c";

# This is a script that checks if the default setting values of xxxterm
# (found in SETTINGS) are reflected by the CONFIG. It is meant to help
# the developers; the end user need not bother with it.

# The rule of the game: if a setting has a boolean value (i.e. on or
# off), then the config value should be the negation of the default
# value, otherwise (i.e. the value is non-boolean) the config value
# should be the default value.

sub follows_the_rule {
	my ($default_value, $config_value) = @_;

	# Boolean setting.
	if ($default_value =~ /^(0|1)$/) {
		if ($default_value != $config_value) {
			return (1);
		}
	# Non-boolean setting.
	} else {
		if ($default_value eq $config_value) {
			return (1);
		}
	}
	return (0);
}

# There are plenty of limitations, for example: struct settings are
# ignored, some string settings are initialized in special ways (we only
# account for strings initialized in main() using g_strdup.), etc.

# The following are settings which will not be checked by the script,
# because either they are exceptions to the rule or it is hard to check
# them in a systematic way. Check them manually.
my @unsupported = qw(
  url_regex
  http_proxy
  browser_mode
  default_script
  search_string

  cmd_font       cmd_font_name
  oops_font      oops_font_name
  tabbar_font    tabbar_font_name
  statusbar_font statusbar_font_name

  runtime_settings
  cookie_wl
  js_wl
  keybinding
  mime_type
  alias
  );

# The -P flag invokes the C pre-processor.
#define DEBUG	(0)

use strict;
#if (DEBUG)
use warnings;
#endif

my %settings = ();
my %config   = ();


# ----------------------------------------------------------------------
# Main


# Walk through SETTINGS and collect all settings and their default
# values. Note that the default string values cannot be initialized in
# SETTINGS.
with_file(SETTINGS, \&process_settings, \%settings);

# We need to go through MAIN to get the default string values.
with_file(MAIN,     \&process_main,     \%settings);

# Then walk through CONFIG and collect all settings and their config
# values.
with_file(CONFIG,   \&process_config,   \%config);

# Finally check if the collected settings follow the rule.
summary();


# ----------------------------------------------------------------------
# Helper subroutines


# Higher-order subroutine that takes a file, a subroutine and a hash. It
# opens the file and applies the subroutine to each line of the file. The
# hash is used for debugging output.
sub with_file {

	my ($file, $sub, $hash) = @_;

	open(my $fh, "<", $file)
	    or die "Cannot open $file: $!";

	while (<$fh>) {
		&$sub($_);
	}

	close($fh)
    	    or warn "Cannot close $file: $!";

	if (DEBUG) {
		print("$file:\n" . "=" x length($file) . "\n");
 		while (my ($key, $value) = each(%{$hash})) {
 			print "$key => $value\n";
 		}
	}
}

sub process_settings {

	# Save integer settings.
	if (/^[g]?int\s+(\w+)\s+=\s(\d+);/) {
		$settings{$1} = $2;
	}

	# Save float settings.
	if (/^[g]?float\s+(\w+)\s+=\s(\d+\.\d+);/) {
		$settings{$1} = $2;
	}

	# Save boolean settings.
	if (/^gboolean\s+(\w+)\s+=\s(\w+);/) {
		if ($2 eq "TRUE") {
			$settings{$1} = 1;
		} elsif ($2 eq "FALSE") {
			$settings{$1} = 0;
		} else {
			die "$1 got non-boolean value $2 in "
			    . SETTINGS;
		}
	}

	# Save string setting. Note that the default string cannot be
	# initialized in SETTINGS, that is why we also process MAIN.
	if (/^[g]?char\s+\*(\w+)\s+=\s+NULL;/ or
	    /^[g]?char\s+(\w+)\[\w+\];/) {
		$settings{$1} = "NULL";
	}
}

my $found_main = 0;

sub process_main {

	# Find main(), as that is where the strings are initialized.
	unless ($found_main) {
		if (/^main\(int argc, char \*argv\[\]\)$/) {
			$found_main = 1;
		}
		return;
	}

	# Once we found main(), get the string settings.
 	for my $setting (keys %settings) {
		$setting = quotemeta $setting;
		if (/^\s+$setting\s+=\s+g_strdup\("(.*)"\);/) {
			$settings{$setting} = $1;
		}
	}
}

sub process_config {

 	# Save integer and boolean settings.
 	if (/^#\s+(\w+)\s+=\s+(\d+)$/) {
 		$config{$1} = $2;
 		return;
 	}

 	# Save float settings.
 	if (/^#\s+(\w+)\s+=\s+(\d+\.\d+)$/) {
 		$config{$1} = $2;
 		return;
 	}

 	# Save string settings.
 	if (/^#\s+(\w+)\s+=\s+(.*)$/) {
 		$config{$1} = $2;
 	}
}

my @breaks_the_rule     = ();
my @missing_in_config   = ();
my @missing_in_settings = ();

sub summary {

	print_with_sep("Summary", "=");

	# Collect settings that are in SETTINGS, but missing in CONFIG
	# as well as settings that break the rule.
	while (my ($setting, $default_value) = each(%settings)) {

		my $config_value = $config{$setting};

		if (!defined $config_value) {
			push(@missing_in_config, $setting);
			next;
		}

		if (!follows_the_rule($default_value, $config_value)) {
			push(@breaks_the_rule, $setting);
		}
	}

	# Collect settings that are in CONFIG, but not in SETTINGS.
	for my $setting (keys %config) {
		if (!defined $settings{$setting}) {
			push(@missing_in_settings, $setting);
		}
	}

	# Report the settings that break the rule.
	print_with_sep("Settings that break the rule", "-");
	for my $setting (@breaks_the_rule) {
		print SETTINGS . ":\t$setting = $settings{$setting}\n"
		    . CONFIG   . ":\t$setting = $config{$setting}\n\n"
		    unless $setting ~~ @unsupported;
	}

	# Report the settings that are in SETTINGS, but not in CONFIG.
	print_with_sep("Settings in " . SETTINGS . " that are not in " . CONFIG, "-");
	map { print "$_ = $settings{$_}\n" unless $_ ~~ @unsupported }
	    @missing_in_config;

	# And vice versa.
	print_with_sep("Settings in " . CONFIG . " that are not in " . SETTINGS, "-");
	map { print "$_ = $config{$_}\n" unless $_ ~~ @unsupported }
	    @missing_in_settings;

	print_with_sep("Manually check the following", "-");
	map { print "$_\n" } @unsupported;
}

sub print_with_sep {
	my ($line, $sep) = @_;

	print "\n$line\n" . $sep x length($line) . "\n";
}
