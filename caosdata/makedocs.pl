#!/usr/bin/perl

# todo: handle XYX# as well as XYX:

my @files = <../caosVM_*.cpp>;

my %doublecmds;
my %doublefuncs;

my $parsingdoc = 0;
my $count = 0;
my $doccount = 0;

my $doclines;
my $docs;
my $section;

my %funcs;
my %voids;

my $data;

my $table1 = '<table align=center border=1 cellpadding=3 cellspacing=0 width="99%"><tr><td class="command">';
my $table2 = '</td></tr><tr><td class="description">';
my $table3 = '</td></tr></table><br>';

sub writedata {
	my ($name, $docline, $type, $newname) = @_;

	my @sp = split(/\(/, $docline);
	my $count = int(@sp) - 2;

	die("unknown type $type passed to writedata") if ($type != 1) && ($type != 2);
	
	if ($newname eq $name) {
		# TODO: these should be v__P1_ in the code, with a fixed regexp
		if ($name eq "_UP_") {
			$data = $data . "FUNCDEFHACK(_UP_, 0, UP)\n";
			return;
		} elsif ($name eq "_P1_") {
			$data = $data . "FUNCDEFHACK(_P1_, 0, P1)\n";
			return;
		} elsif ($name eq "_P2_") {
			$data = $data . "FUNCDEFHACK(_P2_, 0, P2)\n";
			return;
		}

		$data = $data . "CMDDEF(" . $name if ($type == 1);
		$data = $data . "FUNCDEF(" . $name if ($type == 2);
		$count = 0 if $docline =~ /condition/;
		$data = $data . ", " . $count . ")\n";
		if ($docline =~ /condition/) {
			die("only commands can have conditions while processing " . $name) unless ($type == 1);
			$data = $data . 'cmds[phash_cmd(*(int *)"' . $name . '")].needscondition = true;' . "\n";
		}				
	} else {
		my $firstname = $newname;
		$firstname =~ s/(.*) .*/$1/;
														
	  if ($type == 1) {
			if (!$doublecmds{$firstname}) {
				$data = $data . "DBLCMDDEF(\"" . $firstname . "\")\n";
				$doublecmds{$firstname} = 1;
			}
			$data = $data . "DBLCMD(\"";
		} elsif ($type == 2) {
			if (!$doublefuncs{$firstname}) {
				$data = $data . "DBLFUNCDEF(\"" . $firstname . "\")\n";
				$doublefuncs{$firstname} = 1;
			}
			$data = $data . "DBLFUNC(\"";
		}
		$data = $data . $newname . "\", " . $name . ", " . $count . ")\n";
  }
}

sub writedocsanddata {
	my ($name, $type) = @_;

	my $newname = $name;
	if ($name =~ /_.+/) {
		my $one = $name;
		$one =~ s/(.*)_(.*)/$1/;
		my $two = $name;
		$two =~ s/(.*)_(.*)/$2/;
		if (length($one) == 3) {
			$one = $one . ":";
		}
		if (length($two) == 3) {
			# not sure if this is right for future cases - fuzzie
			$two = $two . "+"; # MESG WRT+
		}
		$newname = $one . " " . $two;
	}
	if (length($name) == 2) {
		$name = "_" . $name . "_";
		$newname = $name;
	}
	if ($doclines) {
	  my @lines = split(/\n/, $doclines);
		my $firstline = $lines[0];
		$firstline =~ s/^\s*(.*)\s*$/$1/;
		# todo: make sure the documentation command name is equivalent to $name
		writedata($name, $firstline, $type, $newname);
		
		my $thisdoc;
		$doclines =~ s/\n/<br>/;
		$firstline =~ s/^([^(]*)/<span class="command">$1<\/span>/;
		$thisdoc = $table1 . $firstline . $table2 . "\n";
		my $i = 0;
		my $j = 0;
		foreach (@lines) {
			$i++;
			next if ($i == 1);
			$_ =~ s/^\s*(.*)\s*$/$1/;
			next unless $_;
			$j++;
			$thisdoc .= "<p>" . $_ . "</p>\n";
		}
		$thisdoc .= '<p><font color="#ff0000">undocumented</font></p>' if ($j == 0);
		$thisdoc .= $table3 . "\n";
		$doccount++ unless ($j == 0);
		$count++;
		$$docs{$section}{$newname} .= $thisdoc;
	} else {
	  print "command/function '", $newname, "' wasn't processed because it has no documentation. add at least a prototype.\n";
	}
	
	$doclines = "";
}

open(docfile, ">caosdocs.html");

print docfile '<html><head><title>openc2e CAOS documentation</title><link rel=stylesheet href="caosdocs.css"></head><body>';

foreach my $fname (@files) {
	$section = $fname;
	$section =~ s/^\.\.\/caosVM_(.*).cpp$/$1/;
	open(filehandle, $fname);
	while(<filehandle>) {
		if (/^void caosVM::/) {
			my $type = 0;
			$_ =~ s/^void caosVM:://;
			$_ =~ s/\(\) {$//;
			$_ =~ s/\n//;
			if (/^c_/) {
				$_ =~ s/^c_//;
				$type = 1;
			} elsif (/^v_/) {
				$_ =~ s/^v_//;
				$type = 2;
			} else {
				printf "failed to understand " . $_ . "\n";
				next;
			}
			writedocsanddata($_, $type);
			$_ =~ s/(.*)(_.+)/$1/;
			if (length($_) == 3) {
				$_ = $_ . ":";
			}
			if (length($_) == 2) {
				$_ = "_" . $_ . "_";
			}
			if ($type == 1) {
				$voids{$_} = 1;
			} elsif ($type == 2) {
				$funcs{$_} = 1;
			}
		} elsif (/^\/\*\*$/) {
			$parsingdoc = 1;
		} elsif (/\*\//) {
			$parsingdoc = 0;
		} elsif ($parsingdoc) {
			$doclines = $doclines . $_;
		}
	}
	close(filehandle);
}

foreach my $sect (sort keys(%$docs)) {
	print docfile "<h1>" . $sect . "</h1>\n";
	my $t = $$docs{$sect};
	foreach my $data (sort keys(%$t)) {
		print docfile $$docs{$sect}{$data};
	}
}
# 6 below = number of openc2e-specific commands
my $p = int(($doccount - 6) * 100 / 666);
my $q = int(($count - 6) * 100 / 666);
print docfile "<p>" . $count . " commands: " . $q . "% of DS-era CEE; " . $doccount . " documented commands: " . $p . "% of DS-era CEE<br>(plus 6 openc2e-specific commands)</p>";
print docfile "</body></html>";
close(docfile);

# now, generate prehash files

open(outhandle, ">prehash_cmds.txt");
foreach my $key (keys(%voids)) {
	print outhandle $key, "\n";
}
close(outhandle);
open(outhandle, ">prehash_funcs.txt");
foreach my $key (keys(%funcs)) {
	print outhandle $key, "\n";
}
close(outhandle);

# then, generate caosdata.cpp

open(outhandle, ">caosdata.cpp");
open(templatehandle, "caosdata_template.cpp");
my $template;
while (<templatehandle>) {
	$template .= $_;
}
close(templatehandle);
$template =~ s/__CAOSGENERATED__/Automatically generated by makedocs.pl./;
$template =~ s/__CAOSMACROS__/$data/;
print outhandle $template;
close(outhandle);
