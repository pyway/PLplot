#!/usr/bin/perl

@letters = ( "A", "B", "\$\\Gamma\$", "\$\\Delta\$", "E", "Z", "H", 
             "\$\\Theta\$", "I", "K", "\$\\Lambda\$", "M", 
             "N", "\$\\Xi\$", "O", "\$\\Pi\$", "P", "\$\\Sigma\$", "T", 
	     "\$\\Upsilon\$", "\$\\Phi\$", "X", "\$\\Psi\$", "\$\\Omega\$", 
             "\$\\alpha\$", "\$\\beta\$", "\$\\gamma\$", "\$\\delta\$", 
	     "\$\\epsilon\$", "\$\\zeta\$", "\$\\eta\$", "\$\\theta\$", 
	     "\$\\iota\$", "\$\\kappa\$", "\$\\lambda\$", "\$\\mu\$",
             "\$\\nu\$", "\$\\xi\$", "o", "\$\\pi\$", "\$\\rho\$", 
	     "\$\\sigma\$", "\$\\tau\$", "\$\\upsilon\$", "\$\\phi\$", 
	     "\$\\chi\$", "\$\\psi\$", "\$\\omega\$" );

open (MANIFEST, "> inline.manifest");

foreach $i (@letters) {
  $_ = $i;
  /([a-zA-Z]+)/;
  $jpeg_file = $1.".jpeg";
  $eps_file = $1.".eps";
  system "perl", "tex2eps.pl", "-s", $i, "-x", "5000", "-o", $eps_file;
  system "perl", "eps2jpeg.pl", "-g", "35%", "-i", $eps_file, "-o", $jpeg_file;
  system "rm", "-f", $eps_file;
  print MANIFEST "$jpeg_file\n";
}

close MANIFEST;
