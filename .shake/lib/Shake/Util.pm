# vim: set ts=4 sw=4 noexpandtab si ai sta tw=100:
# This module is copyrighted, see end of file for details.
package Shake::Util;
use strict;
use warnings;
use Exporter;
use base 'Exporter';

our $VERSION = 0.01;
our @EXPORT = ();
our @EXPORT_OK = qw( loaded_modules module_to_filename version_ge );

sub loaded_modules {
	my @modules;
	
	foreach my $file (keys %INC) {
		push @modules, filename_to_module($file);
	}
	
	return @modules;
}

sub module_to_filename {
	my $class = shift;
	$class =~ s!::!/!g;
	return "$class.pm";
}

sub filename_to_module {
	my $file = shift;
	$file =~ s!/!::!g;
	$file =~ s/\.pm$//;
	return $file;
}

sub version_ge {
	my ($ourversion, $reqversion) = @_;
	my @req = split(/\./, $reqversion);
	my @our = split(/\./, $ourversion);
	
	while (@req < @our) {
		push(@req, '0');
	}
	
	while (@our < @req) {
		push(@our, '0');
	}

	foreach my $i (0 .. $#our) {
		if ($req[$i] > $our[$i]) {
			return undef;
		}
	}
	
	return 1;
}


1;
