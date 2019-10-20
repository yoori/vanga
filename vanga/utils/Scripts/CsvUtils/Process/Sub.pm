package CsvUtils::Process::Sub;

use strict;
use Time::Local;
use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field1'}) ||
    die "CsvUtils::Process::Sub: not defined 'field1' argument";
  exists($params{'field2'}) ||
    die "CsvUtils::Process::Sub: not defined 'field2' argument";

  my $fields = { field1_ => $params{'field1'} - 1, field2_ => $params{'field2'} - 1 };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value1 = $row->[$self->{field1_}];
  my $value2 = $row->[$self->{field2_}];

  if(looks_like_number($value1) && looks_like_number($value2))
  {
    push(@$row, $value1 - $value2);
  }
  else
  {
    push(@$row, '');
  }

  return $row;
}

sub flush
{}

1;
