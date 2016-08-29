#!/usr/bin/perl
use strict; 
use warnings;
our %topoarr;

sub trim { my $s = shift; chomp $s; $s =~ s/^\s+|\s+$//g;  return $s; }
sub get_nonempty_string { my $fh = shift; my $line = ""; do { $line = <$fh>; $line = trim( $line ); } while ( $line eq "" ); return $line; }
sub check_topolib {
	my $toposig = "db94.dat";
	my $fh = shift;
	my $line = "";
	$line = get_nonempty_string( $fh );
	$line = get_nonempty_string( $fh );
	if ( $line ne $toposig ) { die "Bad topology signature!\n"; }
}
sub parse_topolib {
	my $numres = 0;
	my $fname = shift;
	my $fh = shift;
	my $lett = shift;
	{ do {
		my $resI = get_nonempty_string( $fh );
		if ( $resI eq "STOP" ) { last; }
		my $buf = get_nonempty_string( $fh );
		my $cod = "???";
		if ( $buf =~ /([\w\+\-\*]+).*/ ) {
			$cod = $1;
		} else {
			die "missing residue code in topology: $fname ( buf = $buf )";
		}
		my $textbuf = "";
		if ( $lett eq "" ) { $textbuf .= "// $resI\n"; }
		do { 
			$buf = get_nonempty_string( $fh );
			if ( $buf =~ /\d+\s+([\w\-\+\*\']+)\s+[\w\-\+\*\']+\s+\w\s+[\-]*\d+\s+[\-]*\d+\s+[\-]*\d+\s+[0-9\-\+\.]+\s+[0-9\-\+\.]+\s+[0-9\-\+\.]+\s+([0-9\-\+\.]+)/ ) {
				if ( $1 ne "DUMM" ) {
					my $outstr;
					if ( $2 >= 0 ) { $outstr = sprintf( "%-4s  %-4s   %.5f", $cod.$lett, $1, $2 ); }
					else { $outstr = sprintf( "%-4s  %-4s  %.5f", $cod.$lett, $1, $2 ); }
					$textbuf .= "$outstr\n";
				}
			} elsif ( $buf =~ /\d+\s+([\w\-\+\*\']+)\s+[\w\-\+\*\']+\s+\w\s+[\-]*\d+\s+[\-]*\d+\s+[\-]*\d+\s+[0-9\-\+\.]+\s+[0-9\-\+\.]+\s+[0-9\-\+\.]+/ ) {
				if ( $1 ne "DUMM" ) {
					my $outstr = sprintf( "%-4s  %-4s   %.5f    // WARNING: missing charge!", $cod.$lett, $1, 0 );
					$textbuf .= "$outstr\n";
				}
			}
		} while ( $buf ne "DONE" );
		$textbuf .= "\n";
		$topoarr{$cod} .= $textbuf;
		++$numres;
	} while ( "XWider" ne "faggot" ); }
	return $numres;
}

my $FN_RESLIB_AMINO = "all_amino94.in";
my $FN_RESLIB_AMINO_NT = "all_aminont94.in";
my $FN_RESLIB_AMINO_CT = "all_aminoct94.in";
my $FN_RESLIB_NUCLEIC = "all_nuc94.in";
my $FN_OUTPUT = "qres.out";

open( my $reslib_amino, "<", $FN_RESLIB_AMINO ) or die "cannot open '$FN_RESLIB_AMINO' for reading: $!";
open( my $reslib_aminont, "<", $FN_RESLIB_AMINO_NT ) or die "cannot open '$FN_RESLIB_AMINO_NT' for reading: $!";
open( my $reslib_aminoct, "<", $FN_RESLIB_AMINO_CT ) or die "cannot open '$FN_RESLIB_AMINO_CT' for reading: $!";
open( my $reslib_nucleic, "<", $FN_RESLIB_NUCLEIC ) or die "cannot open '$FN_RESLIB_NUCLEIC' for reading: $!";
open( my $reslib_out, ">", $FN_OUTPUT ) or die "cannot open '$FN_OUTPUT' for writing: $!";

my $numres = 0;

check_topolib( $reslib_amino );
check_topolib( $reslib_aminont );
check_topolib( $reslib_aminoct );
check_topolib( $reslib_nucleic );

# read topology
$numres += parse_topolib( $FN_RESLIB_AMINO, $reslib_amino, "" );
$numres += parse_topolib( $FN_RESLIB_AMINO_NT, $reslib_aminont, "N" );
$numres += parse_topolib( $FN_RESLIB_AMINO_CT, $reslib_aminoct, "C" );
$numres += parse_topolib( $FN_RESLIB_NUCLEIC, $reslib_nucleic, "" );

# dump results
foreach my $key ( sort( keys %topoarr ) ) {
	print $reslib_out $topoarr{$key};
}

close $reslib_amino;
close $reslib_aminont;
close $reslib_aminoct;
close $reslib_nucleic;
close $reslib_out;

print "All done ($numres residues).";

