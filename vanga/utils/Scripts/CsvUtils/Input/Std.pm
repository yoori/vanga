package CsvUtils::Input::Std;

use Text::CSV_XS;

sub new
{
  my $class = shift;
  my $fields = { csv_ => Text::CSV_XS->new({ binary => 1, eol => undef }) };
  return bless($fields, $class);
}

sub get
{
  my ($self) = @_;
  return $self->{csv_}->getline(*STDIN);

=for comment
  my $line = <STDIN>;

  if(defined($line))
  {
    chomp $line;
    #$line =~ s/\r$//;
    $self->{csv_}->parse($line);
    my @arr = $self->{csv_}->fields();
    return \@arr;
  }

  return undef;
=cut
}

1;
