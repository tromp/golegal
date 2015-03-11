#!/usr/bin/perl                                                                                                                                                                

use strict;
use warnings;

use File::Basename;
use Data::Dumper;
use Cwd 'abs_path';

# figure where i'm running from so i can set the module path
# print abs_path($0) . "\n";
# print dirname(abs_path($0)) . "\n";

# hash table to go board dimensions and expected
# number of legal moves
use lib dirname(abs_path($0));
use checkhash qw(:DEFAULT $legalpg);

# where are the shell binaries located
# apparently between rhel 6 and 7 then moved 'echo'
my $echoloc = "/bin";
if ( ! -f "$echoloc/echo" ) {
  $echoloc = "/usr/bin";
}
my $bcloc   = "/usr/bin";

# we'll store what we find in the output file in
# this global hash variable
my $grids;

# pull our data from standard in (pipe friendly)
open( FIFO, "< -" );
while ( my $tmp = <FIFO> ) {
    chomp($tmp);

    # if the line doesn't start with "newillegal" skip it
    if ( $tmp !~ m/^checkresults: / ) { next }

    # output format
    # 9 6 0 newillegal 1633388083117977960 needy 17301943860296361582 legal 2838789950505283677
    my @data = split( " ", $tmp );

    my $left    = $data[1];
    my $right   = $data[2];
    my $issqr   = $data[3];
    my $illegal = $data[5];
    my $needy   = $data[7];
    my $legal   = $data[9];

    # if the grid isn't rectangular skip it
    if ( $issqr != 0 ) { next }

    # since we're multi-core push the field up into array inside the hash
    push( @{ $grids->{$left}->{$right} }, $legal );

}
close(FIFO);

# loop over the grids we found, sorted by x then y
foreach my $left ( sort keys %{$grids} ) {
    foreach my $right ( sort keys %{ $grids->{$left} } ) {

        # join the values from each grid
        my $l1 = join( '+', @{ $grids->{$left}->{$right} } );

        # run them through bc to add
        # had trouble getting perl to calc number of 2^64
        # shell out to bc (icky but it works and it's quick)
        my $l2 = `$echoloc/echo "$l1" | $bcloc/bc`;
        chomp($l2);

        # pull the expected value from the legal moves grid
        my $l3 = $legalpg->{$left}->{$right};

        # some of the values are over 2^64 so we need to mod them down
        # shelling out to bc again
        my $l4 = `$echoloc/echo "$l3 % (2^64)" | $bcloc/bc`;
        chomp($l4);

        # check our computed value against the table, does it match?
        my $matched = " ";
        if ( $l2 == $l4 ) { $matched = "*" }

        printf( "%1s %02s,%02s => ", $matched, $left, $right );
        printf( "%s <-> ",           $l2 );
        printf( "%s",                $l4 );
        print "\n";
    }
}

exit(0);

