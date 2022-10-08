#!/usr/bin/perl
#
# Read HoiChess log file and print some statistics.

use warnings;
use strict;

my $sum_nodes = 0;
my $sum_nps = 0;
my $num_nps = 0;

while (<>) {
	if (/^Info: nodes_total=(\d+) /) {
		$sum_nodes += $1;
	} elsif (/^Info: searchtime=\d+.\d+ nps=(\d+)/) {
		$sum_nps += $1;
		$num_nps++;
	}
}

my $avg_nps = $num_nps != 0 ? int($sum_nps / $num_nps) : 0;
print("sum_nodes=$sum_nodes avg_nps=$avg_nps\n")
