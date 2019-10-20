package CsvUtils::Process::StringNormalize;

use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;

  my $res_indexes_ref = undef;
  if(exists($params{'field'}))
  {
    my @indexes = split(',', $params{'field'});
    my @res_indexes;
    foreach my $index(@indexes)
    {
      if(!looks_like_number($index))
      {
        die "CsvUtils::Process::StringNormalize: incorrect column index: $index";
      }
      push(@res_indexes, $index - 1);
    }

    $res_indexes_ref = \@res_indexes;
  }

  my $fields = { fields_ => $res_indexes_ref };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  if(defined($self->{fields_}))
  {
    foreach my $index(@{$self->{fields_}})
    {
      my $value = $row->[$index];

      $value =~ s/^\s+//;
      $value =~ s/\s+$//;
      $value =~ s/^["](.*)["]$/$1/;
      $value =~ s/^\s+//;
      $value =~ s/\s+$//;

      $row->[$index] = $value;
    }
  }
  else
  {
    for(my $index = 0; $index < scalar(@$row); ++$index)
    {
      my $value = $row->[$index];

      $value =~ s/^\s+//;
      $value =~ s/\s+$//;
      $value =~ s/^["](.*)["]$/$1/;
      $value =~ s/^\s+//;
      $value =~ s/\s+$//;

      $row->[$index] = $value;
    }
  }

  return $row;
}

sub flush
{}

1;
