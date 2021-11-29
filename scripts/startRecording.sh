#!/usr/bin/bash

# Start logging module data into the stdout.
# This script configures the serial port and starts data reading.
# Also, it adds:
# - The logcat-like time mark ("%m-%d %H:%M:%S") for easy merging with logcat file.
# - Ticks field for easy plotting data using gnuplot utility.
# - Moving average value. Search for AVERAGE_OVER variable to change the number of points.

PORT=/dev/ttyUSB0
SPEED=115200
PERL=/usr/bin/perl
FILE_BASE=serial

# Comment to keep previous files
rm -- ${FILE_BASE}_all.log ${FILE_BASE}_CH?.log 2> /dev/null

stty -F "$PORT" cs8 "$SPEED" ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts raw
#cat log | perl -e '
cat "$PORT" | \
perl -e '
    use POSIX "strftime";
    my $base = 0;
    my $AVERAGE_OVER = 3600; # number of periods for moving average
    my $CHID = 1;
    my $VALID = 3;    
    my %sum;
    my %summers;
    $| = 1;
    while(<>) {
        my @f = split /\s+/, $_;
        if ($f[0] eq "#") {
            print "@f\n";
        } else {
            my $ch = $f[$CHID];
            my $val = $f[$VALID];
            push @{ $summers{$ch} }, $val;
            $sum{$ch} += $val;
            $sum{$ch} -= shift @{ $summers{$ch} } if @{ $summers{$ch} } > $AVERAGE_OVER;
            push @f, sprintf("%.2f", $sum{$ch} / @{ $summers{$ch} });

            my $ticks = shift(@f);
            $base = time - $ticks if ($base eq 0);
            $ticks += $base;        
            my $time = POSIX::strftime("%m-%d %H:%M:%S", localtime($ticks));
            printf("%s.%03d %.03f %s\n", $time, ($ticks - int($ticks))*1000, $ticks, join(" ", @f));
        }
    }
' | \
while read line
do
 # Output to stdout
 echo "$line"

 # if the line has channel tag then put in to the channel file.
 if [[ $line =~ ^[^#]+(CH[0-9]): ]] ; then    
    echo "$line" >> "${FILE_BASE}_${BASH_REMATCH[1]}.log"
 fi
 
 # Add to file
 echo "$line" >> "${FILE_BASE}_all.log" 
done

