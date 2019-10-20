package CsvUtils::Process::KeywordExtract;

use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::KeywordExtract: not defined 'field' argument";

  my $fields = { field_ => $params{'field'} - 1 };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];

  if(ref($value) eq 'ARRAY')
  {
    my @res_arr;
    foreach my $sub_val(@$value)
    {
      push(@res_arr, extract_($sub_val));
    }
    $row->[$self->{field_}] = \@res_arr;
  }
  else
  {
    $row->[$self->{field_}] = extract_($value);
  }

  return $row;
}

sub flush
{}

sub extract_
{
  my ($text) = @_;
  my @words = split(/[^0-9a-zA-Z%]/, $text);
  my %res;
  foreach my $w(@words)
  {
    if(length($w) > 2)
    {
      $res{$w} = 1;
    }
  }
  return join(' ', keys %res);
}

1;
