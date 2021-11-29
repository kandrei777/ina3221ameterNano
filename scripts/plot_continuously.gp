# Plot channel data.
# Usage: gnuplot -p plot_continuously.gp

set xdata time
set timefmt "%s"

plot "serial_CH1.log" u 3:15 w l,\
 "serial_CH2.log" u 3:15 w l,\
 "serial_CH3.log" u 3:15 w l

while (1) {
    replot
    pause 1
}

