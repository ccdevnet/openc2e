#!/usr/bin/perl

use strict;
use warnings;

use YAML;

my %data;

while (<>) {
    my $file = $ARGV[0];
    next unless (m{/\*\*});
    $_ = <>;
    defined $_ or exit;
    next unless m{
        ^\s*
        # eg MOWS (command), LAWN (agent)
        (\w+) \s* \((\w+)\) \s*
        ( (?:
            # argument bit
            # we parse this in more detail later
            (?:\w+) \s*
            (?:\(\w+\)) \s*
        )* )
        \s*$
    }x;
    my ($cname, $ctype, $argdata) = ($1, $2, $3);

    my @args;
    while ($argdata =~ s/.*?(\w+)\s*\((\w+)\)\s*//) {
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

    my $desc = join("\n", @lines);
    $desc .= "\n";
    
    my $key = $cname;
    if ($ctype eq 'command') {
        $key = "c_$key";
    } else {
        $key = "v_$key";
    }
    
    $data{$key} = {
        type => $ctype,
        name => $cname,
        arguments => \@args,
        description => @lines ? $desc : undef,
        filename => $file,
    };
    if (%pragma) {
        $data{$key}{pragma} = \%pragma;
    }
}

print Dump \%data;
