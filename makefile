# Copyright 2012 Olivier Gillet.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

include anu/makefile

FIRMWARE = build/anu/anu.hex
BOOTLOADER = build/anu_bootloader/anu_bootloader.hex

bootstrap_all:
	make -f anu/bootloader/makefile
	make -f anu/makefile
	make -f anu/makefile fuses
	$(AVRDUDE) -B 1 $(AVRDUDE_COM_OPTS) $(AVRDUDE_ISP_OPTS) \
		-U flash:w:$(FIRMWARE):i \
		-U flash:w:$(BOOTLOADER):i \
		-U lock:w:0x2f:m