#!/usr/bin/perl

use strict;
use warnings;

use YAML;

$SIG{__WARN__} = sub { die $_[0] };

my %data;
my %ns;

while (<>) {
	my $file = $ARGV;
	next unless (m{/\*\*});
	$_ = <>;
	defined $_ or exit;
	next unless m{
		^\s*
		# DBG: and the like
		( \S+ \s+ )?
		# eg MOWS (command), LAWN (agent)
		(\S+) \s* \((\w+)\) \s*
		( (?:
			# argument bit
			# we parse this in more detail later
			(?:\w+) \s*
			(?:\([^)]+\)) \s*
		)* )
		\s*$
	}x;
	my ($cns, $cname, $ctype, $argdata) = ($1, $2, $3, $4);
	if (defined $cns) {
		$cns =~ s/\s//g;
	}

	my $fullname = ($cns ? "$cns " : "") . $cname;

	my $impl;
	if ($ctype eq 'command') {
		$impl .= 'c_';
	} else {
		$impl .= 'v_';
	}
	if ($cns && $cns ne '') {
		$_ = $cns . "_";
		$_ =~ s/[^a-zA-Z0-9_]//g;
		$impl .= uc $_;
	}
	$_ = $cname;
	$_ =~ s/[^a-zA-Z0-9_]//g;
	$impl .= $_;
	my $key = $impl;
	$impl = "caosVM::$impl";

	my @args;
	while ($argdata =~ s/.*?(\w+)\s*\(([^)]+)\)\s*//) {
		my ($argname, $argtype) = ($1, $2);
		push @args, {
			name => $argname,
			type => $argtype,
		};
	}

	my @lines;
	DOCLINE: while (<>) {
		last DOCLINE if m{\*/};
		$_ =~ m{^\s*(.*?)\s*$};
		push @lines, $1;
	}
	shift @lines while (@lines && $lines[0] eq '');
	pop @lines while (@lines && $lines[-1] eq '');

	my %pragma;
	while (@lines && $lines[0] =~ s{^\%pragma\s+}{}) {
		unless ($lines[0] =~ m{(\w+)\s*(.*)}) {
			warn "bad pragma";
		}
		shift @lines;
		$pragma{$1} = $2;
		chomp $pragma{$1};
		if ($pragma{$1} eq '') {
			$pragma{$1} = 1;
		}
	}

	if ($pragma{implementation}) {
		$impl = $pragma{implementation};
	}

	my $desc = join("\n", @lines);
	$desc .= "\n";
	
	$data{$key} = {
		type => $ctype,
		name => $fullname,
		match => $cname,
		arguments => \@args,
		description => @lines ? $desc : undef,
		filename => $file,
		implementation => $impl,
	};
	if ($cns && $cns ne '') {
		$data{$key}{namespace} = lc $cns;
		$ns{lc $cns} = 1;
	}
	if (%pragma) {
		$data{$key}{pragma} = \%pragma;
	}
}

print Dump {
	ops => \%data,
	namespaces => [keys %ns],
};
# vim: set noet: 
