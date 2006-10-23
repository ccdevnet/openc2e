#!/usr/bin/perl

use strict;
use warnings;

use YAML;

my %tdisp = (
	'float' => 'CI_NUMERIC',
	'integer' => 'CI_NUMERIC',
	'string' => 'CI_STRING',
	'agent' => 'CI_AGENT',
	'bytestring' => 'CI_BYTESTR',
	'variable' => 'CI_VARIABLE',
	'any' => 'CI_OTHER',
	'anything' => 'CI_OTHER',
	'condition' => '(err)',
	'comparison' => '(err)',
	'decimal' => 'CI_NUMERIC',
	'decimal variable' => 'CI_OTHER',
	'byte-string' => 'CI_BYTESTR',
	'label' => '(err)',
	'vector' => 'CI_VECTOR',
	'bareword' => 'CI_BAREWORD',
	'token' => 'CI_BAREWORD',
);

# zero-tolerance policy
$SIG{__WARN__} = sub { die $_[0] };

my $data = YAML::LoadFile($ARGV[0]);

print <<ENDHEAD1;
// THIS FILE IS AUTOMATICALLY GENERATED
// DO NOT EDIT
ENDHEAD1
print "// Generated at ".scalar (localtime)."\n\n";
print <<ENDHEAD2;
#include "cmddata.h"
#include "dialect.h"
#include "caosVM.h"
#include <string>
#include <map>
#include <iostream>

#ifdef VCHACK
#define CDATA
#else
#define CDATA const
#endif
ENDHEAD2

my $disp_id = 0x00000001;
my @disp_tbl;

foreach my $variant (sort keys %{$data->{variants}}) {
	my $defn = "static CDATA cmdinfo v_". $variant. "_cmds[] = {\n";
	my $argdef = '';
	
	my $idx = 0;
	my $d = $data;
	my $data = $d->{variants}{$variant};

	foreach my $key (sort keys %$data) {
		my $cedocs = cescape($data->{$key}{description} || "UNDOCUMENTED");
		my $syntax = cescape($data->{$key}{syntaxstring});
		my $argc = scalar @{$data->{$key}{arguments}};
		my $name = lc $data->{$key}{match};
		my $fullname = $data->{$key}{name};
		my $retc = $data->{$key}{type} eq 'command' ? 0 : 1;
		if (defined $data->{$key}{pragma}{retc}) {
			$retc = $data->{$key}{pragma}{retc};
		}
		my $delegate;
		my $implementation = undef;
		if ($data->{$key}{pragma}{noparse}) {
			$delegate = undef;
		} elsif ($data->{$key}{pragma}{parser}) {
			$delegate = $data->{$key}{pragma}{parser};
		} else {
			my $class = 'DefaultParser';
			if ($data->{$key}{pragma}{parserclass}) {
				$class = $data->{$key}{pragma}{parserclass};
			}
			$data->{$key}{implementation} =~ s/caosVM:://;
			$implementation = "$data->{$key}{implementation}";
			$delegate = qq{new $class(&v_${\$variant}_cmds[$idx])};
		}

		my $args;
		if (defined $data->{$key}{arguments}) {
			my $as = '';
			foreach my $arg (@{$data->{$key}{arguments}}) {
				my $t = 'CI_OTHER';
				if (defined $tdisp{$arg->{type}}) {
					$t = $tdisp{$arg->{type}};
				} else {
					if (!exists $tdisp{$arg->{type}}) {
						print STDERR "Warning: unknown $arg->{type} in $key ($data->{$key}{filename})\n";
						undef $tdisp{$arg->{type}}
					}
				}
				if ($t eq '(err)') {
					undef $as; 
					last;
				}
				$as .= "$t, ";
			}
			if (defined $as) {
				$as .= 'CI_END';
				$args = "{ $as }";
			}
		}
		my $argp = "NULL";
		if (defined $args) {
			print "static const enum ci_type a_",$variant,"_$key","[] = $args;\n";
			$argp = "a_$variant"."_$key";
		}

		
			
		$data->{$key}{delegate} = $delegate;
		$data->{$key}{idx} = $idx;
		my $did = sprintf "0x%08X", $disp_id;
		$defn .= <<ENDDATA;
		{ // $idx
			$did,
			"$variant $key",
			"$name",
			"$fullname",
			"$syntax $cedocs",
			$argc,
			$retc,
			$argp
		},
ENDDATA
		$disp_tbl[$disp_id] = $implementation;
		$data->{$key}{disp_id} = $disp_id++;
		$idx++;
	}

	print $defn.<<ENDTAIL;
	{ 0, NULL, NULL, NULL, NULL, 0, 0, NULL }
};

