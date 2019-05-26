setenv blink_power 'led status off; sleep 0.1; led status on'
setenv blink_standby 'led status off; sleep 0.1; led status on'

# first read existing loader
run blink_power
sf probe

# or load rkspi_loader.img and write it to 0 offset of spi
# or fail badly

if size ${devtype} ${devnum}:${distro_bootpart} rkspi_loader.img; then
  load ${devtype} ${devnum}:${distro_bootpart} ${kernel_addr_r} rkspi_loader.img

  # erase flash
  run blink_power blink_power
  sf erase 0 400000

  # write flash
  run blink_power blink_power blink_power
  sf write ${kernel_addr_r} 0 ${filesize}

  # blink forever
  while true; do run blink_power; sleep 1; done
else
  # blink forever
  echo "missing rkspi_loader.img"
  while true; do run blink_standby; sleep 1; done
fi
