setenv blink_power 'led status off; sleep 0.1; led status on'

# erase flash
run blink_power blink_power
sf probe
sf erase 0 400000

# blink forever
while true; do run blink_power; sleep 1; done
