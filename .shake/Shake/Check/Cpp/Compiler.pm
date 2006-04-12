# vim: set ts=4 sw=4 noexpandtab si ai sta tw=100:
# This module is copyrighted, see end of file for details.
package Shake::Check::Cpp::Compiler;
use strict;
use warnings;

use Shake::Check::Program;
use base 'Shake::Check::Program';

our $VERSION = 0.01;

sub initialize {
	my ($self) = @_;
	$self->{program} = 'c++';
}

sub desc {
	my ($self) = @_;

	return "checking for $self->{program}";
}

sub can_cache { 1 }

sub fullname { shift->prefix }
sub name { undef }

sub human_name {
	my $self = shift;
	return "a C compiler";
}

1;
__END__
=head1 NAME

Shake::Check::Program - description

=head1 SYNOPSIS

  use Shake::Check::Program;
  # Small code example.

=head1 DESCRIPTION

FIXME

=head1 METHODS

This class implements the following methods:

=head2 method1(Z<>)

...

=head1 BUGS

None known. Bug reports are welcome. Please use our bug tracker at
L<http://gna.org/bugs/?func=additem&group=haver>.

=head1 AUTHOR

Dylan William Hardison, E<lt>dhardison@cpan.orgE<gt>

=head1 SEE ALSO

L<http://www.haverdev.org/>.

=head1 COPYRIGHT and LICENSE

Copyright (C) 2006 by Dylan William Hardison. All Rights Reserved.

This module is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This module is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

1;