static void registerAutoDelegates_$variant() {
	if (variants.find("$variant") == variants.end()) {
		variants["$variant"] = new Variant();
		variants["$variant"]->cmds = v_${variant}_cmds;
		variants["$variant"]->exp_dialect = new ExprDialect();
		variants["$variant"]->cmd_dialect = new Dialect();
	}
	Variant *v = variants["$variant"];
	std::map<std::string, NamespaceDelegate *> nsswitch_cmd, nsswitch_exp;
#define NS_REG(m, ns, name, d) do { \\
	if (m.find(ns) == m.end()) \\
		m[ns] = new NamespaceDelegate(); \\
	m[ns]->dialect.delegates[name] = d; \\
} while (0)
ENDTAIL

	foreach my $key (keys %$data) {
		if (defined($data->{$key}{delegate})) {
			my $type = $data->{$key}{type} eq 'command' ? 'cmd' : 'exp';
			if ($data->{$key}{namespace}) {
				my $c = $data->{$key}{type} eq 'command';
				print "NS_REG(nsswitch_$type, \"";
				print $data->{$key}{namespace};
				print "\", \"", lc($data->{$key}{match}), "\", ";
				print $data->{$key}{delegate}, ");\n";
			} else {
				print q{v->};
				print $type;
				print "_dialect->delegates[\"", lc($data->{$key}{match}), "\"] = ";
				print $data->{$key}{delegate}, ";\n";
			}
		}
	}

	for my $T (qw(cmd exp)) {
		print <<END;
	{
		std::map<std::string, NamespaceDelegate *>::iterator i = nsswitch_$T.begin();
		while (i != nsswitch_$T.end()) {
			std::string name = (*i).first;
			NamespaceDelegate *d = (*i).second;
			v->${T}_dialect->delegates[name] = d;
			i++;
		}
	}
END
	}
	print "#undef NS_REG\n";
	print "}\n"; # end of registerAutoDelegates
} # end of variant loop
	
print "\nvoid registerAutoDelegates() {\n";
foreach my $variant (sort keys %{$data->{variants}}) {
	print "    registerAutoDelegates_$variant();\n";
}
print "}\n";


print "\nvoid dispatchCAOS(class caosVM *vm, int idx) {\n";
print "\tswitch (idx) {\n";

for (1..$#disp_tbl) {
	if (defined $disp_tbl[$_]) {
		printf "\tcase 0x%08X:\n", $_;
		print "\t\tvm->", $disp_tbl[$_], "();\n";
		print "\t\tbreak;\n";
	}
}

print <<END;
	default:
		{
			std::ostringstream oss;
			oss << "INTERNAL ERROR: idx not found, " << idx;
			throw creaturesException(oss.str());
		}
	}
}
END

exit 0;

our %cescapes;
BEGIN { %cescapes = (
	"\n" => "\\n",
	"\r" => "\\r",
	"\t" => "\\t",
	"\\" => "\\\\",
	"\"" => "\\\"",
); }

sub cescape {
	my $str = shift;
	my $ces = join "", keys %cescapes;
	$str =~ s/([\Q$ces\E])/$cescapes{$1}/ge;
	return $str;
}
# vim: set noet: 
