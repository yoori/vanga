package CsvUtils::Process::NumNormalize;

use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::NumFilter: not defined 'field' argument";

  my @indexes = split(',', $params{'field'});
  my @res_indexes;
  foreach my $index(@indexes)
  {
    if(!looks_like_number($index))
    {
      die "CsvUtils::Process::Columns: incorrect column index: $index";
    }
    push(@res_indexes, $index - 1);
  }

  my $fields = { fields_ => \@res_indexes };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  foreach my $index(@{$self->{fields_}})
  {
    my $value = $row->[$index];

    $value =~ s/^\s+//;
    $value =~ s/\s+$//;
    $value =~ s/^["](.*)["]$/$1/;
    $value =~ s/^\s+//;
    $value =~ s/\s+$//;

    if(looks_like_number($value))
    {
      $value =~ s/([.]([0-9]*[1-9])?)0+$/$1/;
      $value =~ s/[.]$//;
      $row->[$index] = $value;
    }
  }

  return $row;
}

sub flush
{}

1;
