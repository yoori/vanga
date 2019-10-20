package CsvUtils::Process::RETransform;

use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::RETransform: not defined 'field' argument";

  my @re_arr;
  foreach my $key(sort keys %params)
  {
    if($key =~ m/^re/)
    {
      my $re = $params{$key};
      #print "RE: $re\n";
      push(@re_arr, qr/$re/);
    }
  }

  my $fields = { field_ => $params{'field'} - 1, re_ => \@re_arr };

  #print $fields->{re_};
  #print "\n";
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my @res_row = @$row;

  my $res_value = '';
  my $value = $row->[$self->{field_}];

  foreach my $re(@{$self->{re_}})
  {
    if($value =~ $re)
    {
      $res_value = $1;
      last;
    }
  }

  push(@res_row, $res_value);

  return \@res_row;
}

sub flush
{}

1;
