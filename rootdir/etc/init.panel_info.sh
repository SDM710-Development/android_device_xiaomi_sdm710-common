#!/vendor/bin/sh
# Copyright (c) 2009-2015, The Linux Foundation. All rights reserved.
# Copyright (c) 2021 Ivan Vecera <ivan@cera.cz>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of The Linux Foundation nor
#       the names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

DEV_PATHS="bus/i2c/devices/{3-0049,3-0020,3-0038,3-004a,3-0062}
           class/touch/touch_dev"

find_attr() {
	attr="$1"

	for dev in $DEV_PATHS; do
		path="/sys/$dev/$attr"
		test -f "$path" && break
		path=""
	done

	test -n "$path" && echo "$path"

	return $?
}

log() {
	/vendor/bin/log -p i -t panel-info-sh "$1"
}

get_attr() {
	attr="$1"	# attribute name
	value="$2"	# default value if not found

	for i in $(seq 5); do
		path=$(find_attr "$attr") || {
			log "No panel found yet. Trying again..."
			sleep 1
			continue
		}
		value=$(cat "$path")
		test -z "$value" && {
			log "Empty value for '$attr'. Trying again..."
			sleep 1
			continue
		}

		log "Succeeded to read '$attr' value='$value'"
		break
	done

	echo $value
}

# Get panel and touch device attributes
panel_color=$(get_attr panel_color)
panel_display=$(get_attr panel_display)
panel_vendor=$(get_attr panel_vendor)
touch_vendor=$(get_attr touch_vendor)

# Translate panel color value and set sys.panel.color property
case "$panel_color" in
	1) setprop sys.panel.color WHITE ;;
	2) setprop sys.panel.color BLACK ;;
	3) setprop sys.panel.color RED ;;
	4) setprop sys.panel.color YELLOW ;;
	5) setprop sys.panel.color GREEN ;;
	6) setprop sys.panel.color PINK ;;
	7) setprop sys.panel.color PURPLE ;;
	8) setprop sys.panel.color GOLDEN ;;
	9) setprop sys.panel.color SLIVER ;;
	@) setprop sys.panel.color GRAY ;;
	A) setprop sys.panel.color SLIVER_BLUE ;;
	B) setprop sys.panel.color CORAL_BLUE ;;
	*) setprop sys.panel.color UNKNOWN ;;
esac

# Translate panel display value and set sys.panel.display property
case "$panel_display" in
	1) setprop sys.panel.display JDI ;;
	2) setprop sys.panel.display LGD ;;
	3) setprop sys.panel.display SHARP ;;
	4) setprop sys.panel.display AUO ;;
	5) setprop sys.panel.display BOE ;;
	6) setprop sys.panel.display TIANMA ;;
	7) setprop sys.panel.display EBBG ;;
	8) setprop sys.panel.display SDC ;;
	9) setprop sys.panel.display EDO ;;
	0) setprop sys.panel.display OFILM ;;
	*) setprop sys.panel.display UNKNOWN ;;
esac

# Translate panel vendor value and set sys.panel.vendor property
case "$panel_vendor" in
	1) setprop sys.panel.vendor BIELTPB ;;
	2) setprop sys.panel.vendor LENS ;;
	3) setprop sys.panel.vendor WINTEK ;;
	4) setprop sys.panel.vendor OFILM ;;
	5) setprop sys.panel.vendor BIELD1 ;;
	6) setprop sys.panel.vendor TPK ;;
	7) setprop sys.panel.vendor LAIBAO ;;
	8) setprop sys.panel.vendor SHARP ;;
	9) setprop sys.panel.vendor JDI ;;
	@) setprop sys.panel.vendor EELY ;;
	A) setprop sys.panel.vendor GISEBBG ;;
	B) setprop sys.panel.vendor LGD ;;
	C) setprop sys.panel.vendor AUO ;;
	D) setprop sys.panel.vendor BOE ;;
	E) setprop sys.panel.vendor DSMUDONG ;;
	F) setprop sys.panel.vendor TIANMA ;;
	G) setprop sys.panel.vendor TRULY ;;
	H) setprop sys.panel.vendor SDC ;;
	I) setprop sys.panel.vendor PRIMAX ;;
	P) setprop sys.panel.vendor SZZC ;;
	Q) setprop sys.panel.vendor GVO ;;
	R) setprop sys.panel.vendor VITALINK ;;
	*) setprop sys.panel.vendor UNKNOWN ;;
esac

# Translate panel touch vendor value and set sys.panel.touch_vendor value
case "$touch_vendor" in
	1|2|3|4) setprop sys.panel.touch_vendor $touch_vendor ;;
              *) setprop sys.panel.touch_vendor UNKNOWN ;;
esac
