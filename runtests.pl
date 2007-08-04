#!/usr/bin/perl
use strict;
use warnings;
use Test::Harness qw(&runtests $verbose);

my @tests = glob("unittests/*.cos");

for my $test (@tests) {
    my $wrapper = $test;
    $wrapper =~ s/cos$/pl/;
		my @variants = ();
		if ($wrapper =~ /\.((c[123av])(,c[123av])*)\.pl$/) {
			for my $v (split /,/, $1) {
				push @variants,$v;
			}
		}
		if (@variants == 0) {
			@variants = ('c3');
		}

		my $varr = '("'.join("\",\"",@variants).'")';
		#$varr = "(".$varr.")";
    open FILE, ">$wrapper";
    print FILE <<EOF;
#!/usr/bin/perl
for my \$v ($varr) {
	exec("./openc2e", "-n", "-d", "\$v"."data", "-g", "\$v", "-b", "$test");
}
EOF
    close FILE;
    chmod 0700, $wrapper;

    $test = $wrapper
}

$verbose = 0;
runtests(@tests);
map { unlink } @tests;
