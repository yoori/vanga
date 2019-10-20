package CsvUtils::Process::GetTopPred;

use strict;
use warnings;

use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

use Class::Struct Factor => [key => '$', weight => '$'];

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'field'}) ||
    die "CsvUtils::Process::NumFilter: not defined 'field' argument";
  exists($params{'top'}) ||
    die "CsvUtils::Process::NumFilter: not defined 'top' argument";

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

  my $exclude_field;
  my $exclude_field_param = $params{'exclude_field'};
  if(defined($exclude_field_param))
  {
    if(!looks_like_number($exclude_field_param))
    {
      die "CsvUtils::Process::Columns: incorrect column index: $exclude_field_param";
    }

    $exclude_field = $exclude_field_param - 1;
  }

  my $fields = {
    fields_ => \@res_indexes,
    exclude_field_ => $exclude_field,
    top_ => $params{'top'}
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my @factors;
  my %exclude_values;

  if(defined($self->{exclude_field_}))
  {
    my $ex_str = $row->[$self->{exclude_field_}];
    my @ex_vals = split(' ', $ex_str);
    @ex_vals = grep { $_ ne '' } @ex_vals;

    foreach my $ex_val(@ex_vals)
    {
      $exclude_values{$ex_val} = 1;
    }
  }

  foreach my $index(@{$self->{fields_}})
  {
    my $value_pair = $row->[$index];
    my @vals = split(':', $value_pair);
    my $value = $vals[0];
    my $weight = $vals[1];

    if(!looks_like_number($weight))
    {
      die "invalid weight '$weight'";
    }

    if(!exists($exclude_values{$value}))
    {
      my $new_factor = new Factor(key => $value, weight => $weight);
      push(@factors, $new_factor);
    }
  }

  @factors = sort { $b->weight() <=> $a->weight() } @factors;
  my @res_factor_keys;

  for(my $i = 0; $i < $self->{top_} && $i < scalar(@factors); ++$i)
  {
    push(@res_factor_keys, $factors[$i]->key());
  }

  my $res_factors = join(' ', @res_factor_keys);
  push(@$row, $res_factors);

  return $row;
}

sub flush
{}

1;
