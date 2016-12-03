#!/usr/bin/perl

use strict;
use warnings;

use YAML;

my @variants = qw(c3 cv sm);

$SIG{__WARN__} = sub { die $_[0] };

my %fnmap = ( # default category mappings
	'caosVM_agent.cpp' => 'Agents',
	'caosVM_camera.cpp' => 'Cameras',
	'caosVM_core.cpp' => 'Core functions',
	'caosVM_creatures.cpp' => 'Creatures',
	'caosVM_compound.cpp' => 'Compound agents',
	'caosVM_debug.cpp' => 'Debugging',
	'caosVM_files.cpp' => 'Files',
	'caosVM_flow.cpp' => 'Flow control',
	'caosVM_genetics.cpp' => 'Genetics',
	'caosVM_history.cpp' => 'History',
	'caosVM_input.cpp' => 'Input',
	'caosVM_map.cpp' => 'Map',
	'caosVM_motion.cpp' => 'Motion',
	'caosVM_net.cpp' => 'Networking',
	'caosVM_ports.cpp' => 'Ports',
	'caosVM_resources.cpp' => 'Resources',
	'caosVM_scripts.cpp' => 'Scripts',
	'caosVM_sounds.cpp' => 'Sound',
	'caosVM_time.cpp' => 'Time',
	'caosVM_variables.cpp' => 'Variables',
	'caosVM_vectors.cpp' => 'Vectors',
	'caosVM_vehicles.cpp' => 'Vehicles',
	'caosVM_world.cpp' => 'World',
);

my %data;
my %ns;

my $prev;

my $missing_status = 0;

while (<>) {
	my $file = $ARGV;
	$file =~ s{.*/}{};
	
	unless (m{/\*\*}) {
		if (m/STUB|TODO/ && defined $prev && !defined $prev->{status}) {
			$prev->{status} = 'stub';
		}
		next;
	}
	$_ = <>;
	defined $_ or exit;
	$_ =~ s/^\s* \** \s*//x; # accept leading * etc
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
	my $firstline = $_;
	
	my ($cns, $cname, $ctype, $argdata) = ($1, $2, $3, $4);
	if (defined $cns) {
		$cns =~ s/\s//g;
	}

	my $fullname = ($cns ? "$cns " : "") . $cname;

	my $impl;
	my $saveimpl;
	if ($ctype eq 'command') {
		$impl = 'c_';
	} else {
		$impl = 'v_';
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
	my $stackdelta = ($ctype eq 'command' ? 0 : 1);



	my @args;
	while ($argdata =~ s/.*?(\w+)\s*\(([^)]+)\)\s*//) {
		my ($argname, $argtype) = ($1, $2);
		push @args, {
			name => $argname,
			type => $argtype,
		};
		$stackdelta-- unless $argtype =~ /variable/;
	}

	my @lines;
	DOCLINE: while (<>) {
		last DOCLINE if m{\*/};
		$_ =~ m{^\s* (?: \* \s* )* (.*?) \s*$}x;
		push @lines, $1;
	}
	shift @lines while (@lines && $lines[0] eq '');
	pop @lines while (@lines && $lines[-1] eq '');

	my %pragma;
	my %evalcost;
	my $status;
	my $cat;

	if ($ctype eq 'command') {
		$evalcost{default} = 1;
	} else {
		$evalcost{default} = 0;
	}

	while (@lines && ($lines[0] =~ s{^\%([a-zA-Z]+)\s+}{} || $lines[0] =~ m{^\s*$})) {
		my $l = shift @lines;
		chomp $l;
		next unless $1;
		if ($1 eq 'pragma') {
			unless ($l =~ m{(\w+)\s*(.*)}) {
				warn "bad pragma";
			}
			$pragma{$1} = $2;
			chomp $pragma{$1};
			if ($pragma{$1} eq '') {
				$pragma{$1} = 1;
			}
		} elsif ($1 eq 'status') {
			if ($status) {
				die "Set status twice";
			}
			$status = $l;
			chomp $status;
		} elsif ($1 eq 'category') {
			if ($cat) { 
				die "set category twice";
			}
			$cat = $l;
		} elsif ($1 eq 'cost') {
			if ($l =~ m{^\s*(-?\d+)\s*$}) {
				$evalcost{default} = $1;
			} elsif ($l =~ m{(\S+)\s+(-?\d+)\s*$}) {
				my $cost = $2;
				my @variants = split ',', $1;
				for my $v (@variants) {
					$evalcost{$v} = $cost;
				}
			} else {
				die "malformed cost directive";
			}
		} else {
			die "Unrecognized directive: $1";
		}
	}

	if (!$status) {
		$missing_status++;
		print STDERR "Missing \%status for $fullname\n";
	}

	if (!$cat) {
		if ($fnmap{$file}) {
			$cat = lc $fnmap{$file};
		} else {
			$cat = lc 'unknown';
		}
	}
	
	if (defined $pragma{stackdelta}) {
		$stackdelta = $pragma{stackdelta};
	}
	
	if (defined $pragma{stackdelta} && lc $pragma{stackdelta} eq "any") {
		$stackdelta = "INT_MAX";
	}
	
	die "Deprecated use of pragma retc for $fullname" if defined $pragma{retc};


	if ($pragma{implementation}) {
		$impl = $pragma{implementation};
	}
	if ($pragma{saveimpl}) {
		$saveimpl = $pragma{saveimpl};
	} else {
		if ($ctype eq 'variable') {
			$saveimpl = $impl;
			$saveimpl =~ s/caosVM::v/caosVM::s/;
		} else {
			$saveimpl = "caosVM::dummy_cmd";
		}
	}
	$firstline =~ s/^\s*//;
	my $desc = join("\n", @lines);
	$desc .= "\n";
	
	my $cd = {
		type => $ctype,
		name => $fullname,
		match => $cname,
		arguments => \@args,
		syntaxstring => $firstline,
		description => @lines ? $desc : undef,
		filename => $file,
		implementation => $impl,
		saveimpl => $saveimpl,
		status => $status,
		category => $cat,
		evalcost => \%evalcost,
		stackdelta => $stackdelta,
	};
	if (%pragma) {
		$cd->{pragma} = \%pragma;
	}
	if ($cns && $cns ne '') {
		$cd->{namespace} = lc $cns;
	}
	unless ($cd->{status}) {
		$cd->{status} = 'probablyok';
	}
	$prev = $cd;
	
	my @v = @variants;
	if ($pragma{variants}) {
		@v = grep { $_ ne ''; } split ' ', $pragma{variants};
	}

	for my $v (@v) {
		if ($v eq 'all') {
			@v = qw(all);
			last;
		}
	}

	for my $v (@v) {
		if (exists $data{$v}{$key}) {
			print STDERR "Name collision for ($key) in variant $v\n";
			exit 1;
		}
		$data{$v}{$key} = $cd;
	}
}

if ($missing_status) {
	print STDERR "$missing_status commands are missing \%status, fixit.\n";
	exit 1;
}

for my $key (keys %{$data{all}}) {
	for my $variant (keys %data) {
		next if $variant eq 'all';
		if (exists $data{$variant}{$key}) {
			print STDERR "Name collision for ($key) in variant $variant\n";
			exit 1;
		}
		$data{$variant}{$key} = $data{all}{$key};
	}
}

delete $data{all};

print Dump {
	variants => \%data,
	namespaces => [keys %ns],
};
# vim: set noet: 
