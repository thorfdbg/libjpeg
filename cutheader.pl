#!/usr/bin/perl -w
#
# $Id: cutheader.pl,v 1.1 2012-06-02 10:27:13 thor Exp $
#

$line = <STDIN>;
if ($line =~ /\/\*\*/) {
    do {
	$line = <STDIN>;
    } while ($line !~ /\*\*\/$/);
} else {
    print $line;
}

while($line = <STDIN>) {
    print $line;
}

    
