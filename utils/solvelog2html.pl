#!/usr/bin/perl
#
# This is a simple script that reads the results of a testsuite run
# (command 'solve') from a HoiChess log file and generates a nice
# html table.
#
# Usage: hoichess -L logfile
#        (run 'solve' command)
# 	 solvelog2html.pl logfile > results.html

use warnings;
use strict;

my $positions_per_row = 10;

my $i = 0;
print_prolog("xyz");
while (<>) {
	chomp;
	#if (s/^solve: (.*)$/$1/) {
	#	$i = 0;
	#	next;
	#} els
	if (/^SOLVE\s+(\d+)\s+(\d+)\s+(.*)\s+([01])\s+(.*)/) {
		my ($total, $right, $move, $correct, $pos) =
			($1, $2, $3, $4, $5);
		print_result($pos, $move, $correct);
	} elsif (/^SOLVE_DONE\s+(\d+)\s+(\d+)/) {
		my ($total, $right) = ($1, $2);
		print_epilog($total, $right);
	}
}


###############################################################################

sub print_prolog {
	my ($testsuite) = @_;
	
	print("<p>Testsuite: $testsuite</p>");
	print('<table border="1">');
	print('<tr><td></td>');
	for (my $j=1; $j<=$positions_per_row; $j++) {
		print("<td align=\"center\">+$j</td>");
	}
	print("</tr>\n");
}

sub print_result {
	my ($pos, $move, $correct) = @_;
	
	if ($i % $positions_per_row == 0) {
		print('<tr>');
		print("<td align=\"right\">$i</td>");
	} 
	
	print('<td align="left">');
	print("$pos<br>");	
	if ($correct) {
		print('<font color="green">');
	} else {
		print('<font color="red">');
	}
	print("$move");	
	print('</font></td>');
	
	if ($i % $positions_per_row == $positions_per_row-1) {
		print("</tr>\n");
	}
	
	$i++;
}

sub print_epilog {
	my ($total, $correct) = @_;
	
	print('</table>');
	printf("<p>Correct: %d of %d (%d%%)</p>", $correct, $total,
		$total != 0 ? $correct/$total*100 : 0);
}

