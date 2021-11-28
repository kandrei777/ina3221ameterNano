#!/bin/sh
# Start logging module data into the stdout.
# This script configures the serial port and starts data reading.
# Also, it adds:
# - The logcat-like time mark ("%m-%d %H:%M:%S") for easy merging with logcat file.
# - Ticks field for easy plotting data using gnuplot utility.
# - Moving average value. Update AVERAGE_OVER to change the number of points.

PORT=/dev/ttyUSB0
SPEED=115200
PERL=/usr/bin/perl

echo "Start: $PERL"

stty -F "$PORT" cs8 "$SPEED" ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts raw
cat "$PORT" | perl -e '
    use POSIX "strftime";
    my $base = time;
    my $AVERAGE_OVER = 3600; # number of periods for moving average
    my $CHID = 1;
    my $VALID = 3;    
    my %sum;
    my %summers;
    
    while(<>) {
        my @f = split /\s+/, $_;
        if ($f[0] eq "#") {
            print "@f";
        } else {
            my $ch = $f[$CHID];
            my $val = $f[$VALID];
            push @{ $summers{$ch} }, $val;
            $sum{$ch} += $val;
            $sum{$ch} -= shift @{ $summers{$ch} } if @{ $summers{$ch} } > $AVERAGE_OVER;
            push @f, sprintf("%.2f", $sum{$ch} / @{ $summers{$ch} });

            my $ticks = shift(@f) + $base;
            my $time = POSIX::strftime("%m-%d %H:%M:%S", localtime($ticks));
            printf("%s.%03d %.03f %s\n", $time, ($ticks - int($ticks))*1000, $ticks, join(" ", @f));
        }
    }
'


