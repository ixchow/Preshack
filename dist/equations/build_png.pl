#!/usr/local/bin/perl -w

#
# Call latex to build an image from an input file.
#
# Built with help from:
#  http://www.linuxjournal.com/article/7870

if (@ARGV != 1) {
	print STDERR "Please give me the name of an input file.\n";
	exit;
}


$in = $ARGV[0];
$out = $in . ".png";

unlink("texput.dvi");
unlink("texput.ps");
unlink("texput.pdf");
unlink("texput.png");
unlink("texput.aux");
unlink("texput.log");
unlink("texput.tex");
unlink($out);

open EQN, "<", $in;

open LATEX, ">", "texput.tex";

print LATEX <<'EOF';
\documentclass[10pt]{article}
\usepackage{amsmath}
\usepackage{amsfonts}
\usepackage{amssymb}
\usepackage{color}
\pagestyle{empty}
\begin{document}
\begin{displaymath}
EOF
while ($line = <EQN>) {
	print LATEX $line;
}
print LATEX <<'EOF';
\end{displaymath}
\end{document}
EOF

close LATEX;

$ret = system("latex --interaction=nonstopmode texput.tex > /dev/null");
if ($ret != 0) {
	print STDERR "Latex-ing equation fails.\n";
} else {
	$ret = system("dvips -G0 -j0 -E texput.dvi -o texput.ps > /dev/null 2> /dev/null");
	if ($ret != 0) {
		print STDERR "dvips-ing equation fails.\n";
	} else {
		$ret = system("gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=pdfwrite -dEPSCrop -sOutputFile=texput.pdf texput.ps > /dev/null 2> /dev/null");
		if ($ret != 0) {
			print STDERR "pdf-ing equation fails.\n";
		} else {
			$ret = system("gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=pnggray -dGraphicsAlphaBits=4 -dTextAlphaBits=4 -r1000 -sOutputFile=texput.png texput.pdf > /dev/null 2> /dev/null");
			if ($ret != 0) {
				print STDERR "png-ing equation fails.\n";
			} else {
				$ret = system("convert texput.png -depth 8 -level 10%,100% -background White -channel A -combine '$out' > /dev/null");
				if ($ret != 0) {
					print STDERR "convert-ing equation fails.\n";
				}
			}
		}
	}
}

unlink("texput.dvi");
unlink("texput.ps");
unlink("texput.pdf");
unlink("texput.png");
unlink("texput.aux");
unlink("texput.log");
unlink("texput.tex");
