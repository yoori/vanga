package CsvUtils::Process::FloatReplaceByFile;

use Scalar::Util qw(looks_like_number);
use Class::Struct;
use Text::CSV_XS;
use CsvUtils::Utils;

use Class::Struct Rule => [min => '$', max => '$', replace => '$'];

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'file'}) ||
    die "CsvUtils::Process::InFile: not defined 'file' argument";
  exists($params{'field'}) ||
    die "CsvUtils::Process::InFile: not defined 'field' argument";

  # parse file
  my @values;
  my $add_empty_size = 0;

  {
    my $file = $params{'file'};
    my $csv = Text::CSV_XS->new({ binary => 1, eol => undef });
    open FILE, $file or die "Can't open $file";
    while(<FILE>)
    {
      my $line = $_;
      chomp $line;
      $csv->parse($line);
      my @arr = $csv->fields();
      push(@values, new Rule(min => $arr[0], max => $arr[1] + 0.00001, replace => $arr[2]));
    }
    close FILE;
  }

  my $fields = {
    values_ => \@values,
    field_ => $params{'field'} - 1,
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $cccnr = new Rule(min => 1);

  my $value = $row->[$self->{field_}];

  if(looks_like_number($value))
  {
    for(my $i = 0; $i < scalar @{$self->{values_}}; ++$i)
    {
      if($self->{values_}->[$i]->min() <= $value && $value < $self->{values_}->[$i]->max())
      {
        $row->[$self->{field_}] = $self->{values_}->[$i]->replace();
        last;
      }
    }
  }

  return $row;
}

sub flush
{}

1;
