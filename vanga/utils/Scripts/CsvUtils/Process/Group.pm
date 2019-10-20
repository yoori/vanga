package CsvUtils::Process::Group;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Group: not defined 'field' argument";

  my $fields = {
    field_ => $params{'field'} - 1,
    rows_ => {}
    };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];
  my $group_key = defined($value) ? $value : '';

  if(!exists($self->{rows_}->{$group_key}))
  {
    my @res;
    for(my $i = 0; $i < scalar @$row; ++$i)
    {
      if($self->{field_} == $i)
      {
        push(@res, $row->[$i]);
      }
      elsif(ref($row->[$i]) eq 'ARRAY')
      {
        push(@res, $row->[$i]);
      }
      else
      {
        push(@res, [ $row->[$i] ]);
      }
    }
    $self->{rows_}->{$group_key} = \@res;
  }
  else
  {
    my $res = $self->{rows_}->{$group_key};
    for(my $i = 0; $i < scalar(@$row); ++$i)
    {
      if($self->{field_} == $i)
      {}
      elsif(ref($row->[$i]) eq 'ARRAY')
      {
        push(@{$res->[$i]}, @{$row->[$i]});
      }
      else
      {
        push(@{$res->[$i]}, $row->[$i]);
      }
    }
  }

  return undef;
}

sub flush
{
  my ($self) = @_;

  my @rows;
  foreach my $group_key(keys %{$self->{rows_}})
  {
    push(@rows, $self->{rows_}->{$group_key});
  }

  return \@rows;
}

1;
