package CsvUtils::Output::Std;

use strict;
use warnings;
use Text::CSV_XS;
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my $fields = { csv_ => Text::CSV_XS->new({ binary => 1, eol => $/ }) };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;
  my $res_row = CsvUtils::Utils::prepare_row($row);
  $self->{csv_}->print(\*STDOUT, $res_row);
  return $row;
}

sub flush
{}

1;
