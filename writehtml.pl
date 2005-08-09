#!/usr/bin/perl
# vim: set noet: 
use strict;
use warnings;

use YAML;
use Template;

my $data = YAML::LoadFile('commandinfo.yml');

foreach my $key (keys %{$data->{ops}}) {
	$data->{ops}{$key}{key} = $key;
	$data->{ops}{$key}{hasdocs} = defined($data->{ops}{$key}{description});
	if ($data->{ops}{$key}{pragma}) {
	   my @pragma;
	   foreach my $pkey (sort keys %{$data->{ops}{$key}{pragma}}) {
		   push @pragma, { key => $pkey, value => $data->{ops}{$key}{pragma}{$pkey}};
	   }
	   $data->{ops}{$key}{pragma} = \@pragma;
   }
}

my $vars = {
	commands => [map {$data->{ops}{$_}} sort keys %{$data->{ops}}]
};

my $tt = new Template;
$tt->process(\*DATA, $vars) or die $tt->error(), "\n";

__END__
<html>
	<head>
		<title>CAOS command reference - openc2e</title>
		<link rel="stylesheet" type="text/css" href="docs.css" />
	</head>
	<body>
		<h1>CAOS command reference - openc2e dev build</h1>
		[% FOREACH op = commands %]
		<div class="command" id="c_[% op.key %]">
			<div class="header">
				<span class="cmdname">
					[% op.name %]
				</span>
				<span class="cmdtype">
					([% op.type %])
				</span>
				[% FOREACH arg = op.arguments -%]
				<span class="argname">
					[% arg.name %]
				</span>
				<span class="argtype">
					([% arg.type %])
				</span>
				[% END %]
			</div>
			<div class="description">
			[% UNLESS op.hasdocs %]
			<div class="nodocs">This command is currently undocumented.</div>
			[% ELSE %]
			<div class="docs">
				[% op.description %]
			</div>
			[% END %]
			</div>
			<div class="administrivia">
				<ul>
					<li>Implemented in: [% op.filename %]</li>
					<li>Implementation function (may be wrong):
						[% op.implementation %]
					</li>
					[% IF op.pragma %]
					<li>Pragmas:
					<ul>
						[% FOREACH pragma = op.pragma %]
						<li> [% pragma.key %] =&gt; [% pragma.value %] </li>
						[% END %]
					</ul>
					[% END %]
				</ul>
			</div>
		</div> <!-- command -->
		[% END %]
	</body>
</html>
